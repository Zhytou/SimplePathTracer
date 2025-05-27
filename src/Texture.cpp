#include "Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION 
#include <stb_image.h>
#include <cassert>
#include <iostream>

namespace spt {
std::unordered_map<std::string, Texture*> Texture::textures;

Texture::~Texture() {
  if (img) {
    stbi_image_free(img);
  }
  img = nullptr;
}

Texture::Texture(const std::string& texName) {
  img = stbi_load(texName.c_str(), &width, &height, &channels, 0);
}

Texture* Texture::getInstance(const std::string& texName) {
  if (textures.find(texName) == textures.end()) {
    textures[texName] = new Texture(texName);
  }
  return textures[texName];
}

Vec3<float> Texture::getColorAt(const Vec2<float>& pos) {
  assert(pos.u >= 0 && pos.u <= 1 && pos.v >= 0 && pos.v <= 1);

  Vec3<float> color(0, 0, 0);
  int row = pos.u*height, col = pos.v*width;
  int idx = (row*width+col)*3;
  color.x = img[idx+0]/255.0f;
  color.y = img[idx+1]/255.0f;
  color.z = img[idx+2]/255.0f;

  return color;
}

}  // namespace spt