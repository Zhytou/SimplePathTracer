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
  // type of scatter
  BSDF_REFLECTION = 1,
  BSDF_TRANSIMISSION = 1 << 1,

  // type of surface
  BSDF_DIFFUSE = 1 << 2,
  BSDF_GLOSSY = 1 << 3,
  BSDF_SPECULAR = 1 << 4,
};

class Material {
  std::string name;
  bool emissive;
  Vec3<float> emission;
  Vec3<float> albedo;
  float metallic;   // (0 = dielectric, 1 = metal)
  float roughness;  // (0 = perfectly smooth, 1 = fully rough)
  float transparency; // (0 = opaque, 1 = fully transparent)
  float ior;
  uint type;
  Texture* tex;
  static uint scatMask; // bxdf scatter type mask
  static uint surfMask; // bxdf surface type mask

  static float GGX_D(const Vec3<float>& H, const Vec3<float>& N, float roughness);
  static Vec3<float> Fresnel_Schlick(float cosTheta, Vec3<float> F0);
  static float GeometrySchlickGGX(float cosTheta, float roughness);
  static float Smith_G(float NdotV, float NdotL, float roughness);

  Vec3<float> brdf(const Vec3<float> &V, const Vec3<float> &N, const Vec3<float> &L, const Vec3<float>& H, const Vec2<float>& UV) const;
  Vec3<float> btdf(const Vec3<float> &V, const Vec3<float> &N, const Vec3<float> &L, const Vec3<float>& H, const Vec2<float>& UV) const;

  std::pair<Vec3<float>, float> reflect(const Vec3<float> &V, const Vec3<float> &N) const;
  std::pair<Vec3<float>, float> transmit(const Vec3<float> &V, const Vec3<float> &N) const;
public:
  Material() = default;

  ~Material() = default;
  
  Material(const tinyobj::material_t& mtl, const std::string& dir);

  bool isEmissive() const;
  
  void setEmission(Vec3<float> e);

  std::string getName() const;
  uint getType() const;
  Vec3<float> getEmission() const;
  Vec3<float> getAlbedo(Vec2<float> uv) const;

  // evaluate BxDF
  Vec3<float> eval(const Vec3<float> &wi, const Vec3<float> &n, const Vec3<float> &wo, const Vec2<float>& uv) const;

  // sample direction and corresponding pdf
  std::pair<Vec3<float>, float> sample(const Vec3<float> &wi, const Vec3<float> &n) const;
};
}  // namespace spt

#endif
