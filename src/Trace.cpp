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

#define TINYXML2_HEADER_ONLY
#include <tinyxml2.h>

namespace spt {
Tracer::Tracer(size_t _depth, size_t _samples, float _p)
    : scene(nullptr), maxDepth(_depth), samples(_samples), maxProb(_p) {}

bool Tracer::loadConfig(const std::string &config, std::unordered_map<std::string, Vec3<float>> &lightRadiances, uint& illuType) {
  // xml root
  tinyxml2::XMLDocument doc;
  doc.LoadFile(config.c_str());
  if (doc.Error()) {
    return false;
  }

  // xml element
  tinyxml2::XMLElement* element = nullptr;
  
  // camera element
  element = doc.FirstChildElement("scene")->FirstChildElement("camera");
  camera.setWidth(element->FloatAttribute("width"));
  camera.setHeight(element->FloatAttribute("height"));
  camera.setFovy(element->FloatAttribute("fovy"));

  // camera sub element
  std::string names[3] = {"eye", "lookat", "up"};
  for (auto name : names) {
    tinyxml2::XMLElement* subelem = element->FirstChildElement(name.c_str());
    float x, y, z;
    x = subelem->FloatAttribute("x");
    y = subelem->FloatAttribute("y");
    z = subelem->FloatAttribute("z");
    
    if (name == "eye") {
      camera.setEye(x, y, z);
    } else if (name == "lookat") {
      camera.setLookAt(x, y, z);
    } else {
      camera.setUp(x, y, z);
    }
  }

  // light radiances elements
  element = doc.FirstChildElement("scene")->FirstChildElement("light");
  while(element != nullptr) {
    std::string mtlname = element->Attribute("mtlname");
    std::string radiance = element->Attribute("radiance");
    std::vector<std::string> radiances = split(radiance, ',');

    assert(radiances.size() == 3);
    float x = std::stof(radiances[0]);
    float y = std::stof(radiances[1]);
    float z = std::stof(radiances[2]);
    lightRadiances[mtlname] = Vec3<float>(x, y, z);
    
    element = element->NextSiblingElement("light");
  }

  // material illumination type
  element = doc.FirstChildElement("scene")->FirstChildElement("material");
  std::string type = element->Attribute("illutype");
  if (type == "microfacet") {
    illuType = BSDF_MICROFACET;
  } else if(type == "blinn_phong") {
    illuType = BSDF_BLINN_PHONG;
  } else {
    illuType = BSDF_PHONG;
  }

  return true;
}

bool Tracer::loadModel(const std::string &model, const std::string &dir, const std::unordered_map<std::string, Vec3<float>> &lightRadiances, uint illuType, std::vector<std::shared_ptr<Hittable>>& objects) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn;
  std::string err;
  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model.c_str(), dir.c_str())) {
    std::cerr << err << std::endl;
    return false;
  }

  std::vector<Material> nmaterials;
  for (const auto &material : materials) {
    Material nmaterial(material, dir, illuType);
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
        int vertex_index = shape.mesh.indices[face_i * 3 + point_i].vertex_index;

        points[point_i].x = attrib.vertices[vertex_index * 3 + 0];
        points[point_i].y = attrib.vertices[vertex_index * 3 + 1];
        points[point_i].z = attrib.vertices[vertex_index * 3 + 2];
      }

      Vec2<float> point_textures[3];
      for (size_t point_i = 0; point_i < 3; point_i++) {
        int texcoord_index = shape.mesh.indices[face_i * 3 + point_i].texcoord_index;

        if (texcoord_index * 2 + 1 < 0 ||
            texcoord_index * 2 + 0 >= attrib.texcoords.size()) {
          point_textures[point_i] = Vec2<float>(0, 0);
          continue;
        } 

        point_textures[point_i].u = attrib.texcoords[texcoord_index * 2 + 0];
        point_textures[point_i].v = attrib.texcoords[texcoord_index * 2 + 1];      
      }

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

      Vec3<float> normal = cross(points[1] - points[0], points[2] - points[0]);
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
        if (dot(point_normals[0], normal) < 0) {
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

void Tracer::load(const std::string &dir, const std::vector<std::string> &models, const std::string &config, int bvhMinCount) {
  // camera, light and material type
  std::unordered_map<std::string, Vec3<float>> lightRadiances;
  uint illuType;
  if (!loadConfig(dir+config, lightRadiances, illuType)) {
    std::cerr << "Error: Config load failure (file: " << config << ")" << std::endl;
    return;
  }
  
  // scene
  std::vector<std::shared_ptr<Hittable>> objects;
  for (auto model : models) {
    if (!loadModel(dir+model, dir, lightRadiances, illuType, objects)) {
      std::cerr << "Error: Model load failure (file: " << model << ")" << std::endl;
      return;
    }
  }
  scene = BVH::constructBVH(objects, 0, objects.size(), bvhMinCount);
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
      color = pow(color, gamma) * 255.f;

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

  if (!res.hit) {
    return Vec3<float>(0, 0, 0);
  }
  assert(res.id >= 0 && res.id < scene->getSize());

  // view direction
  Vec3<float> V = -rayv.getDirection(); // P -> Eye

  // hit info
  Vec3<float>& N = res.normal;
  Vec3<float>& P = res.point;
  Vec2<float>& UV = res.uv;
  Material& mtl = res.material;
  float dis = res.distance;
  
  // update hit point
  P = P + N * EPSILON; // move, because of percision
  // origin point
  Vec3<float> PP = rayv.getOrigin();

  if (mtl.isEmissive()) {
    // ?
    if (PP == camera.getEye()) {
      dis = 1.f;
    }
    return mtl.getEmission() ;
  }

  // importance sampling result
  Vec3<float> L(0.f, 0.f, 0.f); // light direction
  float PDF = 0.f; // probability density function

  if (rand<float>(1) < 0.5) {
    // sample light
    std::tie(L, PDF) = light.sampleLight(scene, P);
  } else {
    // sample bsdf
    std::tie(L, PDF) = mtl.scatter(V, N);
  }

  if (PDF < EPSILON) {
    return Vec3<float>(0.f, 0.f, 0.f);
  }

  Ray rayl(P, L);
  // input light
  Vec3<float> L_i = trace(rayl, depth+1);
  
  // evaluate BSDF
  Vec3<float> BSDF = mtl.bsdf(V, N, L, UV);

  // incident cosine
  float NdotL = fabs(dot(N, L));

  // output light
  Vec3<float> L_o = L_i * BSDF * NdotL / PDF;

  return L_o;
}

}  // namespace spt
