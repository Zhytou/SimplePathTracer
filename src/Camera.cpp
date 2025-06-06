#include "Camera.hpp"

namespace spt {

Ray Camera::getRay(const int& row, const int& col) const {
  float y = tanf(PI * fovy / 180 * 0.5f) * focus * 2;
  float x = 1.f * width / height * y;

  y *= (height - row + rand(0.999f)) / height;
  x *= (col + rand(0.999f)) / width;

  Vec3<float> pos = axisX * x + axisY * y + lowerLeftCorner;
  return Ray(eye, pos - eye);
}
int Camera::getWidth() const { return width; }
int Camera::getHeight() const { return height; }
Vec3<float> Camera::getEye() const { return eye; }
Vec3<float> Camera::getLookAt() const { return lookat; }
Vec3<float> Camera::getAxisZ() const { return axisZ; }

// setter.
void Camera::setWidth(const int& w) {
  width = w;
  update();
}
void Camera::setHeight(const int& h) {
  height = h;
  update();
}
void Camera::setFovy(const float& theta) {
  fovy = theta;
  update();
}
void Camera::setEye(const float& x, const float& y, const float& z) {
  eye.x = x;
  eye.y = y;
  eye.z = z;
  update();
}
void Camera::setLookAt(const float& x, const float& y, const float& z) {
  lookat.x = x;
  lookat.y = y;
  lookat.z = z;
  update();
}
void Camera::setUp(const float& x, const float& y, const float& z) {
  up.x = x;
  up.y = y;
  up.z = z;
  update();
}

void Camera::update() {
  // camera coordinate system
  axisZ = normalize(eye - lookat);
  axisX = normalize(cross(up, axisZ));
  axisY = normalize(cross(axisZ, axisX));

  // view port
  focus = distance(eye, lookat);
  float y = tanf(PI * fovy / 180 * 0.5f) * focus; // half height
  float x = 1.f * width / height * y;
  lowerLeftCorner = lookat - axisX * x - axisY * y;
}

}  // namespace spt
