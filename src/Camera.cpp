#include "../include/Camera.hpp"

namespace sre {

Ray Camera::getRay(const int& row, const int& col) const {
  // 注意：像素（0，0）位置是左上角
  float x = (static_cast<float>(col) + randFloat(1)) /
            static_cast<float>(width) * actualWidth;
  float y = (static_cast<float>(height - row) + randFloat(1)) /
            static_cast<float>(height) * actualHeight;
  Vec3<float> pos = axisX * x + axisY * y + lowerLeftCorner;
  return Ray(eye, pos - eye);
}
int Camera::getWidth() const { return width; }
int Camera::getHeight() const { return height; }
Vec3<float> Camera::getEye() const { return eye; }

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
void Camera::setPosition(const float& x, const float& y, const float& z) {
  eye.x = x;
  eye.y = y;
  eye.z = z;
  update();
}
void Camera::setTarget(const float& x, const float& y, const float& z) {
  lookat.x = x;
  lookat.y = y;
  lookat.z = z;
  update();
}
void Camera::setWorld(const float& x, const float& y, const float& z) {
  up.x = x;
  up.y = y;
  up.z = z;
  update();
}

void Camera::update() {
  // camera coordinate system
  axisZ = eye - lookat;
  axisZ.normalize();
  axisX = Vec3<float>::cross(up, axisZ);
  axisX.normalize();
  axisY = Vec3<float>::cross(axisZ, axisX);
  axisY.normalize();

  // view port
  actualDepth = Vec3<float>::distance(eye, lookat);
  actualWidth =
      fabs(static_cast<float>(tan(PI * fovy / 180 * 0.5)) * actualDepth) * 2;
  actualHeight =
      static_cast<float>(height) / static_cast<float>(width) * actualWidth;
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
}  // namespace sre
