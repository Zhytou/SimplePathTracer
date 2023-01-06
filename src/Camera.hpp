#ifndef SRE_CAMERA_HPP
#define SRE_CAMERA_HPP

#include <cmath>
#include <iostream>
#define PI 3.1415926

#include "Ray.hpp"
#include "Vec.hpp"

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

  // Print
  void printStatus() {
    std::cout << "camera" << '\n';
    std::cout << "h&w: " << height << '\t' << width << '\n';
    std::cout << "actual h&w: " << actualHeight << '\t' << actualWidth << '\n';
    std::cout << "position: " << eye.x << '\t' << eye.y << '\t' << eye.z
              << '\n';
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

  // Getter.
  Ray getRay(const int& row, const int& col) const {
    // 注意：像素（0，0）位置是左上角
    float x = (static_cast<float>(col) + randFloat(1)) /
              static_cast<float>(width) * actualWidth;
    float y = (static_cast<float>(height - row) + randFloat(1)) /
              static_cast<float>(height) * actualHeight;
    Vec3<float> pos = axisX * x + axisY * y + lowerLeftCorner;
    return Ray(eye, pos - eye);
  }
  int getWidth() const { return width; }
  int getHeight() const { return height; }

  // Setter.
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

  //
  void update() {
    // camera coordinate system
    axisZ = eye - lookat;
    axisZ.normalize();
    axisX = cross(up, axisZ);
    axisX.normalize();
    axisY = cross(axisZ, axisX);
    axisY.normalize();

    // view port
    actualDepth = distance(eye, lookat);
    actualWidth =
        fabs(static_cast<float>(tan(PI * fovy / 180 * 0.5)) * actualDepth) * 2;
    actualHeight =
        static_cast<float>(height) / static_cast<float>(width) * actualWidth;
    lowerLeftCorner =
        lookat - axisX * actualWidth / 2 - axisY * actualHeight / 2;
  }
};

#endif