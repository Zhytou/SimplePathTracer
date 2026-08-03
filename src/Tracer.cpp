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
                        color += cast(scene, ray);
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

Vec3<float> Tracer::cast(const Scene& scene, const Ray& ray) {
    IntersectRecord rec;
    if (scene.getBVH()->intersect(ray, rec)) {
        return trace(scene, ray, rec, 0);
    }
    return Vec3<float>(0.f);
}

Vec3<float> Tracer::trace(const Scene& scene, const Ray& rayi, IntersectRecord& reci, int depth) {
    // 0. Initialize scene to render and color to return
    auto bvh = scene.getBVH();
    auto des = scene.getDES();
    Vec3<float> color(0.f);
    Vec3<float> color_e(0.f), color_d(0.f), color_ind(0.f);

    // 1. Avoid no hit and infinite recursion
    if (reci.id < 0 || depth >= m_depth) {
        return color;
    }

    // 2. Get input hit info
    int id        = reci.id;
    auto prmi     = scene.getPrimitive(id);    // primitive hit by the rayi
    auto mtli     = prmi->getMaterial();       // material hit by the rayi(could be nullptr if emissive primitive)
    auto emti     = prmi->getEmitter();        // emitter hit by the rayi
    auto int_medi = prmi->getInteriorMedium(); // internal medium hit by the rayi
    auto ext_medi = prmi->getExteriorMedium(); // external medium hit by the rayi

    Vec3<float> n  = reci.normal;
    Vec3<float> p  = reci.point;
    Vec2<float> uv = reci.texcoord;
    float eta      = int_medi && ext_medi ? ext_medi->getIOR() / int_medi->getIOR() : NAN;

    // 3. Initialize output ray and bsdf
    Vec3<float> wi   = -rayi.getDirection(); // view direction(wi) P -> Eye
    Vec3<float> wo   = Vec3<float>(0.f);     // light direction(wo) P -> ight
    Vec3<float> bsdf = Vec3<float>(0.f);     // material bsdf value at hit point P
    float cos        = 0.f;                  // cosine of the angle between normal and light direction(wo)

    // 4. Get emissive light color
    if (emti) {
        color_e = emti->getColor();
    }

    // 5. Calculate specular manifold sampling color
    if (0) {
    }

    // 6. Calculate direct light sampling color
    if (!emti && !mtli->isDelta()) {
        // 6.1 Sample non-delta light
        auto [emts, prob] = des->sample(); // emitter sampled from the scene

        wo   = emts->sample(p);
        bsdf = mtli->bsdf(wi, n, wo, uv, eta);
        cos  = std::fabs(dot(n, wo)); // for both reflection and transmission

        // 6.2 Do hit test
        Ray rayo(p, wo, DIS_EPS, INFINITY);
        IntersectRecord reco;
        bool hit  = bvh->intersect(rayo, reco);
        auto prmo = hit ? scene.getPrimitive(reco.id) : nullptr;
        auto emto = hit ? prmo->getEmitter() : nullptr;

        // 6.3 Calculate probability density function of
        float pdf_emt = emts->pdf(wo, reco.normal, reco.distance) * prob;
        float pdf_mtl = mtli->pdf(wi, n, wo, uv, eta);
        float weight  = mix(pdf_emt, pdf_mtl);

        color_d += hit && emto && pdf_emt > 0.f ? emto->getColor() * bsdf * cos * weight / pdf_emt : Vec3<float>(0.f); // avoid division by zero when output direction(wo) and light normal are parallel or opposite
    }

    // 7. Apply russian roulette and calculate material sampling color
    float rrp      = depth >= m_rrdepth ? rand(0.0f, 1.0f) : 0.f; // when tracing depth deeper than depth threshold, apply russian roulette
    float rrweight = rrp >= m_rrp ? 0.f : 1.f / m_rrp;            // when rpp higher than rpp threshold, no need to calculate indirect color
    if (!emti && rrp < m_rrp) {
        // 7.1 Sample material
        wo   = mtli->scatter(wi, n, uv, eta);
        bsdf = mtli->bsdf(wi, n, wo, uv, eta);
        cos  = std::fabs(dot(n, wo));

        // 7.2 Do hit test
        Ray rayo(p, wo, DIS_EPS, INFINITY);
        IntersectRecord reco;
        bool hit  = bvh->intersect(rayo, reco);
        auto prmo = hit ? scene.getPrimitive(reco.id) : nullptr;
        auto emto = hit ? prmo->getEmitter() : nullptr;

        // 7.3 Calculate indirect light color
        if (mtli->isDelta()) {
            color_ind = hit ? trace(scene, rayo, reco, depth + 1) * bsdf : Vec3<float>(0.f);
        } else {
            float prob    = des->prob(emti);
            float pdf_mtl = mtli->pdf(wi, n, wo, uv, eta);
            float pdf_emt = emto ? emto->pdf(wo, reco.normal, reco.distance) * prob : 0.f;
            float weight  = mix(pdf_mtl, pdf_emt);

            color_ind = hit && pdf_mtl > 0.f ? trace(scene, rayo, reco, depth + 1) * bsdf * cos * weight / pdf_mtl : Vec3<float>(0.f); // avoid duplicate direct light calculation when sampling material's ray hit area light
        }

        // 7.4 Avoid firefly(e.g. white noise pixels casued by diffuse cuboid->sepcular sphere->light render chain in metal-sphere.json)
        if (!mtli->isDelta() && dot(color_ind, Vec3<float>(0.2126f, 0.7152f, 0.0722f)) > m_lum) { // luminance threshold
            color_ind = Vec3<float>(0.f);
        }
    }

    // 8. Calculate final output color
    color = color_e + color_d + color_ind * rrweight;

    return color;
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
    // pdf1 = pdf1 * pdf1;
    // pdf2 = pdf2 * pdf2;

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

} // namespace spt
