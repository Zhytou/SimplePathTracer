#include "../include/Texture.hpp"

namespace sre {
std::unordered_map<std::string, Texture*> Texture::textures;

Texture* Texture::getInstance(const std::string& name) {
  if (textures.find(name) == textures.end()) {
    Texture* ntex = new Texture();
    ntex->img = cv::imread(name, cv::IMREAD_COLOR);
  }
  return textures[name];
}

Vec3<float> Texture::getColorAt(const Vec2<float>& pos) {
  return Vec3<float>(0, 0, 0);
}

}  // namespace sre