#include "../include/Texture.hpp"

#include <iostream>

namespace sre {
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
  for (auto& [name, tex] : textures) {
    delete tex;
    tex = nullptr;
  }
}
Vec3<float> Texture::getColorAt(const Vec2<float>& pos) {
  Vec3<float> color(0, 0, 0);
  int rows = img.rows - 1, cols = img.cols - 1;
  color.x = img.at<cv::Vec3f>(pos.u * rows, pos.v * cols)[2];
  color.y = img.at<cv::Vec3f>(pos.u * rows, pos.v * cols)[1];
  color.z = img.at<cv::Vec3f>(pos.u * rows, pos.v * cols)[0];
  return color;
}

}  // namespace sre