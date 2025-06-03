#include "Camera.hpp"

namespace spt {

Ray Camera::getRay(const int& row, const int& col) const {
  float x = (static_cast<float>(col) + rand<float>(1)) /
            static_cast<float>(width) * actualWidth;
  float y = (static_cast<float>(height - row) + rand<float>(1)) /
            static_cast<float>(height) * actualHeight;
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
  axisZ = eye - lookat;
  axisZ.normalize();
  axisX = cross(up, axisZ);
  axisX.normalize();
  axisY = cross(axisZ, axisX);
  axisY.normalize();

  // view port
  actualDepth = distance(eye, lookat);
  actualHeight = fabs(static_cast<float>(tan(PI * fovy / 180 * 0.5)) * actualDepth) * 2;
  actualWidth = static_cast<float>(width) / static_cast<float>(height) * actualHeight;
  lowerLeftCorner = lookat - axisX * actualWidth / 2 - axisY * actualHeight / 2;
}

void Camera::printStatus() const {
  std::cout << "camera" << '\n';
  std::cout << "h&w: " << height << '\t' << width << '\n';
  std::cout << "actual h&w: " << actualHeight << '\t' << actualWidth << '\n';
  std::cout << "position: " << eye.x << '\t' << eye.y << '\t' << eye.z << '\n';
  std::cout << "target: " << lookat.x << '\t' << lookat.y << '\t' << lookat.z
            << '\n';
  std::cout << "axis x: " << axisX.x << '\t' << axisX.y << '\t' << axisX.z
            << '\n';
  std::cout << "axis y: " << axisY.x << '\t' << axisY.y << '\t' << axisY.z
            << '\n';
  std::cout << "axis z: " << axisZ.x << '\t' << axisZ.y << '\t' << axisZ.z
            << '\n';
  std::cout << "low left corner: " << lowerLeftCorner.x << '\t'
            << lowerLeftCorner.y << '\t' << lowerLeftCorner.z << '\n';
  std::cout << std::endl;
}
}  // namespace spt
