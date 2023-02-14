#ifndef SRE_MATERIAL_HPP
#define SRE_MATERIAL_HPP

#include <string>
#include <unordered_map>

#include "Ray.hpp"
#include "Texture.hpp"
#include "Vec.hpp"

namespace sre {

class Material {
 private:
  std::string name;           // name 材料名称
  Vec3<float> emission;       // Ke 自射光 对应xml文件中radiance
  Vec3<float> ambience;       // Ka 环境光
  Vec3<float> diffusion;      // Kd 漫射光
  Vec3<float> specularity;    // Ks 镜面光
  Vec3<float> transmittance;  // Tr 透射光
  float shiness;              // Ns 反光强度
  float refraction;           // Ni 折射率
  bool emisssive;             // 是否发光
  Texture* ambientTexture;    // map_Ka 纹理
  Texture* diffuseTexture;    // map_Kd 纹理
  Texture* specularTexture;   // map_Ks 纹理

 public:
  Material();
  ~Material();

  // getter.
  std::string getName() const;
  Vec3<float> getEmission() const;
  Vec3<float> getAmbience(const Vec2<float>& texCoord) const;
  Vec3<float> getDiffusion(const Vec2<float>& texCoord) const;
  Vec3<float> getSpecularity(const Vec2<float>& texCoord) const;
  Vec3<float> getTransmittance(const Vec2<float>& texCoord) const;
  float getShiness() const;
  float getRefraction() const;
  bool isEmissive() const;
  bool isDiffusive() const;
  bool isSpecular() const;
  bool isTransmissive() const;

  // setter.
  void setName(const std::string& n);
  void setEmissive(bool e);
  void setEmission(float x, float y, float z);
  void setAmbience(float x, float y, float z);
  void setDiffusion(float x, float y, float z);
  void setSpecularity(float x, float y, float z);
  void setTransmittance(float x, float y, float z);
  void setShiness(float s);
  void setRefraction(float r);
  void setAmbientTexture(const std::string& at);
  void setDiffuseTexture(const std::string& dt);
  void setSpecularTexture(const std::string& st);

  // print.
  void printStatus() const;
};
}  // namespace sre

#endif
