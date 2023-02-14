#ifndef SRE_TEXTURE_HPP
#define SRE_TEXTURE_HPP

#include <opencv2/opencv.hpp>
#include <string>
#include <unordered_map>

#include "Vec.hpp"

namespace sre {
class Texture {
 private:
  cv::Mat img;
  static std::unordered_map<std::string, Texture*> textures;

 private:
  Texture() = default;
  ~Texture() = default;

 public:
  static Texture* getInstance(const std::string& texName);
  static void realeaseAllInstances();

  // getter.
  Vec3<float> getColorAt(const Vec2<float>& pos);
};
}  // namespace sre

#endif