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

    virtual Vec2f sample() const override {
        float a = rand(0.0f, 1.f);
        float b = rand(0.0f, 1.f);

        float radius = std::sqrt(a);
        float phi    = 2 * PI * b;

        return {radius * std::cos(phi), radius * std::sin(phi)};
    }
    virtual float pdf(const Vec2f& p) const override {
        return 1.f / PI;
    }
};

class CosineDistribution : public Distribution<Vec3f> {
   public:
    CosineDistribution() {}

    virtual Vec3f sample() const override {
        float a = rand(0.0f, 1.f);
        float b = rand(0.0f, 1.f);

        float cos_theta = std::sqrt(a);
        float sin_theta = std::sqrt(1.f - cos_theta * cos_theta);
        float phi       = 2 * PI * b;

        return {cosf(phi) * sin_theta, sinf(phi) * sin_theta, cos_theta};
    }
    virtual float pdf(const Vec3f& wo_local) const override {
        return std::max(0.f, wo_local.z) / PI;
    }
};

class MicrofacetDistribution : public Distribution<Vec3f> {
   public:
    MicrofacetDistribution(float roughness) : m_roughness(roughness), m_alpha(roughness * roughness) {}

    virtual Vec3f sample() const                                        = 0;
    virtual float pdf(const Vec3f& h_local) const                       = 0;
    virtual Vec3f sample(const Vec3f& w_local) const                    = 0;
    virtual float pdf(const Vec3f& w_local, const Vec3f& h_local) const = 0;

    virtual float D(const Vec3f& h_local) const                         = 0;
    virtual float G1(const Vec3f& w_local) const                        = 0;
    virtual float G(const Vec3f& wo_local, const Vec3f& wi_local) const = 0;

    float getAlpha() const { return m_alpha; }
    float getRoughness() const { return m_roughness; }
    void setRoughness(float roughness) { m_roughness = roughness; }

   private:
    float m_roughness;
    float m_alpha; // the actual roughness use in sampling
};

class GGXDistribution : public MicrofacetDistribution {
   public:
    GGXDistribution(float roughness) : MicrofacetDistribution(roughness) {}

    virtual Vec3f sample() const override {
        float a = rand(0.0f, 1.f);
        float b = rand(0.0f, 1.f);

        float alpha  = getAlpha();
        float alpha2 = alpha * alpha;

        float cos_theta = std::sqrt((1.f - a) / (1.f + (alpha2 - 1.f) * a));
        float sin_theta = std::sqrt(1.f - cos_theta * cos_theta);
        float phi       = 2 * PI * b;

        return {cosf(phi) * sin_theta, sinf(phi) * sin_theta, cos_theta};
    }

    virtual float pdf(const Vec3f& h_local) const override {
        return D(h_local) * h_local.z;
    }

    virtual Vec3f sample(const Vec3f& w_local) const override {
        // transform to hemisphere coordinate
        float alpha    = getAlpha();
        Vec3f ww_local = {alpha * w_local.x, alpha * w_local.y, w_local.z};
        ww_local       = normalize(ww_local.z < 0 ? -ww_local : ww_local);
        Vec3f ww_e1, ww_e2;
        TBN(ww_local, ww_e1, ww_e2);

        // disk sampling
        float a      = rand(0.0f, 1.f);
        float b      = rand(0.0f, 1.f);
        float radius = std::sqrt(a);
        float phi    = 2 * PI * b;
        float x      = radius * std::cos(phi);
        float y      = radius * std::sin(phi);

        // wrap to disk
        float h = std::sqrt(1.f - x * x);
        float t = (1.f + ww_local.z) / 2.f;
        y       = (1 - t) * h + t * y; // lerp(t, h, y)
        float z = std::sqrt(std::max(0.f, 1.f - x * x - y * y));

        // transform to hemisphere coordinate
        Vec3f hh_local = x * ww_e1 + y * ww_e2 + z * ww_local;
        Vec3f h_local  = {alpha * hh_local.x, alpha * hh_local.y, std::max(hh_local.z, EPS)};

        return normalize(h_local);
    }

    virtual float pdf(const Vec3f& w_local, const Vec3f& h_local) const override {
        return G1(w_local) / std::max(std::abs(w_local.z), PDF_EPS) * D(h_local) * std::abs(dot(w_local, h_local));
    }

    virtual float D(const Vec3f& h_local) const override {
        float alpha  = getAlpha();
        float alpha2 = alpha * alpha;

        float num   = alpha2;
        float denom = (h_local.z * h_local.z) * (alpha2 - 1.0f) + 1.0f;
        denom       = PI * denom * denom;

        return num / denom;
    }

    virtual float G1(const Vec3f& w_local) const override {
        float a = getAlpha();
        float z = std::abs(w_local.z);

        float num, denom;
        if (true) { // smith ggx
            float a2 = a * a;
            float z2 = z * z;

            num   = 2 * z;
            denom = z + std::sqrt(a2 + (1 - a2) * z2);
        } else { // schlick approximation
            // for real-time rendering, k = (roughness + 1.0f)^2 / 8;
            // for offline rendering, k = roughness^2 / 2;
            float k = a / 2;

            num   = z;
            denom = z * (1.0f - k) + k;
        }

        return num / denom;
    }

    float G(const Vec3f& wi_local, const Vec3f& wo_local) const override {
        return G1(wi_local) * G1(wo_local);
    }
};

} // namespace spt

#endif