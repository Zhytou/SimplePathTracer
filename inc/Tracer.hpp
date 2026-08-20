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

    virtual const char* getTypeName() const = 0;
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
     * @brief  Trace a ray through the scene and evaluate the radiance along the given ray
     * @param scene The scene to render
     * @param ray The ray to trace
     * @return The result of tracing the ray
     */
    virtual Vec3<float> trace(const Scene& scene, Ray& ray) const = 0;
    /**
     * @brief  Postprocess the high dynamic range color to low dynamic range color
     * @param hdr The hdr color [0, inf)
     * @param range The range of the ldr color [0, range) default 255
     * @return The ldr color [0, range]
     */
    static Vec3<float> postprocess(const Vec3<float>& hdr, float range = 255.f);
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
    static float G(const Vec3<float>& p, const Vec3<float>& n, const Vec3<float>& pp, const Vec3<float>& nn);
    /**
     * @brief  Check the Visibility term of rendering equation for two given point p and pp
     * 
     * @param p The first point
     * @param pp The second point
     * @return The value of the V function
     */
    static bool V(const Scene& scene, const Vec3<float>& p, const Vec3<float>& pp);

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
    virtual Vec3<float> trace(const Scene& scene, Ray& ray) const override;
};

class BidirectionalPathTracer : public Tracer {
   public:
    BidirectionalPathTracer(int d = 10, int rrd = 3, int spp = 3, float rrp = 0.8, float lum = INFINITY, int ts = 32, int thd = 32) : Tracer(d, rrd, spp, rrp, lum, ts, thd) {}
    ~BidirectionalPathTracer() {}

    virtual const char* getTypeName() const override { return "BidirectionalPathTracer"; }
    virtual Vec3<float> trace(const Scene& scene, Ray& ray) const override;

    int subtrace(const Scene& scene, std::vector<PathVertex>& path_emt) const;
    int subtrace(const Scene& scene, Ray& ray, std::vector<PathVertex>& path_cam) const;
    Vec3<float> connect(const Scene& scene, const std::vector<PathVertex>& path_emt, const std::vector<PathVertex>& path_cam, int s, int t) const;
    float weight(const Scene& scene, const std::vector<PathVertex>& path_emt, const std::vector<PathVertex>& path_cam, int s, int t) const;
};

} // namespace spt

#endif
