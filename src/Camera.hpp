#ifndef SRE_CAMERA_HPP
#define SRE_CAMERA_HPP

#include "Ray.hpp"
#include "Vec.hpp"

class Camera {
 private:
  int width, height;
  double fovy;
  Vec3<float> eye;
  Vec3<float> lookat;
  Vec3<float> up;

 public:
  Camera() {}

  // Getter.
  Ray getRay(const float& s, const float& t) const {
    return Ray(Vec3<float>(0, 0, 0), Vec3<float>(0, 0, 0));
  }
  int getWidth() const { return width; }
  int getHeight() const { return height; }

  // Setter.
  void setWidth(const int& w) { width = w; }
  void setHeight(const int& h) { height = h; }
  void setPosition(const float& x, const float& y, const float& z) {
    eye.x = x;
    eye.y = y;
    eye.z = z;
  }
  void setTarget(const float& x, const float& y, const float& z) {
    lookat.x = x;
    lookat.y = y;
    lookat.z = z;
  }
};

#endif