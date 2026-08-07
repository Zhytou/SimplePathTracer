#ifndef SPT_DISTRIBUTION_HPP
#define SPT_DISTRIBUTION_HPP

#include "Utils.hpp"

namespace spt {

class Distribution {
   public:
    Distribution() {}
    virtual ~Distribution() {}

    virtual Vec3<float> sample() const                   = 0;
    virtual float pdf(const Vec3<float>& wo_local) const = 0;
};

class DeltaDistribution : public Distribution {
   public:
    DeltaDistribution() {}

    virtual Vec3<float> sample() const override {
        return {0.f, 0.f, 1.f};
    }
    virtual float pdf(const Vec3<float>& wo_local) const override {
        return INFINITY;
    }
};

class CosineDistribution : public Distribution {
   public:
    CosineDistribution() {}

    virtual Vec3<float> sample() const override {
        float a = rand(0.0f, 1.f);
        float b = rand(0.0f, 1.f);

        float cos_theta = std::sqrt(a);
        float sin_theta = std::sqrt(1.f - cos_theta * cos_theta);
        float phi       = 2 * PI * b;

        return {cosf(phi) * sin_theta, sinf(phi) * sin_theta, cos_theta};
    }
    virtual float pdf(const Vec3<float>& wo_local) const override {
        return std::max(0.f, wo_local.z) / PI;
    }
};

class MicrofacetDistribution : public Distribution {
   public:
    MicrofacetDistribution() {}

    virtual Vec3<float> sample() const                  = 0;
    virtual float pdf(const Vec3<float>& h_local) const = 0;

    virtual float D(const Vec3<float>& h_local) const                               = 0;
    virtual float G1(const Vec3<float>& w_local) const                              = 0;
    virtual float G(const Vec3<float>& wo_local, const Vec3<float>& wi_local) const = 0;
};

class GGXDistribution : public MicrofacetDistribution {
   public:
    GGXDistribution(float roughness) : m_roughness(roughness) {}

    virtual Vec3<float> sample() const override {
        float a = rand(0.0f, 1.f);
        float b = rand(0.0f, 1.f);

        float alpha  = m_roughness * m_roughness;
        float alpha2 = alpha * alpha;

        float cos_theta = std::sqrt((1.f - a) / (1.f + (alpha2 - 1.f) * a));
        float sin_theta = std::sqrt(1.f - cos_theta * cos_theta);
        float phi       = 2 * PI * b;

        return {cosf(phi) * sin_theta, sinf(phi) * sin_theta, cos_theta};
    }

    virtual float pdf(const Vec3<float>& h_local) const override {
        return D(h_local) * h_local.z;
    }

    virtual float D(const Vec3<float>& h_local) const override {
        float alpha  = m_roughness * m_roughness;
        float alpha2 = alpha * alpha;

        float num   = alpha2;
        float denom = (h_local.z * h_local.z) * (alpha2 - 1.0f) + 1.0f;
        denom       = PI * denom * denom;

        return num / denom;
    }

    virtual float G1(const Vec3<float>& w_local) const override {
        float r = (m_roughness + 1.0f);
        float k = (r * r) / 8.0f;

        float num   = w_local.z;
        float denom = w_local.z * (1.0f - k) + k;

        return num / denom;
    }

    float G(const Vec3<float>& wi_local, const Vec3<float>& wo_local) const override {
        return G1(wi_local) * G1(wo_local);
    }

   private:
    float m_roughness;
};

} // namespace spt

#endif