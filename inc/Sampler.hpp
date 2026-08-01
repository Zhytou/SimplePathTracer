#ifndef SPT_SAMPLER_HPP
#define SPT_SAMPLER_HPP

#include "Image.hpp"
#include "Vec.hpp"

#include <algorithm>
#include <cmath>

namespace spt {

enum class WrapMode {
    SAMPLER_WRAP_REPEAT,
    SAMPLER_WRAP_CLAMP_TO_EDGE,
    SAMPLER_WRAP_MIRROR
};

enum class FilterMode {
    SAMPLER_FILTER_NEAREST,
    SAMPLER_FILTER_LINEAR
};

class Sampler {
   public:
    Sampler(WrapMode wrap, FilterMode filter) : m_wrap(wrap), m_filter(filter) {}

    template <typename T, size_t N>
    Vec<T, N> sample(std::shared_ptr<Image<T>> image, const Vec2<float>& texcoord) const {
        if (!image) { throw std::runtime_error("Sampler::sample: Image is null!"); }

        float u = wrap(texcoord.x, m_wrap);
        float v = wrap(texcoord.y, m_wrap);
        int w   = image->getWidth();
        int h   = image->getHeight();

        switch (m_filter) {
            case FilterMode::SAMPLER_FILTER_NEAREST: {
                int col = std::clamp(static_cast<int>(u * w), 0, w - 1);
                int row = std::clamp(static_cast<int>(v * h), 0, h - 1);
                return image->template getPixel<N>(row, col);
            }
            case FilterMode::SAMPLER_FILTER_LINEAR: {
                float u0 = u * w - 0.5f;
                float v0 = v * h - 0.5f;

                int col0 = std::clamp(static_cast<int>(std::floor(u0)), 0, w - 1);
                int row0 = std::clamp(static_cast<int>(std::floor(v0)), 0, h - 1);
                int col1 = std::clamp(col0 + 1, 0, w - 1);
                int row1 = std::clamp(row0 + 1, 0, h - 1);

                float tx = u0 - std::floor(u0);
                float ty = v0 - std::floor(v0);

                Vec<T, N> p00 = image->template getPixel<N>(row0, col0);
                Vec<T, N> p10 = image->template getPixel<N>(row0, col1);
                Vec<T, N> p01 = image->template getPixel<N>(row1, col0);
                Vec<T, N> p11 = image->template getPixel<N>(row1, col1);
                Vec<T, N> p0  = lerp(p00, p10, tx);
                Vec<T, N> p1  = lerp(p01, p11, tx);

                return lerp(p0, p1, ty);
            }
            default: {
                throw std::runtime_error("Sampler::sample: Invalid filter mode!");
            }
        }
    }

   private:
    WrapMode m_wrap     = WrapMode::SAMPLER_WRAP_REPEAT;
    FilterMode m_filter = FilterMode::SAMPLER_FILTER_NEAREST;

    static float wrap(float coord, WrapMode mode) {
        switch (mode) {
            case WrapMode::SAMPLER_WRAP_REPEAT: {
                return coord - std::floor(coord);
            }
            case WrapMode::SAMPLER_WRAP_CLAMP_TO_EDGE: {
                return std::clamp(coord, 0.0f, 1.0f);
            }
            case WrapMode::SAMPLER_WRAP_MIRROR: {
                float base = std::floor(coord);
                float rem  = coord - base;
                return (static_cast<int>(std::abs(base)) % 2 == 0) ? rem : (1.0f - rem);
            }
            default: {
                throw std::runtime_error("Sampler::wrap: Invalid wrap mode!");
            }
        }
    }

    template <typename T, size_t N>
    inline Vec<T, N> lerp(const Vec<T, N>& a, const Vec<T, N>& b, float t) const {
        Vec<T, N> ab{0.f};
        for (int i = 0; i < N; ++i) {
            ab[i] = a[i] + t * (b[i] - a[i]);
        }
        return ab;
    }
};

} // namespace spt

#endif