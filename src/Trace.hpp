#ifndef SRE_TRACE_HPP
#define SRE_TRACE_HPP

#include <omp.h>

#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
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

  Vec3<unsigned char> trace(const Ray &ray, size_t depth);
  HitResult shoot(const Ray &ray);

 public:
  Tracer() {}

  void load(const std::string &, const std::string &);
  cv::Mat render();
};

void Tracer::load(const std::string &pathName, const std::string &fileName) {
  std::string modelName = pathName + fileName + ".obj",
              configName = pathName + fileName + ".xml";

  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn;
  std::string err;
  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, &warn,
                        modelName.c_str(), pathName.c_str())) {
    throw std::runtime_error(warn + err);
  }

  for (const auto &shape : shapes) {
    assert(shape.mesh.material_ids.size() ==
           shape.mesh.num_face_vertices.size());

    size_t triagnleNum = shape.mesh.material_ids.size();
    for (size_t i = 0; i < triagnleNum; i++) {
      Vec3<float> points[3];

      for (size_t offset = 0; offset < 3; offset++) {
        int vertex_index = shape.mesh.indices[i * 3 + offset].vertex_index;

        points[offset].x = attrib.vertices[vertex_index + 0];
        points[offset].y = attrib.vertices[vertex_index + 1];
        points[offset].z = attrib.vertices[vertex_index + 2];
      }

      Material material;

      triangles.emplace_back(
          Triangle(points[0], points[1], points[2], material));
    }
  }
  std::cout << shapes.size() << ' ' << materials.size() << '\n';
  std::cout << std::endl;

  tinyxml2::XMLDocument doc;

  int errorId = doc.LoadFile(configName.c_str());

  /*
  if (errorId != 0) {
    std::cout << "load " << configName
              << " xml file failed at error id:" << errorId << std::endl;
    return;
  }
  */
  tinyxml2::XMLElement *root = doc.RootElement();

  camera.setWidth(1024);
  camera.setHeight(1024);
  camera.setPosition(278, 273, -800);
  camera.setTarget(278, 273, -800);
}

cv::Mat Tracer::render() {
  int height = camera.getWidth(), width = camera.getHeight();
  cv::Mat img(cv::Size(width, height), CV_8UC3, cv::Scalar(0, 0, 0));

#pragma omp parallel for num_threads(100)
  for (int row = height - 1; row >= 0; --row) {
    for (int col = 0; col < width; ++col) {
      Ray ray = camera.getRay(1, 2);
      Vec3<unsigned char> color(0, 0, 0);  // trace(ray, 0);

      img.at<cv::Vec3b>(row, col)[0] = color.x;
      img.at<cv::Vec3b>(row, col)[1] = color.y;
      img.at<cv::Vec3b>(row, col)[2] = color.z;
    }
  }

  return img;
}

HitResult Tracer::shoot(const Ray &ray) {
  HitResult res;
  for (const auto &triangle : triangles) {
    auto curRes = triangle.hit(ray);
    if (curRes.isHit && res.distance > curRes.distance) {
      res = curRes;
    }
  }
  return res;
}

Vec3<unsigned char> Tracer::trace(const Ray &ray, size_t depth) {
  HitResult res = shoot(ray);
  if (!res.isHit) {
    return Vec3<unsigned char>(0, 0, 0);
  }

  if (depth > maxDepth) {
    return Vec3<unsigned char>(0, 0, 0);
  }

  // Ray nray(res.hitPoint, )
  return Vec3<unsigned char>(0, 0, 0);
}

#endif