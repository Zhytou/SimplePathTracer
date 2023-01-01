#ifndef SRE_MATERIAL_HPP
#define SRE_MATERIAL_HPP

#include "Vec.hpp"

struct Material {
 private:
  Vec3<float> radiance;
  Vec3<float> diffuse;        // Kd 散射光
  Vec3<float> specular;       // Ks 镜面光
  Vec3<float> transmittance;  // Tr
  double shiness;             // Ns
  double refraction;          // Ni 折射

 public:
  Material() {}
  ~Material() {}

  // Getter
  bool illuminated() { return true; }

  Vec3<unsigned char> getColor() { return Vec3<unsigned char>(255, 0, 0); }

  // Setter
  void setRadiance(const Vec3<float>& r) { radiance = r; }
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
