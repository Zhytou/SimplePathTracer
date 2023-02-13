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
  Camera() = default;
  ~Camera() = default;

  // print.
  void printStatus() const;

  // getter.
  Ray getRay(const int& row, const int& col) const;
  int getWidth() const;
  int getHeight() const;
  Vec3<float> getEye() const;

  // setter.
  void setWidth(const int& w);
  void setHeight(const int& h);
  void setFovy(const float& theta);
  void setPosition(const float& x, const float& y, const float& z);
  void setTarget(const float& x, const float& y, const float& z);
  void setWorld(const float& x, const float& y, const float& z);

 private:
  void update();
};

}  // namespace sre

#endif