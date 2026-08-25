#ifndef SPT_MATERIAL_HPP
#define SPT_MATERIAL_HPP

#include <tiny_obj_loader.h>

#include "Image.hpp"
#include "Sampler.hpp"
#include "Utils.hpp"

namespace spt {

enum TransportMode {
    RADIANCE,  // camera path(with shading normal correction and eta scale factor)
    IMPORTANCE // light path(without shading normal correction and eta scale factor)
};

class Material {
   public:
    Material(int id, const std::string& name) : m_id(id), m_name(name), m_sampler(WrapMode::SAMPLER_WRAP_REPEAT, FilterMode::SAMPLER_FILTER_NEAREST) {}
    virtual ~Material() {}

    virtual bool isDelta() const { return false; }
    int getID() const { return m_id; }
    std::string getName() const { return m_name; }
    virtual const char* getTypeName() const = 0;
    void setID(int id) { m_id = id; }
    void setName(const std::string& name) { m_name = name; }

    /**
     * @brief Samples an outgoing direction according to the material's BSDF.
     * 
     * @param wi Local input ray direction (pointing AWAY from the surface). 
     * @param wo Local output ray direction (pointing AWAY from the surface).
     * @param uv Texture coordinates.
     * @param m Transport mode.
     * 
     * @return The Monte Carlo throughput (weight) of the sampled direction. 
     *         For non-delta materials, this equals (bsdf * cos / pdf). 
     *         For delta materials, this equals bsdf, namely 1.0 for ideal reflection or 1 / (eta^2) for ideal refraction.
     * 
     * @note For delta materials (e.g., ideal specular reflection and refraction), the 
     *       cos(theta) term in the rendering equation is cancelled out. Mathematically, 
     *       a Dirac delta BSDF is defined with a 1 / cos(theta) factor, i.e.,
     *       f_r = F * delta(wo - wr) / cos(theta), to ensure that integrating over the 
     *       hemisphere yields the total energy F. 
     *       This 1 / cos(theta) factor directly cancels the geometric cos(theta) term in 
     *       the rendering equation integrand. Consequently, when importance sampled, 
     *       the Fresnel factor F cancels with the sampling probability P = F, leaving 
     *       a net throughput free of both F and cos(theta).
     */
    virtual Vec3<float> sample(const Vec3<float>& wi, Vec3<float>& wo, const Vec2<float>& uv, TransportMode m = RADIANCE) const = 0;

    /**
     * @brief Evaluates the BSDF value for given direction and point.
     * 
     * @param wi Local input ray direction (pointing AWAY from the surface). 
     * @param wo Local output ray direction (pointing AWAY from the surface).
     * @param uv Texture coordinates.
     * @param m Transport mode.
     * 
     * @return The computed BSDF value.
     */
    virtual Vec3<float> eval(const Vec3<float>& wi, const Vec3<float>& wo, const Vec2<float>& uv, TransportMode m = RADIANCE) const = 0;

    /**
     * @brief Computes the probability density function (PDF) for the given direction and point.
     * 
     * @param wi Local input ray direction (pointing AWAY from the surface). 
     * @param wo Local output ray direction (pointing AWAY from the surface).
     * @param uv Texture coordinates.
     * 
     * @return The computed PDF value.
     */
    virtual float pdf(const Vec3<float>& wi, const Vec3<float>& wo, const Vec2<float>& uv) const = 0;

   protected:
    int m_id = -1;
    std::string m_name;
    Sampler m_sampler;
};

class Diffuse : public Material {
   public:
    Diffuse(int id, const std::string& name, const Vec3<float>& albedo, const std::shared_ptr<Image<float>>& albedo_map = nullptr) : Material(id, name), m_albedo(albedo), m_albedo_map(albedo_map) {}

    virtual const char* getTypeName() const override { return "Diffuse"; }
    Vec3<float> getAlbedo(const Vec2<float>& uv) const { return m_albedo_map ? m_sampler.sample<float, 3>(m_albedo_map, uv) : m_albedo; }
    void setAlbedo(const Vec3<float>& albedo) { m_albedo = albedo; }
    void setAlbedoMap(const std::shared_ptr<Image<float>>& albedo_map) { m_albedo_map = albedo_map; }

    virtual Vec3<float> sample(const Vec3<float>& wi, Vec3<float>& wo, const Vec2<float>& uv, TransportMode m = RADIANCE) const override;
    virtual Vec3<float> eval(const Vec3<float>& wi, const Vec3<float>& wo, const Vec2<float>& uv, TransportMode m = RADIANCE) const override;
    virtual float pdf(const Vec3<float>& wi, const Vec3<float>& wo, const Vec2<float>& uv) const override;

   private:
    Vec3<float> m_albedo = Vec3<float>(0.f);
    std::shared_ptr<Image<float>> m_albedo_map;
};

class Mirror : public Material {
   public:
    Mirror(int id, const std::string& name) : Material(id, name) {}

    virtual bool isDelta() const override { return true; }
    virtual const char* getTypeName() const override { return "Mirror"; }

    virtual Vec3<float> sample(const Vec3<float>& wi, Vec3<float>& wo, const Vec2<float>& uv, TransportMode m = RADIANCE) const override;
    virtual Vec3<float> eval(const Vec3<float>& wi, const Vec3<float>& wo, const Vec2<float>& uv, TransportMode m = RADIANCE) const override;
    virtual float pdf(const Vec3<float>& wi, const Vec3<float>& wo, const Vec2<float>& uv) const override;
};

class Dielectric : public Material {
   public:
    Dielectric(int id, const std::string& name, float int_ior, float ext_ior) : Material(id, name), m_int_ior(int_ior), m_ext_ior(ext_ior) {}

    virtual bool isDelta() const override { return true; }
    virtual const char* getTypeName() const override { return "Dielectric"; }

    virtual Vec3<float> sample(const Vec3<float>& wi, Vec3<float>& wo, const Vec2<float>& uv, TransportMode m = RADIANCE) const override;
    virtual Vec3<float> eval(const Vec3<float>& wi, const Vec3<float>& wo, const Vec2<float>& uv, TransportMode m = RADIANCE) const override;
    virtual float pdf(const Vec3<float>& wi, const Vec3<float>& wo, const Vec2<float>& uv) const override;

   private:
    float m_int_ior = 1.f;
    float m_ext_ior = 1.f;
};

class MicrofacetMaterial : public Material {
   public:
    MicrofacetMaterial(int id, const std::string& name, float roughness, std::shared_ptr<Image<float>> roughness_map = nullptr) : Material(id, name), m_roughness(roughness), m_roughness_map(roughness_map) {}

    virtual bool isDelta() const override { return false; }

    float getRoughness(const Vec2<float>& uv) const { return m_roughness_map ? m_sampler.sample<float, 1>(m_roughness_map, uv)[0] : m_roughness; }
    void setRoughness(float roughness) { m_roughness = roughness; }
    void setRoughnessMap(std::shared_ptr<Image<float>> roughness_map) { m_roughness_map = roughness_map; }

   protected:
    float m_roughness = 0.f;
    std::shared_ptr<Image<float>> m_roughness_map;
};

class MicrofacetConductor : public MicrofacetMaterial {
   public:
    MicrofacetConductor(int id, const std::string& name, float real_ior, float imag_ior, float roughness, std::shared_ptr<Image<float>> roughness_map = nullptr) : MicrofacetMaterial(id, name, roughness, roughness_map), m_real_ior(real_ior), m_imag_ior(imag_ior) {}

    virtual const char* getTypeName() const override { return "MicrofacetConductor"; }

    virtual Vec3<float> sample(const Vec3<float>& wi, Vec3<float>& wo, const Vec2<float>& uv, TransportMode m = RADIANCE) const override;
    virtual Vec3<float> eval(const Vec3<float>& wi, const Vec3<float>& wo, const Vec2<float>& uv, TransportMode m = RADIANCE) const override;
    virtual float pdf(const Vec3<float>& wi, const Vec3<float>& wo, const Vec2<float>& uv) const override;

   private:
    float m_real_ior; // index of refraction eta
    float m_imag_ior; // extinction index k
};

class MicrofacetDielectric : public MicrofacetMaterial {
   public:
    MicrofacetDielectric(int id, const std::string& name, float int_ior, float ext_ior, float roughness, std::shared_ptr<Image<float>> roughness_map = nullptr) : MicrofacetMaterial(id, name, roughness, roughness_map), m_int_ior(int_ior), m_ext_ior(ext_ior) {}

    virtual const char* getTypeName() const override { return "MicrofacetDielectric"; }

    virtual Vec3<float> sample(const Vec3<float>& wi, Vec3<float>& wo, const Vec2<float>& uv, TransportMode m = RADIANCE) const override;
    virtual Vec3<float> eval(const Vec3<float>& wi, const Vec3<float>& wo, const Vec2<float>& uv, TransportMode m = RADIANCE) const override;
    virtual float pdf(const Vec3<float>& wi, const Vec3<float>& wo, const Vec2<float>& uv) const override;

   private:
    float m_ext_ior = 1.f;
    float m_int_ior = 1.f;
};

} // namespace spt

#endif
