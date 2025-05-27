#ifndef SRE_TEXTURE_HPP
#define SRE_TEXTURE_HPP

#include <string>
#include <unordered_map>

#include "Vec.hpp"

namespace spt {
class Texture {
 private:
  int height, width, channels;
  uint8_t* img;
  static std::unordered_map<std::string, Texture*> textures;

  Texture() : img(nullptr) {}
  Texture(const std::string& imgName);
  ~Texture();

 public:
  static Texture* getInstance(const std::string& texName);

  // getter.
  Vec3<float> getColorAt(const Vec2<float>& pos);
};
}  // namespace spt

#endif