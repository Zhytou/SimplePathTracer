#ifndef SPT_TRACE_HPP
#define SPT_TRACE_HPP

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "BVH.hpp"
#include "Camera.hpp"
#include "Light.hpp"
#include "Ray.hpp"
#include "Scene.hpp"

namespace spt {
class Tracer {
   public:
    Tracer(int d = 10, int rrd = 3, int s = 3, float p = 0.8);
    ~Tracer() = default;

    // Render a scene and save the result as an image
    // @param scene The scene to render
    // @param imgpath The path to save the image
    void render(const Scene& scene, const std::filesystem::path& imgpath = "result.png");
    // Trace a ray through the scene and return the color at the intersection point
    // @param bvh The BVH of the scene
    // @param light The light source
    // @param ray The ray to trace
    // @param depth The current depth of the path trace
    // @return The color at the intersection point
    Vec3<float> trace(const std::shared_ptr<BVH>& bvh, const std::shared_ptr<Light>& light, const Ray& ray, int depth);
    // Postprocess the high dynamic range color to low dynamic range color
    // @param hdr The hdr color [0, inf)
    // @param range The range of the ldr color [0, range) default 255
    // @return The ldr color [0, range]
    static Vec3<float> postprocess(const Vec3<float>& hdr, float range = 255.f);
    // Mix two PDFs and return the weight of the mixed PDF
    // @param pdf1 The first PDF
    // @param pdf2 The second PDF
    // @return The weight of the mixed PDF
    static float mix(float pdf1, float pdf2);
    // Show the progress of rendering
    // @param percent The percentage of rendering
    // @param second The time cost of rendering
    static void progress(float percent, float second);

   private:
    int m_depth   = 10;   // max depth of path trace
    int m_rrdepth = 4;    // depth of russian roulette
    int m_spp     = 32;   // samples per pixel
    float m_rrp   = 0.8f; // probability of russian roulette
};

} // namespace spt

#endif
