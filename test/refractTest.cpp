#include <iostream>
#include <vector>

#include "../include/Ray.hpp"
#include "../include/Vec.hpp"

int main() {
  sre::Vec3<float> point(0, 0, 0);
  sre::Vec3<float> normal(0, 0, 1);
  sre::Vec3<float> idirection(sqrt(3), 0, 1);

  sre::Ray ray = sre::Ray::standardRefractRay(
      point, sre::Vec3<float>::normalize(idirection), normal, sqrt(3));
  auto origin = ray.getOrigin();
  auto direction = ray.getDirection();
  std::cout << "ray origin: " << origin.x << ' ' << origin.y << ' ' << origin.z
            << '\n'
            << "ray direction: " << direction.x << ' ' << direction.y << ' '
            << direction.z << '\n';
  std::cout << std::endl;
}