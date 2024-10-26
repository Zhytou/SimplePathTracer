#include "../include/Texture.hpp"

#include <cassert>
#include <iostream>

namespace spt {
std::unordered_map<std::string, Texture*> Texture::textures;

Texture* Texture::getInstance(const std::string& texName) {
  if (textures.find(texName) == textures.end()) {
    Texture* ntex = new Texture();
    ntex->img = cv::imread(texName, cv::IMREAD_COLOR);
    textures[texName] = ntex;
  }
  return textures[texName];
}
void Texture::realeaseAllInstances() {
  for (auto itr = textures.begin(); itr != textures.end(); itr++) {
    delete itr->second;
    itr->second = nullptr;
  }
}
Vec3<float> Texture::getColorAt(const Vec2<float>& pos) {
  assert(pos.u >= 0 && pos.u <= 1 && pos.v >= 0 && pos.v <= 1);
  Vec3<float> color(0, 0, 0);
  int rows = img.rows - 1, cols = img.cols - 1;
  switch (img.depth()) {
    case CV_8U:
      color.x = img.at<cv::Vec3b>(pos.u * rows, pos.v * cols)[2] / 255.0f;
      color.y = img.at<cv::Vec3b>(pos.u * rows, pos.v * cols)[1] / 255.0f;
      color.z = img.at<cv::Vec3b>(pos.u * rows, pos.v * cols)[0] / 255.0f;
      break;
    default:
      color.x = img.at<cv::Vec3f>(pos.u * rows, pos.v * cols)[2];
      color.y = img.at<cv::Vec3f>(pos.u * rows, pos.v * cols)[1];
      color.z = img.at<cv::Vec3f>(pos.u * rows, pos.v * cols)[0];
      break;
  }

  return color;
}

}  // namespace spt