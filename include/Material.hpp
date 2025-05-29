#ifndef SRE_MATERIAL_HPP
#define SRE_MATERIAL_HPP

#include <string>

#include "Texture.hpp"
#include "Vec.hpp"

#define EPSILON 1e-6f

namespace tinyobj {
  struct material_t;
}

namespace spt {

enum BxDFType {
  BSDF_DIFFUSE_R = 1 << 1,
  BSDF_GLOSSY_R = 1 << 2,
  BSDF_SPECULAR_R = 1 << 3,
  // BSDF_DIFFUSE_T = 1 << 4,
  BSDF_GLOSSY_T = 1 << 5,
  BSDF_SPECULAR_T = 1 << 6,
};

class Material {
  std::string name;
  bool emissive;
  Vec3<float> emission;
  Vec3<float> albedo;
  float metallic;
  float roughness;
  float ior;
  Texture* tex;

  static float GGX_D(const Vec3<float>& wh, const Vec3<float>& n, float roughness);
  static Vec3<float> Fresnel_Schlick(float cosTheta, Vec3<float> F0);
  static float GeometrySchlickGGX(float NdotV, float roughness);
  static float Smith_G(const Vec3<float>& wi, const Vec3<float>& wo, const Vec3<float>& n, float roughness);
public:
  Material() = default;

  ~Material() = default;
  
  Material(const tinyobj::material_t& mtl, const std::string& dir);

  bool isEmissive() const;
  
  void setEmission(Vec3<float> e);

  std::string getName() const;
  Vec3<float> getEmission() const;
  Vec3<float> getAlbedo(Vec2<float> uv) const;

  // evaluate BxDF
  Vec3<float> eval(const Vec3<float> &wi, const Vec3<float> &n, const Vec3<float> &wo, const Vec2<float>& uv, BxDFType type) const;

  // sample direction and corresponding pdf
  std::pair<Vec3<float>, float> sample(const Vec3<float> &wi, const Vec3<float> &n, BxDFType type) const;
};
}  // namespace spt

#endif
