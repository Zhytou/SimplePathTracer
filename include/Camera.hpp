#ifndef SRE_CAMERA_HPP
#define SRE_CAMERA_HPP

#include <cmath>
#include <iostream>

#include "Ray.hpp"

namespace spt {

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

  // print
  void printStatus() const;

  // getter
  Ray getRay(const int& row, const int& col) const;
  int getWidth() const;
  int getHeight() const;
  Vec3<float> getEye() const;
  Vec3<float> getLookAt() const;
  Vec3<float> getAxisZ() const;

  // setter
  void setWidth(const int& w);
  void setHeight(const int& h);
  void setFovy(const float& theta);
  void setEye(const float& x, const float& y, const float& z);
  void setLookAt(const float& x, const float& y, const float& z);
  void setUp(const float& x, const float& y, const float& z);

 private:
  void update();
};

}  // namespace spt

#endif