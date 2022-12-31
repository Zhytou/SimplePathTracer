#ifndef SRE_MATERIAL_HPP
#define SRE_MATERIAL_HPP

#include "Vec.hpp"

struct Material {
 private:
  Vec3<float> Kd;  // 散射光
  Vec3<float> Ks;  // 镜面光
  Vec3<float> Tr;  //
  double Ni;       // 折射
  double Ns;       // 反射
 public:
  Material() {}
};

#endif
