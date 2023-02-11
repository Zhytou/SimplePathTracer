#include <iostream>
#include <vector>

#include "../src/Camera.hpp"
#include "../src/Triangle.hpp"

int main() {
  /*
   Vec3<float> backWallColor(255, 0, 255);
   if (res.material.getColor() == backWallColor) {
     if (ncolor == Vec3<float>(0, 0, 0)) {
       HitResult res;
       shoot(nray, res);
       auto point = nray.getPointAt(res.distance);

       std::cout << "reflect ray origin " << nray.getOrigin().x << '\t'
                 << nray.getOrigin().y << '\t' << nray.getOrigin().z << '\n'
                 << "reflect ray direction " << nray.getDirection().x << '\t'
                 << nray.getDirection().y << '\t' << nray.getDirection().z
                 << '\n'
                 << (res.isHit ? "Yes" : "No") << '\n'
                 << "t: " << res.distance << '\n'
                 << "hit point: " << res.hitPoint.x << ' ' << res.hitPoint.y
                 << ' ' << res.hitPoint.z << '\n'
                 << "normal: " << res.normal.x << ' ' << res.normal.y << ' '
                 << res.normal.z << '\n'
                 << "hit point by count: " << point.x << ' ' << point.y << ' '
                 << point.z << '\n';
     } else {
       std::cout << "hit back wall with reflect color " << ncolor.x << '\t'
                 << ncolor.y << '\t' << ncolor.z << '\n';
     }
   }
 */
  Vec3<float> point(0, 0, 0);
  Vec3<float> normal(0, 0, 1);
  Vec3<float> idirection(0, 0.5, 0.5)

      Ray ray = randomReflectRay(point, idirection, normal);
  auto origin = ray.getOrigin();
  auto direction = ray.getDirection();

  std::cout << "ray origin: " << origin.x << ' ' << origin.y << ' ' << origin.z
            << '\n'
            << "ray direction: " << direction.x << ' ' << direction.y << ' '
            << direction.z << '\n';
  std::cout << std::endl;
}