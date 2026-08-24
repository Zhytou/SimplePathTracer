#ifndef SPT_TRACE_HPP
#define SPT_TRACE_HPP

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "PathVertex.hpp"
#include "Ray.hpp"
#include "Scene.hpp"

namespace spt {

class Tracer {
   public:
    Tracer(int d = 10, int rrd = 3, int spp = 3, float rrp = 0.8, float lum = INFINITY, int ts = 32, int thd = 32);
    virtual ~Tracer() {}

    virtual const char* getTypeName() const       = 0;
    virtual const char* getAbbrevTypeName() const = 0;
    int getDepth() const { return m_depth; }
    int getRussianRrouletteDepth() const { return m_rrdepth; }
    int getSamplesPerPixel() const { return m_spp; }
    float getRussianRrouletteProb() const { return m_rrp; }
    float getLumiosityTheshold() const { return m_lum; }

    /**
     * @brief  Render a scene and save the result as an image
     * @param scene The scene to render
     * @param imgpath The path to save the image
     */
    void render(const Scene& scene, const std::filesystem::path& imgpath = "result.png");
    /**
     * @brief  Trace a ray through the scene and evaluate the radiance at the given pixel
     * @param scene The scene to render
     * @param coord The coordinate of subpixel
     */
    virtual void trace(const Scene& scene, const Vec2f& coord) const = 0;
    /**
     * @brief Generates a stratified jittered offset within a [0, 1)^2 pixel domain.
     * 
     * Divides the pixel filter area into a grid and returns a randomized 2D sample 
     * offset for anti-aliasing.
     * 
     * @param k The sample index (0 <= k < m_spp).
     * @return The 2D subpixel offset vector in normalized pixel space.
     */
    Vec2f jitter(int k);
    /**
     * @brief  Mix two PDFs and return the weight of the mixed PDF
     * @param pdf1 The first PDF
     * @param pdf2 The second PDF
     * @return The weight of the mixed PDF
     */
    static float mix(float pdf1, float pdf2);
    /**
     * @brief  Show the progress of rendering
     * @param percent The percentage of rendering
     * @param second The time cost of rendering
     */
    static void progress(float percent, float second);
    /**
     * @brief  Evaluate the Geometry term of rendering equation for a given point p and light source pp
     * @note The G term different from the G in the microsoft bsdf model, here it is used in the rendering equation when integrating over the light source.
     * 
     * G = dot(pp - p, n) * dot(p - pp, nn) / dist4
     * 
     * @param p The shading point
     * @param n The normal of the shading point
     * @param pp The sampled light point
     * @param nn The normal of the sampled light point
     * @return The value of the G function
     */
    static float G(const Vec3f& p, const Vec3f& n, const Vec3f& pp, const Vec3f& nn);
    /**
     * @brief  Check the Visibility term of rendering equation for two given point p and pp
     * 
     * @param p The first point
     * @param pp The second point
     * @return The value of the V function
     */
    static bool V(const Scene& scene, const Vec3f& p, const Vec3f& pp);

   protected:
    int m_depth   = 10;       // max depth of path trace
    int m_rrdepth = 4;        // depth of russian roulette
    int m_spp     = 32;       // samples per pixel
    float m_rrp   = 0.8f;     // probability of russian roulette
    float m_lum   = INFINITY; // luminance threshold for indirect light clamping, closed by default
    int m_thd     = 32;       // number of threads used for rendering
    int m_ts      = 32;       // size of tile for parallel rendering
};

class PathTracer : public Tracer {
   public:
    PathTracer(int d = 10, int rrd = 3, int spp = 3, float rrp = 0.8, float lum = INFINITY, int ts = 32, int thd = 32) : Tracer(d, rrd, spp, rrp, lum, ts, thd) {}
    ~PathTracer() {}

    virtual const char* getTypeName() const override { return "PathTracer"; }
    virtual const char* getAbbrevTypeName() const override { return "pt"; }
    virtual void trace(const Scene& scene, const Vec2f& coord) const override;
};

class BidirectionalPathTracer : public Tracer {
   public:
    BidirectionalPathTracer(int d = 10, int rrd = 3, int spp = 3, float rrp = 0.8, float lum = INFINITY, int ts = 32, int thd = 32) : Tracer(d, rrd, spp, rrp, lum, ts, thd) {}
    ~BidirectionalPathTracer() {}

    virtual const char* getTypeName() const override { return "BidirectionalPathTracer"; }
    virtual const char* getAbbrevTypeName() const override { return "bpt"; }
    virtual void trace(const Scene& scene, const Vec2f& coord) const override;

    int subtrace(const Scene& scene, std::vector<PathVertex>& path, Ray& ray, float pdf, bool is_emt) const;
    Vec3f connect(const Scene& scene, const std::vector<PathVertex>& path_emt, const std::vector<PathVertex>& path_cam, const Vec2i& strategy, Vec2f& coord_raster) const;
    float weight(const Scene& scene, const std::vector<PathVertex>& path_emt, const std::vector<PathVertex>& path_cam, const PathVertex& vex, const Vec2i& strategy) const;
};

} // namespace spt

#endif
