#include <omp.h>
#include <chrono>
#include <iomanip>
#include <functional>

#include "Trace.hpp"
#include "Material.hpp"
#include "Triangle.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>

#define TINYXML2_HEADER_ONLY
#include <tinyxml2.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace spt {
Tracer::Tracer(size_t depth, size_t rrdepth, size_t samples, float prob)
    : scene(nullptr), maxd(depth), rrd(rrdepth), spp(samples), rrp(prob) {}

bool Tracer::load(const std::string& dir, const std::string &config, int bvhcnt) {
  // 1. load config
  // xml root
  tinyxml2::XMLDocument doc;
  doc.LoadFile((dir + config).c_str());
  if (doc.Error()) {
    std::cerr << "Error: Config load failure (file: " << dir+config << ")" << std::endl;
    return false;
  }
  tinyxml2::XMLElement* element = nullptr;
  
  // 1.1 camera
  element = doc.FirstChildElement("scene")->FirstChildElement("camera");  
  camera.setWidth(element->FloatAttribute("width"));  
  camera.setHeight(element->FloatAttribute("height"));  
  camera.setFovy(element->FloatAttribute("fovy"));
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

  // 1.2 light radiances
  std::unordered_map<std::string, Vec3<float>> lightRadiances;
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

  // 1.3 material illumination
  uint illuType = 0;
  element = doc.FirstChildElement("scene")->FirstChildElement("material");
  std::string type = element->Attribute("illutype");
  if (type == "microfacet") {
    illuType = BSDF_MICROFACET;
  } else if(type == "blinn_phong") {
    illuType = BSDF_BLINN_PHONG;
  } else {
    illuType = BSDF_PHONG;
  }

  // 1.4 model
  std::vector<std::string> models;
  element = doc.FirstChildElement("scene")->FirstChildElement("model");
  while(element != nullptr) {
    std::string name = element->Attribute("name");
    models.push_back(name);

    element = element->NextSiblingElement("model");
  }

  // 2. load models
  std::vector<std::shared_ptr<Hittable>> objects;
  for (auto model : models) {
    model = dir + model;
    if (!loadModel(model, dir, lightRadiances, illuType, objects)) {
      std::cerr << "Error: Model load failure (file: " << model << ")" << std::endl;
      return false;
    }
  }
  scene = BVH::constructBVH(objects, 0, objects.size(), bvhcnt);

  // 3. light
  light.setCDF();

  // 4. info
  std::cout << "Path Tracer Info:";
  std::cout << "\n----------------------";
  std::cout << "\nConfig " << config;
  std::cout << "\nImage " << camera.getHeight() << 'x' << camera.getWidth();
  std::cout << "\nCamera " << camera.getEye() << ' ' << camera.getLookAt() << ' ' << camera.getLookAt();
  std::cout << "\nScene " << scene->getSize() << ' ' << scene->getNodeCount() << ' ';
  for(auto model : models) {
    std::cout << model << ' ';
  }
  std::cout << "\nMax-depth " << maxd <<  " RR-depth " << rrp << " Samples per pixel "<< spp;
  std::cout << "\n----------------------\n";

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

void Tracer::render(const std::string& png) {
  const auto beg = std::chrono::steady_clock::now();
  int h = camera.getHeight(), w = camera.getWidth();
  std::vector<uint8_t> img(h * w * 3);

#pragma omp parallel for num_threads(30)
  for (int row = 0; row < h; row++) {
    for (int col = 0; col < w; col++) {
      Vec3<float> color(0, 0, 0);
      for (int k = 0; k < spp; k++) {
        Ray ray = camera.getRay(row, col);
        color += trace(ray, 0);
      }
      color /= spp;
      
      // gamma correction
      float gamma = 1.0f/2.2f;
      color = pow<float>(color, gamma) * 255.f;

      int idx = (row * w + col ) * 3;      
      img[idx + 0] = std::min(255.f, color.x);
      img[idx + 1] = std::min(255.f, color.y);
      img[idx + 2] = std::min(255.f, color.z);

      // show progress
      float percent = 100.f * (row * w + col) / (h * w - 1);
      auto cur = std::chrono::steady_clock::now();
      std::chrono::duration<float> dur = cur - beg;
      showProgress(percent, dur.count());
    }
  }

  int result = stbi_write_png(png.c_str(), w, h, 3, img.data(), w*3);
  return ;
}

Vec3<float> Tracer::trace(const Ray &rayv, size_t depth) {
  assert(scene != nullptr);

  // avoid infinite recursion
  if (depth >= maxd) {
    return Vec3(0.f, 0.f, 0.f);
  }

  // russian roulette
  float rrweight = 1.f;
  if (depth >= rrd) {
    if (rand(1.f) > rrp) {
      return Vec3(0.f, 0.f, 0.f);
    }
    rrweight = 1.f / rrp;
  }

  HitResult res;
  scene->hit(rayv, res);

  if (!res.hit) {
    return Vec3<float>(0.f, 0.f, 0.f);
  }
  assert(res.id >= 0 && res.id < scene->getSize());

  // view direction(wi)
  Vec3<float> V = -rayv.getDirection(); // P -> Eye
  // light direction(wo)
  Vec3<float> L(0.f, 0.f, 0.f); // P -> light

  // hit info
  Vec3<float> N = res.normal;
  Vec3<float> P = res.point;
  Vec2<float> UV = res.uv;
  Material mtl = res.material;
  float dis = depth == 0 ? 1.f : res.distance; // no attenuation for camera view

  // emissive light
  if (mtl.isEmissive()) {
    return mtl.getEmission(); // emissive luminosity
  }

  // output luminosity
  Vec3<float> Lum_o(0.f, 0.f, 0.f);
  // direct light
  // !NOTE: only GLOSSY and DIFFUSE support light sampling
  if (!mtl.isDelta()) {
    float PDF_d = 0.f; // probability density function for direct light sampling
    Vec3<float> Lum_d(0.f, 0.f, 0.f); // direct luminosity
    std::tie(Lum_d, L, PDF_d) = light.sample(scene, P);

    Vec3<float> BSDF = mtl.bsdf(V, N, L, UV);
    float NdotL = std::max(dot(N, L), 0.f);

    if (PDF_d > 0.f) {
      // multiple importance sampling
      float weight = mix(PDF_d, mtl.pdf(V, N, L, UV));
      Lum_o = Lum_d * BSDF * NdotL / PDF_d * weight * rrweight;
    }
  }

  // indirect light
  L = mtl.scatter(V, N, UV); // sample bsdf
  Ray rayl(P, L);
  Vec3<float> Lum_ind = trace(rayl, depth+1); // indirect luminosity
  Vec3<float> BSDF = mtl.bsdf(V, N, L, UV); // evaluate BSDF
  float NdotL = ::fabsf(dot(N, L)); // incident cosine

  // !NOTE: Monte Carlo in this framework is designed for continuous BSDFs integration(e.g., diffuse).
  // !      For perfect specular reflection or transmission (delta distributions), the direction is deterministic.
  // ! Consequently:
  // !  - The PDF is effectively infinite (Dirac delta), so we do NOT divide by PDF.
  // !  - The cosine term (N·L) is inherently handled by the delta function’s 
  // !    integration property and should NOT be explicitly multiplied.
  // ! Instead, we directly evaluate the reflected/transmitted radiance scaled by 
  // ! the Fresnel factor (and η² for transmission), ensuring energy conservation.
  if (mtl.isDelta()) {
    Lum_o = Lum_ind * BSDF * rrweight;
  } else {
    // probability density function for bsdf sampling and light sampling
    float PDF_ind = mtl.pdf(V, N, L, UV); 
    float PDF_d = light.pdf(scene, rayl);
    // multiple importance sampling
    float weight = mix(PDF_ind, PDF_d);
    if (PDF_ind > 0.f) {
      Lum_o += Lum_ind * BSDF * NdotL / PDF_ind * weight * rrweight;
    }
  }
  
  return Lum_o;
}

void Tracer::showProgress(float percent, float second) {
  const int barWidth = 50;
  std::cout << "[";
  int pos = static_cast<int>(barWidth * percent / 100.0f);
  for (int i = 0; i < barWidth; ++i) {
    if (i < pos) std::cout << "=";
    else if (i == pos) std::cout << ">";
    else std::cout << " ";
  }
  std::cout << "] " << std::setw(5) << std::fixed << std::setprecision(2) << percent;
  std::cout << "% " << std::setw(7) << second << 's';
  if (percent >= 100.0f) {
    std::cout << "\n";
  } else {
    std::cout << '\r';
    std::cout.flush();
  }
}

float mix(float pdf1, float pdf2) {
  // pdf1 *= pdf1;
  // pdf2 *= pdf2;
  return pdf1 / (pdf1 + pdf2);
}

}  // namespace spt
