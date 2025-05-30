#include <omp.h>
#include <algorithm>
#include <fstream>

#include "Trace.hpp"
#include "Material.hpp"
#include "Triangle.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace spt {
Tracer::Tracer(size_t _depth, size_t _samples, float _p)
    : scene(nullptr), maxDepth(_depth), samples(_samples), maxProb(_p) {}

bool Tracer::loadConfiguration(
    const std::string &configName,
    std::unordered_map<std::string, Vec3<float>> &lightRadiances) {
  std::ifstream ifs;
  ifs.open(configName, std::ios::in);
  if (!ifs.is_open()) {
    return false;
  }

  std::string buf;
  int i, j;

  int width, height;
  float fovy;
  float xyzs[3][3];

  // xml
  getline(ifs, buf);
  // camera
  getline(ifs, buf);
  i = 0;
  while (i < buf.size()) {
    switch (buf[i]) {
      case 'w':
        assert(buf[i + 1] == 'i' && buf[i + 2] == 'd' && buf[i + 3] == 't' &&
               buf[i + 4] == 'h' && buf[i + 5] == '=' && buf[i + 6] == '\"');
        i += 7;
        j = buf.find('\"', i);
        width = stoi(buf.substr(i, j - i));
        i = j + 1;
        break;
      case 'h':
        assert(buf[i + 1] == 'e' && buf[i + 2] == 'i' && buf[i + 3] == 'g' &&
               buf[i + 4] == 'h' && buf[i + 5] == 't' && buf[i + 6] == '=' &&
               buf[i + 7] == '\"');
        i += 8;
        j = buf.find('\"', i);
        height = stoi(buf.substr(i, j - i));
        i = j + 1;
        break;
      case 'f':
        assert(buf[i + 1] == 'o' && buf[i + 2] == 'v' && buf[i + 3] == 'y' &&
               buf[i + 4] == '=' && buf[i + 5] == '\"');
        i += 6;
        j = buf.find('\"', i);
        fovy = stof(buf.substr(i, j - i));
        i = j + 1;
        break;
      default:
        i += 1;
        break;
    }
  }

  // eye-lookat-up
  for (int row = 0; row < 3; row++) {
    getline(ifs, buf);
    i = 0;

    int col = 0;
    while (i < buf.size()) {
      if (i > 0 && buf[i - 1] == ' ' &&
          (buf[i] == 'x' || buf[i] == 'y' || buf[i] == 'z')) {
        assert(buf[i + 1] == '=' && buf[i + 2] == '\"');
        i += 3;
        j = buf.find('\"', i);
        xyzs[row][col] = stof(buf.substr(i, j - i));
        i = j + 1;
        col += 1;
      } else {
        i += 1;
      }
    }
  }

  // TODO: 修改loadConfig逻辑，保证staircase.xml也能被正常解读
  while (getline(ifs, buf)) {
    std::string mtlname;
    Vec3<float> radiance;
    i = 0;
    while (i < buf.size()) {
      if (i > 0 && buf[i - 1] == ' ' && buf[i] == 'm') {
        assert(buf.substr(i, 9) == "mtlname=\"");
        i += 9;
        j = buf.find('\"', i);
        mtlname = buf.substr(i, j - i);
        i = j + 1;
      } else if (i > 0 && buf[i - 1] == ' ' && buf[i] == 'r') {
        assert(buf.substr(i, 10) == "radiance=\"");
        i += 10;
        j = buf.find(',', i);
        radiance.x = stof(buf.substr(i, j - i));
        i = j + 1;

        j = buf.find(',', i);
        radiance.y = stof(buf.substr(i, j - i));
        i = j + 1;

        j = buf.find('\"', i);
        radiance.z = stof(buf.substr(i, j - i));
        i = j + 1;
      } else {
        i += 1;
      }
    }
    if (!mtlname.empty()) {
      lightRadiances.emplace(mtlname, radiance);
    }
  }

  camera.setWidth(width);
  camera.setHeight(height);
  camera.setFovy(fovy);
  camera.setEye(xyzs[0][0], xyzs[0][1], xyzs[0][2]);
  camera.setLookAt(xyzs[1][0], xyzs[1][1], xyzs[1][2]);
  camera.setWorld(xyzs[2][0], xyzs[2][1], xyzs[2][2]);
  return true;
}

bool Tracer::loadModel(
    const std::string &modelName, const std::string &pathName,
    const std::unordered_map<std::string, Vec3<float>> &lightRadiances) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn;
  std::string err;
  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, &warn,
                        modelName.c_str(), pathName.c_str())) {
    return false;
  }

  std::vector<Material> nmaterials;
  for (const auto &material : materials) {
    Material nmaterial(material, pathName);
    auto itr = lightRadiances.find(material.name);
    if (itr != lightRadiances.end()) {
      nmaterial.setEmission(itr->second);
    }
    nmaterials.emplace_back(nmaterial);
  }

  for (const auto &shape : shapes) {
    assert(shape.mesh.material_ids.size() == shape.mesh.num_face_vertices.size());

    size_t triagnleNum = shape.mesh.material_ids.size();
    for (size_t face_i = 0; face_i < triagnleNum; face_i++) {
      Vec3<float> points[3];
      for (size_t point_i = 0; point_i < 3; point_i++) {
        int vertex_index =
            shape.mesh.indices[face_i * 3 + point_i].vertex_index;

        points[point_i].x = attrib.vertices[vertex_index * 3 + 0];
        points[point_i].y = attrib.vertices[vertex_index * 3 + 1];
        points[point_i].z = attrib.vertices[vertex_index * 3 + 2];
      }

      Vec2<float> point_textures[3];
      for (size_t point_i = 0; point_i < 3; point_i++) {
        int texcoord_index =
            shape.mesh.indices[face_i * 3 + point_i].texcoord_index;

        if (texcoord_index * 2 + 1 < 0 ||
            texcoord_index * 2 + 0 >= attrib.texcoords.size()) {
          point_textures[point_i] = Vec2<float>(0, 0);
          continue;
        } 

        point_textures[point_i].u = attrib.texcoords[texcoord_index * 2 + 0];
        point_textures[point_i].v = attrib.texcoords[texcoord_index * 2 + 1];
      }
      // std::cout << "texture load success!\n";

      bool normalValid = true;
      Vec3<float> point_normals[3];
      for (size_t point_i = 0; point_i < 3; point_i++) {
        int normal_index =
            shape.mesh.indices[face_i * 3 + point_i].normal_index;

        if (normal_index * 3 + 2 < 0 ||
            normal_index * 3 + 0 >= attrib.normals.size()) {
          normalValid = false;
          break;
        }

        point_normals[point_i].x = attrib.normals[normal_index * 3 + 0];
        point_normals[point_i].y = attrib.normals[normal_index * 3 + 1];
        point_normals[point_i].z = attrib.normals[normal_index * 3 + 2];
      }

      Vec3<float> normal =
          Vec3<float>::cross(points[1] - points[0], points[2] - points[0]);
      if (normalValid && (point_normals[0] == point_normals[1] ||
                          point_normals[0] == point_normals[2] ||
                          point_normals[1] == point_normals[2])) {
        if (point_normals[0] == point_normals[1] ||
            point_normals[0] == point_normals[2]) {
          normal = point_normals[0];
        } else {
          normal = point_normals[1];
        }
      } else {
        if (Vec3<float>::dot(point_normals[0], normal) < 0) {
          normal = -normal;
        }
      }

      Material material = nmaterials[shape.mesh.material_ids[face_i]];
      auto object = std::make_shared<Triangle>(objects.size(), points[0], points[1], points[2], point_textures[0], point_textures[1], point_textures[2], normal, material);
      if (material.isEmissive()) {
        light.setLight(object);
      }
      objects.push_back(object);
    }
  }

  return true;
}

void Tracer::load(const std::string &pathName, const std::vector<std::string> &modelNames,
                  const std::string &configName, int bvhMinCount) {
  // Camera
  std::unordered_map<std::string, Vec3<float>> lightRadiances;
  std::string config = pathName + configName;
  if (!loadConfiguration(config, lightRadiances)) {
    std::cout << "Camera config loading fails!" << std::endl;
    return;
  }
  std::cout << "Camera config loading success!" << std::endl;
  
  // Scene
  for (auto modelName : modelNames) {
    std::string model = pathName + modelName;
    if (!loadModel(model, pathName, lightRadiances)) {
      std::cout << "Model loading fails!" << std::endl;
      return;
    }
  }
  std::cout << "Model loading success!" << std::endl;
  scene = BVH::constructBVH(objects, 0, objects.size(), bvhMinCount);

  printStatus();
}

void Tracer::render(const std::string& imgName) {
  int h = camera.getHeight(), w = camera.getWidth();
  std::vector<uint8_t> img(h * w * 3);

#pragma omp parallel for num_threads(20)
  for (int row = 0; row < h; row++) {
    for (int col = 0; col < w; col++) {
      Vec3<float> color(0, 0, 0);
      for (int k = 0; k < samples; k++) {
        Ray ray = camera.getRay(row, col);
        color += trace(ray, 0);
      }
      color /= samples;
      
      // gamma correction
      float gamma = 1.0f/2.2f;
      color = Vec3<float>::pow(color, gamma) * 255.f;

      int idx = (row*w+col)*3;      
      img[idx+0] = std::min(255.f, color.x);
      img[idx+1] = std::min(255.f, color.y);
      img[idx+2] = std::min(255.f, color.z);
    }
  }

  int result = stbi_write_png(imgName.c_str(), w, h, 3, img.data(), w*3);
  if (result) {
    std::cout << "Render success!" << std::endl;
  } 

  return ;
}

Vec3<float> Tracer::trace(const Ray &rayv, size_t depth) {
  assert(scene != nullptr);
  if (depth >= maxDepth) {
    return Vec3<float>(0, 0, 0);
  }

  HitResult res;
  scene->hit(rayv, res);

  if (!res.isHit) {
    return Vec3<float>(0, 0, 0);
  }
  assert(res.id >= 0 && res.id < objects.size());

  // view direction
  Vec3<float> V = -rayv.getDirection(); // P -> Eye

  // hit info
  Vec3<float>& N = res.normal;
  Vec3<float>& P = res.hitPoint;
  Material& mtl = res.material;
  float dis = res.distance;

  // tex coord
  Vec2<float> UV = objects[res.id]->getTexCoord(P);
  // update hit point
  P = P + N * EPSILON; // move, because of percision
  // origin point
  Vec3<float> PP = rayv.getOrigin();

  if (mtl.isEmissive()) {
    // ?
    if (PP == camera.getEye()) {
      dis = 1.f;
    }
    return mtl.getEmission() / (dis * dis);
  }

  // importance sampling result
  Vec3<float> L(0.f, 0.f, 0.f); // light direction
  float PDF = 0.f; // Probability density function

  if (randFloat(1) < 0.5) {
    // sample light
    std::tie(L, PDF) = light.sampleLight(scene, P);
  } else {
    // sample brdf
    std::tie(L, PDF) = mtl.sample(V, N);
  }

  float NdotL = std::max(Vec3<float>::dot(N, L), 0.f);
  if (PDF < EPSILON || NdotL < EPSILON) {
    return Vec3<float>(0.f, 0.f, 0.f);
  }

  Ray rayl(P, L);
  // input light
  Vec3<float> L_i = trace(rayl, depth+1);
  
  // evaluate BxDF
  Vec3<float> BxDF = mtl.eval(V, N, L, UV);

  // output light
  Vec3<float> L_o = L_i * BxDF * NdotL / PDF;

  return L_o;
}

void Tracer::printStatus() {
  // configuration
  std::cout << "sample number: " << samples << '\n'
            << "tracing depth: " << maxDepth << '\n'
            << "threshod probability: " << maxProb << '\n';
  camera.printStatus();
  
  // shapes
  std::cout << "shapes" << '\n'
            << "triange number: " << objects.size() << '\n';

  // scenes
  // std::cout << "scenes" << '\n';
  // scenes->printStatus();
}
}  // namespace spt
