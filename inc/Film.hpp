#ifndef SPT_FILM_HPP
#define SPT_FILM_HPP

#include "Image.hpp"
#include "Utils.hpp"

namespace spt {

class Film {
   public:
    Film(int width, int height) : m_width(width), m_height(height), m_colors(width * height, Vec3f(0.f)) {}
    ~Film() = default;

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    void setWidth(int width) {
        m_width = width;
        m_colors.resize(m_width * m_height);
    }
    void setHeight(int height) {
        m_height = height;
        m_colors.resize(m_width * m_height);
    }

    /**
     * @brief  Deposit the color at the given coordinate
     * @param coord The coordinate of the pixel
     * @param color The color to deposit
     */
    void deposit(const Vec2f coord, const Vec3f& color) {
        // std::lock_guard<std::mutex> lock(m_mutex);
        // no need to lock the mutex for deposit, always tile to deposit
        int x = coord.x, y = coord.y;
        m_colors[x * m_width + y] += color;
    }
    /**
     * @brief  Splat the color at the given coordinate
     * @param coord The coordinate of the pixel
     * @param color The color to splat
     */
    void splat(const Vec2f coord, const Vec3f& color) {
        std::lock_guard<std::mutex> lock(m_mutex);
        int r = coord[0], c = coord[1];
        m_splat_colors.push_back({Vec2i{r, c}, color});
    }
    /**
     * @brief  Resolve the film by adding the splats to the pixels
     */
    std::shared_ptr<Image<unsigned char>> resolve(int spp) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& [coord, color] : m_splat_colors) {
            int r = coord[0], c = coord[1];
            if (r < 0 || r >= m_height || c < 0 || c >= m_width) {
                continue;
            }
            m_colors[r * m_width + c] += color;
        }
        m_splat_colors.clear();

        auto image = std::make_shared<Image<unsigned char>>(m_width, m_height, 3);
        for (int i = 0; i < m_width * m_height; ++i) {
            int r = i / m_width, c = i % m_width;
            Vec3f color = postprocess(m_colors[i] / spp);
            image->setElement(r, c, 0, color[0]);
            image->setElement(r, c, 1, color[1]);
            image->setElement(r, c, 2, color[2]);
        }
        return image;
    }
    /**
     * @brief  Postprocess the high dynamic range color to low dynamic range color
     * @param hdr The hdr color [0, inf)
     * @param range The range of the ldr color [0, range) default 255
     * @return The ldr color [0, range]
     */
    Vec3f postprocess(const Vec3f& hdr, float range = 255.f) {
        Vec3f ldr = hdr;

        // 1. Tone mapping(ACES Filmic) f(x) = (x * (a * x + b)) / (x * (c * x + d) + e)
        const float a = 2.51f;
        const float b = 0.03f;
        const float c = 2.43f;
        const float d = 0.59f;
        const float e = 0.14f;

        ldr.x = (hdr.x * (a * hdr.x + b)) / (hdr.x * (c * hdr.x + d) + e);
        ldr.y = (hdr.y * (a * hdr.y + b)) / (hdr.y * (c * hdr.y + d) + e);
        ldr.z = (hdr.z * (a * hdr.z + b)) / (hdr.z * (c * hdr.z + d) + e);

        // 2. Gamma correction
        const float gamma = 1.0f / 2.2f;
        ldr               = pow<float>(ldr, gamma);

        // 3. Scale and clamp the ldr color to the range [0, range]
        ldr *= range;
        ldr = clamp<float>(ldr, 0.f, range);

        return ldr;
    }

   private:
    std::mutex m_mutex;
    int m_width;
    int m_height;
    std::vector<Vec3f> m_colors;
    std::vector<std::pair<Vec2i, Vec3f>> m_splat_colors;
};

} // namespace spt

#endif