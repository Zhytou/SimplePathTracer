#ifndef SRE_CAMERA_HPP
#define SRE_CAMERA_HPP

#include <cmath>
#include <iostream>
#define PI 3.1415926

#include "Ray.hpp"
#include "Vec.hpp"

namespace sre {

class Camera {
 private:
  int width, height;
  float actualDepth, actualWidth, actualHeight;
  double fovy;
  Vec3<float> eye;
  Vec3<float> lookat;
  Vec3<float> up;

  Vec3<float> axisX, axisY, axisZ;
  Vec3<float> lowerLeftCorner;

 public:
  Camera() {}
  ~Camera() {}

  // print.
  void printStatus() const;

  // getter.
  Ray getRay(const int& row, const int& col) const;
  int getWidth() const { return width; }
  int getHeight() const { return height; }
  Vec3<float> getEye() const { return eye; }

  // setter.
  void setWidth(const int& w) {
    width = w;
    update();
  }
  void setHeight(const int& h) {
    height = h;
    update();
  }
  void setFovy(const float& theta) {
    fovy = theta;
    update();
  }
  void setPosition(const float& x, const float& y, const float& z) {
    eye.x = x;
    eye.y = y;
    eye.z = z;
    update();
  }
  void setTarget(const float& x, const float& y, const float& z) {
    lookat.x = x;
    lookat.y = y;
    lookat.z = z;
    update();
  }
  void setWorld(const float& x, const float& y, const float& z) {
    up.x = x;
    up.y = y;
    up.z = z;
    update();
  }

 private:
  void update();
};

}  // namespace sre

#endif