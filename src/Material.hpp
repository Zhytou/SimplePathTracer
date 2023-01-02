#ifndef SRE_MATERIAL_HPP
#define SRE_MATERIAL_HPP

#include <string>

#include "Vec.hpp"

struct Material {
 private:
  Vec3<float> color;
  Vec3<float> emission;       // Ke 自射光
  Vec3<float> ambient;        // Ka 环境
  Vec3<float> diffuse;        // Kd 漫射光
  Vec3<float> specular;       // Ks 镜面光
  Vec3<float> transmittance;  // Tr
  double shiness;             // Ns
  double refraction;          // Ni 折射
  bool isEmisssive;

  std::string ambientTexure;   // map_Ka
  std::string diffuseTexure;   // map_Kd
  std::string specularTexure;  // map_Ks

 public:
  Material() = default;
  ~Material() = default;

  // Getter
  bool emissive() { return isEmisssive; }
  Vec3<float> getColor() const { return color; }
  // Vec3<float> getEmission() const { return emission; }
  Vec3<float> getAmbient() const { return ambient; }
  Vec3<float> getDiffuse() const { return diffuse; }
  Vec3<float> getSpecular() const { return specular; }
  Vec3<float> getTransmittance() const { return transmittance; }

  // Setter
  void setEmissive(bool e) { isEmisssive = e; }
  void setColor(const Vec3<float>& c) { color = c; }
  // void setEmission(const Vec3<float>& r) { emission = r; }
  void setAmbient(float x, float y, float z) {
    ambient.x = x;
    ambient.y = y;
    ambient.z = z;
  }
  void setDiffuse(float x, float y, float z) {
    diffuse.x = x;
    diffuse.y = y;
    diffuse.z = z;
  }
  void setSpecular(float x, float y, float z) {
    specular.x = x;
    specular.y = y;
    specular.z = z;
  }
  void setTransmittance(float x, float y, float z) {
    transmittance.x = x;
    transmittance.y = y;
    transmittance.z = z;
  }
};

#endif
