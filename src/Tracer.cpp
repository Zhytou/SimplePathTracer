#include "Tracer.hpp"
#include "BoxLogger.hpp"
#include "Material.hpp"
#include "Queue.hpp"
#include "Scene.hpp"
#include "Triangle.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <fstream>
#include <functional>
#include <mutex>
#include <rapidjson/document.h>
#include <thread>

namespace spt {
namespace fs = std::filesystem;

Tracer::Tracer(int d, int rrd, int spp, float rrp, float lum, int ts, int thd)
    : m_depth(d), m_rrdepth(rrd), m_spp(spp), m_rrp(rrp), m_lum(lum), m_thd(std::min((uint)thd, std::thread::hardware_concurrency() + 1)), m_ts(ts) {

    BOX_LOG("TRACER CONFIGURATION", 115)
        << "- " << std::left << std::setw(33) << "Maximum Depth:" << std::setw(7) << m_depth << " - " << std::setw(33) << "Samples Per Pixel:" << std::setw(7) << m_spp << '\n'
        << "- " << std::left << std::setw(33) << "Russian Roulette Minimum Depth:" << std::setw(7) << m_rrdepth << " - " << std::setw(33) << "Russian Roulette Probability:" << std::setw(7) << std::fixed << std::setprecision(1) << m_rrp << '\n'
        << "- " << std::left << std::setw(33) << "Luminance Limit:" << std::setw(7) << std::fixed << std::setprecision(1) << m_lum << " - " << std::setw(33) << "Tile Size:" << std::setw(7) << m_ts << '\n'
        << "- " << std::left << std::setw(33) << "Number of Threads:" << std::setw(7) << m_thd << '\n';
}

void Tracer::render(const Scene& scene, const std::filesystem::path& imgpath) {
    // 0. Prepare render resources
    auto camera      = scene.getCamera();
    const int height = camera->getHeight();
    const int width  = camera->getWidth();
    const int n      = sqrt(m_spp); // subpixel sampling factor(n * n grid)
    auto img         = std::make_shared<Image<unsigned char>>(width, height, 3);

    // 1. Split frame into mutiple tiles
    struct Tile {
        int r0 = 0, c0 = 0;
        int r1 = 0, c1 = 0;
    };
    Queue<Tile> tiles;
    for (int row = 0; row < height; row += m_ts) {
        for (int col = 0; col < width; col += m_ts) {
            tiles.push({row, col, std::min(height, row + m_ts), std::min(width, col + m_ts)});
        }
    }

    // 2. Initialize timing/progress counter and synchronization
    const auto beg       = std::chrono::steady_clock::now();
    int tot              = tiles.size(); // total number of tiles
    std::atomic<int> cnt = 0;            // completed tile counter
    std::mutex mtx;

    // 3. Define tile worker routine
    auto renderTile = [&]() {
        Tile tile;
        while (tiles.pop(tile)) {
            for (int row = tile.r0; row < tile.r1; row++) {
                for (int col = tile.c0; col < tile.c1; col++) {
                    // 3.1 render the pixel color by path tracing
                    Vec3<float> color(0.f);
                    for (int k = 0; k < m_spp; k++) {
                        Ray ray = camera->emit(row, col, k, n);
                        color += trace(scene, ray);
                    }
                    color /= m_spp;

                    // 3.2 postprocess the output radiance[0, +inf] into color[0, 255] and set the image element
                    color   = postprocess(color, 255.f);
                    int idx = (row * width + col) * 3;
                    img->setElement(row, col, 0, color.x);
                    img->setElement(row, col, 1, color.y);
                    img->setElement(row, col, 2, color.z);
                }
            }

            // 3.3 show progress
            float per                        = 100.f * ++cnt / tot;              // percentage of accomplished tiles
            auto cur                         = std::chrono::steady_clock::now(); // current time
            std::chrono::duration<float> dur = cur - beg;
            {
                std::lock_guard<std::mutex> lock(mtx);
                progress(per, dur.count());
            }
        }
    };

    // 4. Spawn worker threads and wait for completion
    std::vector<std::thread> thds;
    for (int i = 0; i < m_thd; i++) {
        thds.emplace_back(renderTile);
    }
    for (auto& thd : thds) {
        thd.join();
    }
    Image<unsigned char>::write(img, imgpath);

    return;
}

Vec3<float> Tracer::postprocess(const Vec3<float>& hdr, float range) {
    Vec3<float> ldr = hdr;

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

float Tracer::mix(float pdf1, float pdf2) {
    pdf1 = pdf1 * pdf1;
    pdf2 = pdf2 * pdf2;

    return pdf1 / (PDF_EPS + pdf1 + pdf2);
}

void Tracer::progress(float percent, float second) {
    const int barWidth = 50;
    std::cout << "[";
    int pos = static_cast<int>(barWidth * percent / 100.0f);
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos)
            std::cout << "=";
        else if (i == pos)
            std::cout << ">";
        else
            std::cout << " ";
    }
    std::cout << "] " << std::setw(5) << std::fixed << std::setprecision(2) << percent;
    std::cout << "% " << std::setw(7) << second << 's';
    if (percent >= 100.0f) {
        std::cout << "\n";
    } else {
        std::cout << '\r';
        std::cout.flush();
    }
}

float Tracer::G(const Vec3<float>& p, const Vec3<float>& n_p, const Vec3<float>& pp, const Vec3<float>& n_pp) {
    auto dir = pp - p;
    float d2 = dot(dir, dir);
    float d  = std::sqrt(d2);
    dir      = dir / d;

    return std::max(0.f, dot(n_p, dir)) * std::max(0.f, dot(n_pp, -dir)) / d2;
}

Vec3<float> PathTracer::trace(const Scene& scene, Ray& ray) const {
    auto its       = Intersection();           // intersection info
    auto bvh       = scene.getBVH();           // intersection accelerator of the scene
    auto des       = scene.getDES();           // direct emitter sampler of the scene
    auto hit       = bvh->intersect(ray, its); // current hit primitive or not
    auto hit_spec  = false;                    // previous hit material is specular or not
    auto pdf_bsdf  = 0.f;                      // previous pdf of the bsdf sampling
    auto weight_rr = 1.f;                      // current weight of russian roulette
    Vec3<float> radiance(0.f);                 // accumulative emitted and scattered radiance
    Vec3<float> throughput(1.f);

    for (int depth = 0; hit && depth < m_depth; depth++) {
        // 1. Collect hit info
        auto prm = scene.getPrimitive(its.id); // primitive hit by the ray
        auto mtl = prm->getMaterial();         // material of the hit primitive(could be nullptr if emissive)
        auto emt = prm->getEmitter();          // emitter of the hit primitive

        // 2. Initialize geometry info
        const Vec3<float>& o  = ray.getOrigin(); // ray origin
        const Vec3<float>& p  = its.point;       // hit point
        const Vec3<float>& n  = its.normal;      // normal at the hit point
        const Vec2<float>& uv = its.texcoord;    // texture coordinate at the hit point

        Vec3<float> wi       = -ray.getDirection(); // view direction(wi) P -> Eye
        Vec3<float> wo       = Vec3<float>(0.f);    // light direction(wo) P -> ight
        Vec3<float> wi_local = its.toLocal(wi);
        Vec3<float> wo_local = its.toLocal(wo);

        // 3. Calculate emissive light radiance or do direct light sampling
        if (emt) {
            auto emitted = emt->eval(wi);

            if (depth == 0 || hit_spec) { // return emissive radiance if at the first depth or hit a specular material
                radiance += throughput * emitted;
            } else { // do multiple importance sampling
                float dist       = length(p - o);
                float cos_theta  = std::max(0.f, dot(n, o - p) / dist);
                float pdf_emt    = a2w(emt->pdf(), dist, cos_theta) * des->prob(emt);
                float pdf_mtl    = pdf_bsdf;
                float weight_mis = mix(pdf_mtl, pdf_emt);
                radiance += throughput * emitted * weight_mis;
            }
        } else if (mtl && !mtl->isDelta()) {
            // 3.1 Sample the emitter
            Vec3<float> pp, nn;               // sampled light point and normal
            auto [emt, prob] = des->sample(); // sampled emitter and corresponding probability
            auto emitted     = emt->sample(p, pp, nn) / prob;

            // 3.2 Check if the sampled light point is visible from the hit point
            wo         = normalize(pp - p);
            wo_local   = its.toLocal(wo);
            float tmin = DIS_EPS;
            float tmax = length(pp - p) - DIS_EPS;
            Ray shadow_ray(p, wo, tmin, tmax);
            Intersection shadow_its;
            bool visible = !bvh->intersect(shadow_ray, shadow_its);
            float g      = G(p, n, pp, nn);

            if (visible && g > 0.f) {
                Vec3<float> bsdf = mtl->eval(wi_local, wo_local, uv);
                float dist       = length(pp - p);
                float cos_theta  = std::max(0.f, dot(nn, pp - p) / dist); //Cosine of the angle between light surface normal and outgoing light direction
                float pdf_emt    = a2w(emt->pdf(), dist, cos_theta) * prob;
                float pdf_mtl    = mtl->pdf(wi_local, wo_local, uv);
                float weight_mis = mix(pdf_emt, pdf_mtl);
                radiance += throughput * emitted * bsdf * g * weight_mis;
            }
        }

        // 4. Apply russian roulette and do indirect light sampling
        if (mtl) {
            // 4.1 Sample the material
            throughput *= mtl->sample(wi_local, wo_local, uv);
            hit_spec = mtl->isDelta();
            pdf_bsdf = mtl->pdf(wi_local, wo_local, uv);

            // 4.2 Update ray and trace the reflected/refracted ray
            wo  = its.toWorld(wo_local);
            ray = Ray(p, wo, DIS_EPS, INFINITY);
            hit = bvh->intersect(ray, its);
        } else {
            hit = false;
        }
    }

    return radiance;
}

Vec3<float> BidirectionalPathTracer::trace(const Scene& scene, Ray& ray) const {
    std::vector<PathVertex> cam_path, emt_path;
    int m             = subtrace(scene, ray, cam_path);
    int n             = subtrace(scene, emt_path);
    Vec3<float> color = Vec3<float>(0.f);
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
        }
    }
    return color;
}

int BidirectionalPathTracer::subtrace(const Scene& scene, Ray& ray, std::vector<PathVertex>& cam_path) const {
    auto bvh = scene.getBVH(); // intersection accelerator of the scene
    auto des = scene.getDES(); // direct emitter sampler of the scene

    Vec3<float> throughput(1.f);
    float pdf;

    for (int depth = 0; depth < m_depth; ++depth) {
        auto its = Intersection();
        if (!bvh->intersect(ray, its)) { break; }

        auto prm = scene.getPrimitive(its.id);
        auto mtl = prm->getMaterial();
        auto emt = prm->getEmitter();

        auto p  = its.point;
        auto n  = its.normal;
        auto uv = its.texcoord;

        auto wi       = -ray.getDirection();
        auto wo       = Vec3<float>(0.f);
        auto wi_local = its.toLocal(wi);
        auto wo_local = its.toLocal(wo);
        throughput *= mtl->sample(wi_local, wo_local, uv);
        pdf = mtl->pdf(wi_local, wo_local, uv);

        auto v = PathVertex{
            .its  = its,
            .wi   = wi,
            .tp   = throughput,
            .pdf  = pdf,
            .spec = mtl && mtl->isDelta(),
        };
        cam_path.push_back(v);

        ray = Ray(p, wo, DIS_EPS, INFINITY);
    }

    return cam_path.size();
}

int BidirectionalPathTracer::subtrace(const Scene& scene, std::vector<PathVertex>& emt_path) const {
    auto bvh = scene.getBVH(); // intersection accelerator of the scene
    auto des = scene.getDES(); // direct emitter sampler of the scene

    Vec3<float> throughput(1.f);
    float pdf;

    auto org = Vec3<float>(0.f);
    auto dir = Vec3<float>(0.f);
    auto n   = Vec3<float>(0.f);

    auto [emt, prob] = des->sample();
    auto emitted     = emt->sample(org, dir, n) / prob;
    auto ray         = Ray(Vec3<float>(0.f), Vec3<float>(0.f));

    for (int depth = 0; depth < m_depth; ++depth) {
        auto its = Intersection();
        if (!bvh->intersect(ray, its)) { break; }

        auto prm = scene.getPrimitive(its.id);
        auto mtl = prm->getMaterial();
        auto emt = prm->getEmitter();

        auto p  = its.point;
        auto n  = its.normal;
        auto uv = its.texcoord;

        auto wi       = -ray.getDirection();
        auto wo       = Vec3<float>(0.f);
        auto wi_local = its.toLocal(wi);
        auto wo_local = its.toLocal(wo);
        throughput *= mtl->sample(wi_local, wo_local, uv);
        pdf = mtl->pdf(wi_local, wo_local, uv);

        auto v = PathVertex{
            .its  = its,
            .wi   = wi,
            .tp   = throughput,
            .pdf  = pdf,
            .spec = mtl && mtl->isDelta(),
        };
        emt_path.push_back(v);

        ray = Ray(p, wo, DIS_EPS, INFINITY);
    }

    return emt_path.size();
}

Vec3<float> BidirectionalPathTracer::connect(const Scene& scene) const {
    return Vec3<float>(0.f);
}

} // namespace spt
