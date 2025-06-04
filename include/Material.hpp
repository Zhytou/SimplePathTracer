#ifndef SRE_MATERIAL_HPP
#define SRE_MATERIAL_HPP

#include <string>

#include "Texture.hpp"

#define EPSILON 1e-6f

namespace tinyobj {
  struct material_t;
}

namespace spt {

enum BSDFType {
  // type of scatter
  BSDF_REFLECTION = 1,
  BSDF_TRANSIMISSION = 1 << 1,

  // type of surface
  BSDF_DIFFUSE = 1 << 2,
  BSDF_GLOSSY = 1 << 3,
  BSDF_SPECULAR = 1 << 4,

  // type of illumination model
  BSDF_MICROFACET = 1 << 5,
  BSDF_PHONG = 1 << 6,
  BSDF_BLINN_PHONG = 1 << 7,
};

class Material {
  std::string name;
  
  // light property
  bool emissive;
  Vec3<float> emission;

  // microfacet property
  Vec3<float> diffuse; // Kd
  // m-r workflow
  float metallic;   // Pm (0 = dielectric, 1 = metal)
  float roughness;  // Pr (0 = perfectly smooth, 1 = fully rough)
  // s-g workflow
  Vec3<float> specular; // Ks
  float glossiness; // sheen
  
  // phong/phong-blinn property
  float shininess; // Ns

  // shared property
  Texture* albedo;
  float transparency; // Tr or d (0 = opaque, 1 = fully transparent)
  float ior; // Ni
  
  uint type;
  static uint scatMask; // bsdf scatter type mask
  static uint surfMask; // bsdf surface type mask
  static uint illuMask; // bsdf illumimation model mask

  static float GGX_D(const Vec3<float>& H, const Vec3<float>& N, float roughness);
  static Vec3<float> Fresnel_Schlick(float cosTheta, Vec3<float> F0);
  static float GeometrySchlickGGX(float cosTheta, float roughness);
  static float Smith_G(float NdotV, float NdotL, float roughness);

  Vec3<float> brdf(const Vec3<float> &V, const Vec3<float> &N, const Vec3<float> &L, const Vec3<float>& H, const Vec2<float>& UV) const;
  Vec3<float> btdf(const Vec3<float> &V, const Vec3<float> &N, const Vec3<float> &L, const Vec3<float>& H, const Vec2<float>& UV, float eta) const;

  std::pair<Vec3<float>, float> reflect(const Vec3<float> &V, const Vec3<float> &N) const;
  std::pair<Vec3<float>, float> transmit(const Vec3<float> &V, const Vec3<float> &N) const;

  Vec3<float> sample(const Vec3<float> &V, const Vec3<float> &N, const std::string& mode) const;
public:
  Material() = default;

  ~Material() = default;
  
  Material(const tinyobj::material_t& mtl, const std::string& dir, uint illuType);

  bool isEmissive() const;
  
  void setEmission(Vec3<float> e);

  std::string getName() const;
  uint getType() const;
  Vec3<float> getEmission() const;
  Vec3<float> getBaseColor(Vec2<float> uv) const;

  // evaluate BSDF
  Vec3<float> bsdf(const Vec3<float> &wi, const Vec3<float> &n, const Vec3<float> &wo, const Vec2<float>& uv) const;

  // sample direction and corresponding pdf
  std::pair<Vec3<float>, float> scatter(const Vec3<float> &wi, const Vec3<float> &n) const;
};
}  // namespace spt

#endif
