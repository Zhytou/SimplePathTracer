#ifndef SRE_TRACE_HPP
#define SRE_TRACE_HPP

#include <omp.h>

#include <algorithm>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "Camera.hpp"
#include "Ray.hpp"
#include "Triangle.hpp"
#include "Vec.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "../third-parties/tinyobjloader/tiny_obj_loader.h"

class Tracer {
 private:
  std::vector<Triangle> triangles;
  Camera camera;
  std::unordered_map<std::string, Vec3<float>> lights;
  size_t maxDepth;
  size_t samples;
  float thresholdP;

  void loadConfiguration(const std::string &configName);
  Vec3<float> cast(const Ray &ray);
  Vec3<float> trace(const Ray &ray, size_t depth);
  void shoot(const Ray &ray, HitResult &res);
  void printStatus();

 public:
  Tracer(size_t _depth = 8, size_t _samples = 100, float _p = 0.8)
      : maxDepth(_depth), samples(_samples), thresholdP(_p) {}

  void loadExampleScene();
  void load(const std::string &pathName, const std::string &fileName);
  cv::Mat render();
};

void Tracer::loadExampleScene() {
  float eyePosZ = 2;
  // Configuration
  camera.setWidth(500);
  camera.setHeight(500);
  camera.setFovy(90);
  camera.setPosition(0, 0, -eyePosZ);
  camera.setTarget(0, 0, -1);
  camera.setWorld(0, 1, 0);

  // Scene-Box-h,w,l
  float w = eyePosZ;
  float h = eyePosZ;
  float l = eyePosZ * 2;

  // Scene-Light
  Material m;
  m.setEmissive(true);
  m.setDiffusion(0, 0, 0);
  m.setSpecularity(0, 0, 0);
  m.setTransmittance(1, 1, 1);
  m.setShiness(1);
  m.setRefraction(0);
  triangles.emplace_back(Vec3<float>(-0.5, 2, 2), Vec3<float>(-0.5, 2, 0.5),
                         Vec3<float>(0.5, 2, 0.5), m);
  triangles.emplace_back(Vec3<float>(-0.5, 2, 2), Vec3<float>(0.5, 2, 2),
                         Vec3<float>(0.5, 2, 0.5), m);
  triangles.emplace_back(Vec3<float>(-0.5, 2, -2), Vec3<float>(-0.5, 2, -0.5),
                         Vec3<float>(0.5, 2, -0.5), m);
  triangles.emplace_back(Vec3<float>(-0.5, 2, -2), Vec3<float>(0.5, 2, -2),
                         Vec3<float>(0.5, 2, -0.5), m);

  // Scene-Ground
  m.setEmissive(false);
  m.setDiffusion(0.79, 0.76, 0.73);
  m.setSpecularity(0, 0, 0);
  m.setTransmittance(1, 1, 1);
  m.setShiness(1);
  m.setRefraction(0);
  triangles.emplace_back(Vec3<float>(-w * 3, -h, l),
                         Vec3<float>(-w * 3, -h, -1),
                         Vec3<float>(w * 3, -h, -1), m);
  triangles.emplace_back(Vec3<float>(-w * 3, -h, l), Vec3<float>(w * 3, -h, l),
                         Vec3<float>(w * 3, -h, -1), m);

  // Scene-Top
  m.setEmissive(false);
  m.setDiffusion(0.79, 0.76, 0.73);
  m.setSpecularity(0, 0, 0);
  m.setTransmittance(1, 1, 1);
  m.setShiness(1);
  m.setRefraction(0);
  triangles.emplace_back(Vec3<float>(-w, h, l), Vec3<float>(-w, h, -1),
                         Vec3<float>(w, h, -1), m);
  triangles.emplace_back(Vec3<float>(-w, h, l), Vec3<float>(w, h, l),
                         Vec3<float>(w, h, -1), m);

  // Scene-BackWall
  m.setEmissive(false);
  m.setDiffusion(0.79, 0.76, 0.73);
  m.setSpecularity(0, 0, 0);
  m.setTransmittance(1, 1, 1);
  m.setShiness(1);
  m.setRefraction(0);
  triangles.emplace_back(Vec3<float>(-w, -h, l), Vec3<float>(-w, h, l),
                         Vec3<float>(w, h, l), m);
  triangles.emplace_back(Vec3<float>(-w, -h, l), Vec3<float>(w, -h, l),
                         Vec3<float>(w, h, l), m);

  // Scene-RightWall
  m.setEmissive(false);
  m.setDiffusion(0.2, 0.76, 0);
  m.setSpecularity(0, 0, 0);
  m.setTransmittance(1, 1, 1);
  m.setShiness(1);
  m.setRefraction(0);
  triangles.emplace_back(Vec3<float>(-w, h, 0), Vec3<float>(-w, -h, 0),
                         Vec3<float>(-w, h, l), m);
  triangles.emplace_back(Vec3<float>(-w, h, l), Vec3<float>(-w, -h, l),
                         Vec3<float>(-w, -h, 0), m);

  // Scene-LeftWall
  m.setEmissive(false);
  m.setDiffusion(0, 0.24, 0.9);
  m.setSpecularity(0, 0, 0);
  m.setTransmittance(1, 1, 1);
  m.setShiness(1);
  m.setRefraction(0);
  triangles.emplace_back(Vec3<float>(w, h, 0), Vec3<float>(w, -h, 0),
                         Vec3<float>(w, h, l), m);
  triangles.emplace_back(Vec3<float>(w, h, l), Vec3<float>(w, -h, l),
                         Vec3<float>(w, -h, 0), m);

  // Scene-Shape
  /*
  m.setEmissive(false);
  m.setDiffusion(1, 0, 0);
  m.setSpecularity(0, 0, 0);
  m.setTransmittance(1, 1, 1);
  m.setShiness(1);
  m.setRefraction(0);
  triangles.emplace_back(Vec3<float>(-1, -h + 1, 1.5),
                         Vec3<float>(1, -h + 1, 1.5),
                         Vec3<float>(0, -h + 1, 0.5), m);
  */
  m.setEmissive(false);
  m.setDiffusion(0, 0.9, 0);
  m.setSpecularity(0, 0, 0);
  m.setTransmittance(1, 1, 1);
  m.setShiness(1);
  m.setRefraction(0);
  triangles.emplace_back(Vec3<float>(-1.2, 1, 1), Vec3<float>(-0.25, -1.5, 1),
                         Vec3<float>(-1.5, -1.5, 3), m);
  m.setEmissive(false);
  m.setDiffusion(0, 0, 0.8);
  m.setSpecularity(0, 0, 0);
  m.setTransmittance(1, 1, 1);
  m.setShiness(1);
  m.setRefraction(0);
  triangles.emplace_back(Vec3<float>(1.2, 1, 1), Vec3<float>(0.25, -1.5, 1),
                         Vec3<float>(1.5, -1.5, 3), m);

  printStatus();
}

void Tracer::loadConfiguration(const std::string &configName) {
  camera.setWidth(1024);
  camera.setHeight(1024);
  camera.setFovy(39.3077);
  camera.setPosition(278, 273, -800);
  camera.setTarget(278, 273, -799);
  camera.setWorld(0, 1, 0);

  lights.emplace("Light", Vec3<float>(34, 24, 8));
}

void Tracer::load(const std::string &pathName, const std::string &fileName) {
  // Configuration -Camera
  std::string configName = pathName + fileName + ".xml";
  loadConfiguration(configName);

  // Scene
  std::string modelName = pathName + fileName + ".obj";
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn;
  std::string err;
  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, &warn,
                        modelName.c_str(), pathName.c_str())) {
    throw std::runtime_error(warn + err);
  }
  /*
  for (int i = 0; i < 100; i += 3) {
    std::cout << "No." << i / 3 << " vertex: " << attrib.vertices[i] << '\t'
              << attrib.vertices[i + 1] << '\t' << attrib.vertices[i + 2]
              << '\n';
  }
  std::cout << std::endl;

  for (int i = 0; i < 100; i += 3) {
    std::cout << "No." << i / 3
              << " face: " << shapes[0].mesh.indices[i].vertex_index + 1 << '\t'
              << shapes[0].mesh.indices[i + 1].vertex_index + 1 << '\t'
              << shapes[0].mesh.indices[i + 2].vertex_index + 1 << '\n';
  }
  std::cout << std::endl;
  */

  std::vector<Material> actualMaterials;
  for (const auto &material : materials) {
    Material actualMaterial;

    if (lights.find(material.name) != lights.end()) {
      actualMaterial.setEmissive(true);
    }
    actualMaterial.setDiffusion(material.diffuse[0], material.diffuse[1],
                                material.diffuse[2]);
    actualMaterial.setSpecularity(material.specular[0], material.specular[1],
                                  material.specular[2]);
    actualMaterial.setTransmittance(material.transmittance[0],
                                    material.transmittance[1],
                                    material.transmittance[2]);
    actualMaterial.setShiness(material.shininess);
    actualMaterial.setRefraction(material.ior);
    actualMaterials.emplace_back(actualMaterial);
  }

  for (const auto &shape : shapes) {
    assert(shape.mesh.material_ids.size() ==
           shape.mesh.num_face_vertices.size());

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

      Vec3<float> point_normals[3];
      for (size_t point_i = 0; point_i < 3; point_i++) {
        int normal_index =
            shape.mesh.indices[face_i * 3 + point_i].normal_index;

        point_normals[point_i].x = attrib.normals[normal_index * 3 + 0];
        point_normals[point_i].y = attrib.normals[normal_index * 3 + 1];
        point_normals[point_i].z = attrib.normals[normal_index * 3 + 2];
      }

      Vec3<float> normal = cross(points[1] - points[0], points[2] - points[0]);
      if (dot(point_normals[0], normal) < 0) {
        normal = -normal;
      }

      Material material = actualMaterials[shape.mesh.material_ids[face_i]];

      triangles.emplace_back(points[0], points[1], points[2], normal, material);
    }
  }

  printStatus();
}

cv::Mat Tracer::render() {
  int height = camera.getHeight(), width = camera.getWidth();
  // 注意：CV_32F白色为（1，1，1）对应CV_8U的白色（255，255，255）
  cv::Mat img(cv::Size(width, height), CV_32FC3, cv::Scalar(0, 0, 0));

#pragma omp parallel for num_threads(500)
  for (int row = 0; row < height; row++) {
#pragma omp parallel for num_threads(500)
    for (int col = 0; col < width; col++) {
      Ray ray = camera.getRay(row, col);
      Vec3<float> color = cast(ray);

      img.at<cv::Vec3f>(row, col)[2] = color.x;
      img.at<cv::Vec3f>(row, col)[1] = color.y;
      img.at<cv::Vec3f>(row, col)[0] = color.z;
    }
  }

  return img;
}

void Tracer::shoot(const Ray &ray, HitResult &res) {
  HitResult curRes;
  for (size_t i = 0; i < triangles.size(); i++) {
    if (!res.isHit) {
      triangles[i].hit(ray, curRes);
      if (curRes.isHit) {
        res = curRes;
      }
    } else {
      triangles[i].hitCompare(ray, curRes, res.distance);
      if (curRes.isHit && res.distance > curRes.distance) {
        res = curRes;
      }
    }
  }
  return;
}

Vec3<float> Tracer::cast(const Ray &ray) {
  HitResult res;
  shoot(ray, res);

  if (!res.isHit) {
    return Vec3<float>(0, 0, 0);
  }

  // return Vec3<float>(1, 1, 1) * res.material.getDiffusion();
  if (res.material.isEmissive()) {
    // 发光
    return Vec3<float>(1, 1, 1);
  }

  Vec3<float> color(0, 0, 0);
  const float pdf = 1 / (2 * PI);

#pragma omp parallel for num_threads(10)
  for (int k = 0; k < samples; k++) {
    if (res.material.isDiffusive()) {
      // 漫反射
      Ray reflectRay =
          randomReflectRay(res.hitPoint, ray.getDirection(), res.normal);
      Vec3<float> reflectLightColor = trace(reflectRay, 0);
      color += reflectLightColor * res.material.getDiffusion();
    } else if (res.material.isSpecular()) {
      // 镜面反射
      Ray reflectRay =
          standardReflectRay(res.hitPoint, ray.getDirection(), res.normal);
      Vec3<float> reflectLightColor = trace(reflectRay, 0);
      color += reflectLightColor * res.material.getSpecularity();
    }

    if (res.material.isTransmissive()) {
      // 折射
      Ray refractRay =
          standardRefractRay(res.hitPoint, ray.getDirection(), res.normal,
                             res.material.getRefraction());
      Vec3<float> refractLightColor = trace(refractRay, 0);
      color += refractLightColor * res.material.getTransmittance();
    }
  }
  color /= samples * pdf;

  return color;
}

Vec3<float> Tracer::trace(const Ray &ray, size_t depth) {
  if (depth >= maxDepth) {
    return Vec3<float>(0, 0, 0);
  }

  HitResult res;
  shoot(ray, res);
  if (!res.isHit) {
    return Vec3<float>(0, 0, 0);
  }

  if (res.material.isEmissive()) {
    return Vec3<float>(1, 1, 1);
  }

  float possibility = randFloat(1);
  if (possibility > thresholdP) {
    return Vec3<float>(0, 0, 0);
  }

  Vec3<float> color(0, 0, 0);
  float cosine = fabs(dot(ray.getDirection(), res.normal));

  if (res.material.isDiffusive()) {
    // 漫反射
    Ray reflectRay =
        randomReflectRay(res.hitPoint, ray.getDirection(), res.normal);
    Vec3<float> reflectLightColor = trace(reflectRay, depth + 1) * cosine;
    color = reflectLightColor * res.material.getDiffusion();
  } else if (res.material.isSpecular()) {
    // 镜面反射
    Ray reflectRay =
        standardReflectRay(res.hitPoint, ray.getDirection(), res.normal);
    Vec3<float> reflectLightColor = trace(reflectRay, depth + 1) * cosine;
    color = reflectLightColor * res.material.getSpecularity();
  }

  if (res.material.isTransmissive()) {
    // 折射
    Ray refractRay =
        standardRefractRay(res.hitPoint, ray.getDirection(), res.normal,
                           res.material.getRefraction());
    Vec3<float> refractLightColor = trace(refractRay, depth + 1) * cosine;
    color += refractLightColor * res.material.getTransmittance();
  }

  return color / thresholdP;
}

void Tracer::printStatus() {
  // configuration
  std::cout << "sample number: " << samples << '\n'
            << "tracing depth: " << maxDepth << '\n';
  camera.printStatus();

  // shapes
  std::cout << "shapes" << '\n'
            << "triange number: " << triangles.size() << '\n';
  for (int i = 0; i < std::min(size_t(1), triangles.size()); i++) {
    std::cout << "No." << i << " triangle is following" << '\n';
    triangles[i].printStatus();
  }
  std::cout << std::endl;
}
#endif