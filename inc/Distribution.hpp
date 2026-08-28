#ifndef SPT_DISTRIBUTION_HPP
#define SPT_DISTRIBUTION_HPP

#include "Utils.hpp"

namespace spt {

template <typename T>
class Distribution {
   public:
    Distribution() {}
    virtual ~Distribution() {}

    virtual T sample() const          = 0;
    virtual float pdf(const T&) const = 0;
};

template <arithmetic T>
class UniformDistribution1D : public Distribution<T> {
   public:
    UniformDistribution1D(T min, T max) : m_min(min), m_max(max) {}

    virtual T sample() const override {
        return rand(m_min, m_max);
    }
    virtual float pdf(const T& t) const override {
        return 1.f / (m_max - m_min);
    }

   private:
    T m_min = 0;
    T m_max = 1;
};

class PrefixDiscreteDistribution1D : public Distribution<int> {
   public:
    PrefixDiscreteDistribution1D(const std::vector<float>& weights) {
        int sum_w = 0;
        for (auto w : weights) {
            sum_w += w;
            m_prefix_weights.push_back(sum_w);
        }
    }

    virtual int sample() const override {
        int r = rand(0.f, m_prefix_weights.back());
        return std::lower_bound(m_prefix_weights.begin(), m_prefix_weights.end(), r) - m_prefix_weights.begin();
    }
    virtual float pdf(const int& t) const override {
        return m_prefix_weights[t] / m_prefix_weights.back();
    }

   private:
    std::vector<float> m_prefix_weights;
};

class DiskDistribution : public Distribution<Vec2f> {
   public:
    DiskDistribution() {}
};

class CosineDistribution : public Distribution<Vec3f> {
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

class MicrofacetDistribution : public Distribution<Vec3f> {
   public:
    MicrofacetDistribution() {}

    virtual Vec3<float> sample() const = 0;
    // TODO: implemnt visible normal distribution function
    // virtual Vec3<float> sample(const Vec3<float>& wi_local) const = 0;
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
        // float r = (m_roughness + 1.0f);
        // float k = (r * r) / 8.0f;
        float r = m_roughness;
        float k = r * r / 2.0f;
        float z = std::abs(w_local.z);

        float num   = z;
        float denom = z * (1.0f - k) + k;

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