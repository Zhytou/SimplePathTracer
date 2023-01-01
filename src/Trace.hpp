#ifndef SRE_TRACE_HPP
#define SRE_TRACE_HPP

#include <omp.h>

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
#include "../third-parties/tinyxml2/tinyxml2.h"

class Tracer {
 private:
  std::vector<Triangle> triangles;
  Camera camera;
  size_t maxDepth;

  Vec3<unsigned char> cast(const Ray &ray);
  Vec3<unsigned char> trace(const Ray &ray, size_t depth);
  HitResult shoot(const Ray &ray);

 public:
  Tracer() {}

  void loadExampleScene();
  void load(const std::string &, const std::string &);
  cv::Mat render();
};

void Tracer::loadExampleScene() {
  // Configuration
  camera.setWidth(500);
  camera.setHeight(500);
  camera.setFovy(90);
  camera.setPosition(0, 0, -100);
  camera.setTarget(0, 0, -9);
  camera.setWorld(0, 1, 0);
  camera.printStatus();

  // Scene
  Material m;
  Vec3<float> p1(0, 0, 0);
  Vec3<float> p2(0, 50, 0);
  Vec3<float> p3(10, 50, 0);
  triangles.emplace_back(Triangle(p1, p2, p3, m));
}

void Tracer::load(const std::string &pathName, const std::string &fileName) {
  // Configuration
  std::string configName = pathName + fileName + ".xml";
  std::unordered_map<std::string, Vec3<float>> lights = {
      {"Light", Vec3<float>(34, 24, 8)}};

  tinyxml2::XMLDocument doc;
  int errorId = doc.LoadFile(configName.c_str());
  /*
  if (errorId != 0) {
    std::cout << "load " << configName
              << " xml file failed at error id:" << errorId << std::endl;
    return;
  }
  tinyxml2::XMLElement *root = doc.RootElement();*/

  camera.setWidth(1024);
  camera.setHeight(1024);
  camera.setFovy(39.3077);
  camera.setPosition(278, 273, -800);
  camera.setTarget(278, 273, -799);
  camera.setWorld(0, 1, 0);

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

  std::vector<Material> actualMaterials;

  for (const auto &material : materials) {
    Material actualMaterial;

    if (lights.find(material.name) != lights.end()) {
      actualMaterial.setRadiance(lights[material.name]);
    }
    actualMaterial.setDiffuse(material.diffuse[0], material.diffuse[1],
                              material.diffuse[2]);
    actualMaterial.setSpecular(material.specular[0], material.specular[1],
                               material.specular[2]);

    actualMaterials.emplace_back(actualMaterial);
  }

  for (const auto &shape : shapes) {
    assert(shape.mesh.material_ids.size() ==
           shape.mesh.num_face_vertices.size());

    for (int i = 0; i < 9; i += 3) {
      for (size_t offset = 0; offset < 3; offset++) {
        std::cout << attrib.vertices[i + offset] << ' ';
      }
      std::cout << '\n';
      for (size_t offset = 0; offset < 3; offset++) {
        std::cout << attrib.normals[i + offset] << ' ';
      }
      std::cout << '\n';

      for (size_t offset = 0; offset < 3; offset++) {
        std::cout << shapes[0].mesh.indices[i + offset].vertex_index << ' ';
      }
      std::cout << '\n';
    }
    std::cout << std::endl;

    size_t triagnleNum = shape.mesh.material_ids.size();
    for (size_t i = 0; i < triagnleNum; i++) {
      Vec3<float> points[3];
      for (size_t offset = 0; offset < 3; offset++) {
        int vertex_index = shape.mesh.indices[i * 3 + offset].vertex_index;

        points[offset].x = attrib.vertices[vertex_index + 0];
        points[offset].y = attrib.vertices[vertex_index + 1];
        points[offset].z = attrib.vertices[vertex_index + 2];
      }

      Vec3<float> point_normals[3];
      for (size_t offset = 0; offset < 3; offset++) {
        int normal_index = shape.mesh.indices[i * 3 + offset].normal_index;

        point_normals[offset].x = attrib.normals[normal_index + 0];
        point_normals[offset].y = attrib.normals[normal_index + 1];
        point_normals[offset].z = attrib.normals[normal_index + 2];
      }

      Vec3<float> normal = cross(points[1] - points[0], points[2] - points[0]);
      if (dot(point_normals[0], normal) < 0) {
        normal = -normal;
      }

      Material material = actualMaterials[shape.mesh.material_ids[i]];

      triangles.emplace_back(
          Triangle(points[0], points[1], points[2], normal, material));
    }
  }
}

cv::Mat Tracer::render() {
  int height = camera.getWidth(), width = camera.getHeight();
  cv::Mat img(cv::Size(width, height), CV_8UC3, cv::Scalar(0, 0, 0));

#pragma omp parallel for num_threads(1000)
  for (int row = 0; row < height; row++) {
    for (int col = 0; col < width; col++) {
      Ray ray = camera.getRay(row, col);
      Vec3<unsigned char> color = cast(ray);
      break;
      // std::cout << static_cast<int>(color.x) << ' ' <<
      // static_cast<int>(color.y)
      //           << ' ' << static_cast<int>(color.z) << '\n';
      img.at<cv::Vec3b>(row, col)[0] = color.x;
      img.at<cv::Vec3b>(row, col)[1] = color.y;
      img.at<cv::Vec3b>(row, col)[2] = color.z;
    }
  }
  std::cout << std::endl;
  return img;
}

HitResult Tracer::shoot(const Ray &ray) {
  HitResult res;
  for (size_t i = 0; i < triangles.size(); i++) {
    auto curRes = triangles[i].hit(ray);
    if (curRes.isHit && res.distance > curRes.distance) {
      res = curRes;
    }
  }
  return res;
}

Vec3<unsigned char> Tracer::cast(const Ray &ray) {
  HitResult res = shoot(ray);

  if (!res.isHit) {
    return Vec3<unsigned char>(0, 255, 0);
  }

  if (res.material.illuminated()) {
    return res.material.getColor();
  } else {
    return Vec3<unsigned char>(0, 0, 255);
  }
}

Vec3<unsigned char> Tracer::trace(const Ray &ray, size_t depth) {
  if (depth > maxDepth) {
    return Vec3<unsigned char>(0, 0, 0);
  }

  HitResult res = shoot(ray);
  if (!res.isHit) {
    return Vec3<unsigned char>(0, 0, 0);
  }

  Ray nray = randomRay(res.hitPoint, res.normal);

  return Vec3<unsigned char>(0, 0, 0);
}

#endif