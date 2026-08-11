#ifndef SPT_TRACE_HPP
#define SPT_TRACE_HPP

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Ray.hpp"
#include "Scene.hpp"

namespace spt {
class Tracer {
   public:
    Tracer(int d = 10, int rrd = 3, int spp = 3, float rrp = 0.8, float lum = INFINITY, int ts = 32, int thd = 32);
    virtual ~Tracer() {}

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
     * @brief  Evaluate the G function for a given point p and light source pp
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
     * @brief  Convert the area PDF to the solid angle PDF
     * @param pdf_a Area PDF
     * @param dist Distance between the shading point and the light source
     * @param cos_theta Cosine of the angle between light surface normal and outgoing light direction
     * @return Corresponding solid-angle PDF defined over direction space
     */
    static float a2w(float pdf_a, float dist, float cos_theta) { return pdf_a * dist * dist / std::max(cos_theta, PDF_EPS); }
    /**
     * @brief  Convert the solid angle PDF to the area PDF
     * @param pdf_w Solid-angle probability density defined over direction space
     * @param dist Distance between shading point and sampled light point
     * @param cos_theta Cosine of the angle between light surface normal and outgoing light direction
     * @return Corresponding area PDF defined over light surface area
     */
    static float w2a(float pdf_w, float dist, float cos_theta) { return pdf_w * cos_theta / std::max(dist * dist, PDF_EPS); }

   protected:
    int m_depth   = 10;       // max depth of path trace
    int m_rrdepth = 4;        // depth of russian roulette
    int m_spp     = 32;       // samples per pixel
    float m_rrp   = 0.8f;     // probability of russian roulette
    float m_lum   = INFINITY; // luminance threshold for indirect light clamping, closed by default
    int m_thd     = 32;       //number of threads used for rendering
    int m_ts      = 32;       // size of tile for parallel rendering
};

class PathTracer : public Tracer {
   public:
    PathTracer(int d = 10, int rrd = 3, int spp = 3, float rrp = 0.8, float lum = INFINITY, int ts = 32, int thd = 32) : Tracer(d, rrd, spp, rrp, lum, ts, thd) {}
    ~PathTracer() {}

    virtual Vec3<float> trace(const Scene& scene, Ray& ray) const override;
};

struct PathVertex {
    Intersection its;
    Vec3<float> wi; // incoming direction to this vertex
    Vec3<float> tp; // path throughput, namely accumulated bsdf*cos/pdf
    float pdf;      // sampling pdf when arrive this vertex
    bool spec;      // delta bsdf(mirror/ideal glass)
};

class BidirectionalPathTracer : public Tracer {
   public:
    BidirectionalPathTracer(int d = 10, int rrd = 3, int spp = 3, float rrp = 0.8, float lum = INFINITY, int ts = 32, int thd = 32) : Tracer(d, rrd, spp, rrp, lum, ts, thd) {}
    ~BidirectionalPathTracer() {}

    virtual Vec3<float> trace(const Scene& scene, Ray& ray) const override;
    Vec3<float> connect(const Scene& scene) const;

    int subtrace(const Scene& scene, Ray& ray, std::vector<PathVertex>& cam_path) const;
    int subtrace(const Scene& scene, std::vector<PathVertex>& emt_path) const;
};

} // namespace spt

#endif
