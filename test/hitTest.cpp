#include <iostream>
#include <vector>

#include "../src/Camera.hpp"
#include "../src/Triangle.hpp"

int main() {
  std::vector<Triangle> triangles;
  Material m;
  // m.setEmission(Vec3<float>(255, 255, 255));
  triangles.emplace_back(
      Triangle(Vec3<float>(-10, 40, 30), Vec3<float>(-10, 40, 10),
               Vec3<float>(10, 40, 10), Vec3<float>(0, -1, 0), m));
  triangles.emplace_back(
      Triangle(Vec3<float>(-10, 40, 30), Vec3<float>(10, 40, 30),
               Vec3<float>(10, 40, 10), Vec3<float>(0, -1, 0), m));
  Camera camera;
  camera.setWidth(500);
  camera.setHeight(500);
  camera.setFovy(90);
  camera.setPosition(0, 0, -50);
  camera.setTarget(0, 0, -9);
  camera.setWorld(0, 1, 0);
  camera.printStatus();

  Ray ray = camera.getRay(400, 250);
  auto origin = ray.getOrigin();
  auto direction = ray.getDirection();

  std::cout << "ray origin: " << origin.x << ' ' << origin.y << ' ' << origin.z
            << '\n'
            << "ray direction: " << direction.x << ' ' << direction.y << ' '
            << direction.z << '\n';
  HitResult res;
  for (auto& triangle : triangles) {
    triangle.hit(ray, res);
    auto point = ray.pointAt(res.distance);
    std::cout << (res.isHit ? "Yes" : "No") << '\n'
              << "t: " << res.distance << '\n'
              << "hit point: " << res.hitPoint.x << ' ' << res.hitPoint.y << ' '
              << res.hitPoint.z << '\n'
              << "normal: " << res.normal.x << ' ' << res.normal.y << ' '
              << res.normal.z << '\n'
              << "hit point by count: " << point.x << ' ' << point.y << ' '
              << point.z << '\n';
  }
  std::cout << std::endl;
}