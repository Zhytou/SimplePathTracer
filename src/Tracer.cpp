#include "Tracer.hpp"
#include "BoxLogger.hpp"
#include "Distribution.hpp"
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

bool Tracer::V(const Scene& scene, const Vec3<float>& p, const Vec3<float>& pp) {
    auto bvh   = scene.getBVH();
    auto dis   = distance(pp, p);
    auto dir   = (pp - p) / dis;
    auto org   = p;
    float tmin = DIS_EPS;
    float tmax = dis - DIS_EPS;

    Ray ray(org, dir, tmin, tmax);
    Intersection its;
    return !bvh->intersect(ray, its);
}

Vec3<float> PathTracer::trace(const Scene& scene, Ray& ray) const {
    auto bvh = scene.getBVH(); // intersection accelerator of the scene
    auto des = scene.getDES(); // direct emitter sampler of the scene

    Vec3<float> radiance(0.f);   // accumulative sum of emitted and scattered radiance
    Vec3<float> throughput(1.f); // cumulative product of (bsdf * |cosθ| / pdf) along path

    Intersection its;                           // intersection info
    bool hit        = bvh->intersect(ray, its); // current hit primitive or not
    bool hit_spec   = false;                    // previous hit material is specular or not
    float pdf_bsdf  = 0.f;                      // previous pdf of the bsdf sampling
    float weight_rr = 1.f;                      // current weight of russian roulette
    Vec3<float> wi_local(0.f);                  // local view direction(wi) P -> Eye
    Vec3<float> wo_local(0.f);                  // local light direction(wo) P -> Light

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

        wi_local = its.toLocal(-ray.getDirection());
        // 3. Calculate emissive light radiance or do direct light sampling
        if (emt) { // hit emissive primitive
            auto emitted = emt->eval(wi_local);

            if (depth == 0 || hit_spec) { // return emissive radiance if at the first depth or hit a specular material
                radiance += throughput * emitted;
            } else { // do multiple importance sampling
                float dis        = distance(o, p);
                float cos_theta  = std::max(0.f, dot(n, o - p) / dis);
                float pdf_emt    = a2w(emt->pdf(), dis, cos_theta) * des->prob(emt);
                float pdf_mtl    = pdf_bsdf;
                float weight_mis = mix(pdf_mtl, pdf_emt);
                radiance += throughput * emitted * weight_mis;
            }
        } else if (mtl && !mtl->isDelta()) {
            // 3.1 Sample the emitter
            Vec3<float> pp, nn;                // sampled light point and normal
            auto [emts, prob] = des->sample(); // sampled emitter and corresponding probability
            auto emitted      = emts->sample(p, pp, nn) / prob;

            // 3.2 Accept connection only if mutually visible and geometrically valid:
            //     V ensures no occlusion; G > 0 ensures both surfaces face each other
            bool vis  = V(scene, p, pp); // visibility term
            float geo = G(p, n, pp, nn); // geometry term
            if (vis && geo > 0.f) {
                wo_local         = its.toLocal(normalize(pp - p));
                Vec3<float> bsdf = mtl->eval(wi_local, wo_local, uv);
                float dis        = distance(pp, p);
                float cos_theta  = std::max(0.f, dot(nn, pp - p) / dis);    //Cosine of the angle between light surface normal and outgoing light direction
                float pdf_emt    = a2w(emts->pdf(), dis, cos_theta) * prob; // pdf of the emitter sampling in solid angle
                float pdf_mtl    = mtl->pdf(wi_local, wo_local, uv);        // pdf of the material sampling in solid angle
                float weight_mis = mix(pdf_emt, pdf_mtl);                   // multiple importance sampling weight
                radiance += throughput * emitted * bsdf * geo * weight_mis;
            }
        }

        // 4. Apply russian roulette and do indirect light sampling
        if (mtl) {
            // 4.1 Sample the material
            throughput *= mtl->sample(wi_local, wo_local, uv);
            hit_spec = mtl->isDelta();
            pdf_bsdf = mtl->pdf(wi_local, wo_local, uv);

            // 4.2 Update ray and trace the reflected/refracted ray
            ray = Ray(p, its.toWorld(wo_local), DIS_EPS, INFINITY);
            hit = bvh->intersect(ray, its);
        } else {
            hit = false;
        }
    }

    return radiance;
}

Vec3<float> BidirectionalPathTracer::trace(const Scene& scene, Ray& ray) const {
    std::vector<PathVertex> path_cam, path_emt;
    int size_emt = subtrace(scene, path_emt);
    int size_cam = subtrace(scene, ray, path_cam);

    auto radiance = Vec3<float>(0.f);
    for (int s = 0; s <= size_emt; s++) {     // s is source emitter
        for (int t = 1; t <= size_cam; t++) { // t is target camera
            int depth = s + t - 2;
            // exclude (0, 1) and (1, 1)
            if ((s == 1 && t == 1) || depth < 0 || depth > m_depth) { continue; }
            // connect path_emt[0, s - 1] to path_cam[0, t - 1]
            radiance += connect(scene, path_emt, path_cam, s, t);
        }
    }
    return radiance;
}

int BidirectionalPathTracer::subtrace(const Scene& scene, Ray& ray, std::vector<PathVertex>& path_cam) const {
    auto bvh = scene.getBVH(); // intersection accelerator of the scene

    Vec3f tp(1.f); // initial throughput
    Intersection its = {
        .id    = -1,
        .point = ray.getOrigin(),
    };
    path_cam.push_back(PathVertex{
        .intersection = its,
        .throughput   = tp,
        .forward_pdf  = 1.f,
        .backward_pdf = 0.f,
        .delta        = true,
    });

    for (int depth = 0; depth < m_depth; ++depth) {
        // 1. Collect the intersection info
        if (!bvh->intersect(ray, its)) { break; }
        auto prm = scene.getPrimitive(its.id);
        auto mtl = prm->getMaterial();
        auto emt = prm->getEmitter();

        // 2. Generate the new hit path vertex, even if hit emitter directly
        auto nxt = PathVertex{
            .intersection = its,
            .throughput   = tp,
            .delta        = mtl && mtl->isDelta(),
        };

        // 3. Update the forward and backward pdf of the previous and next vertex if necessary
        if (path_cam.size() >= 2) {
            auto& pre        = path_cam[path_cam.size() - 2];
            auto& cur        = path_cam[path_cam.size() - 1];
            pre.backward_pdf = cur.pdf(scene, &nxt, pre);
            nxt.forward_pdf  = cur.pdf(scene, &pre, nxt);
        } else { // set the forward_pdf of path_cam[1] vertex
            nxt.forward_pdf = w2a(1.f, its.distance, dot(its.normal, -ray.getDirection()));
        }
        path_cam.push_back(nxt);

        // 4. Sample bsdf to get a new ray
        if (!mtl) { break; }
        Vec3f wi_local(its.toLocal(-ray.getDirection())), wo_local(0.f); // view direction(wi) P->Eye, light direction(wo) P->Light
        tp *= mtl->sample(wi_local, wo_local, its.texcoord);
        ray = Ray(its.point, its.toWorld(wo_local), DIS_EPS, INFINITY);
    }

    return path_cam.size();
}

int BidirectionalPathTracer::subtrace(const Scene& scene, std::vector<PathVertex>& path_emt) const {
    auto bvh = scene.getBVH(); // intersection accelerator of the scene
    auto des = scene.getDES(); // direct emitter sampler of the scene

    CosineDistribution dsb;
    Vec3f org, dir, norm;                                         // sampled origin, direction, and normal
    auto [emts, prob] = des->sample();                            // sampled emitter and corresponding selection probability
    Vec3f tp          = emts->sample(org, dir, norm, dsb) / prob; // initial throughput (radiance * cos / (pdf_a * pdf_w * prob))
    Ray ray(org, dir);                                            // sampled ray
    Intersection its = {
        .id     = emts->getPrimitive()->getID(),
        .point  = org,
        .normal = norm,
    };
    TBN(its.normal, its.tangent, its.bitangent);

    float pdf = dsb.pdf(its.toLocal(dir)) * emts->pdf() * prob;
    path_emt.push_back(PathVertex{
        .intersection = its,
        .throughput   = tp,
        .forward_pdf  = pdf,
        .backward_pdf = 0.f,
        .delta        = emts->isDelta(),
    });

    for (int depth = 0; depth < m_depth; ++depth) {
        // 1. Collect the intersection info
        if (!bvh->intersect(ray, its)) { break; }
        auto prm = scene.getPrimitive(its.id);
        auto mtl = prm->getMaterial();
        auto emt = prm->getEmitter();

        // 2. Generate the new hit path vertex
        auto nxt = PathVertex{
            .intersection = its,
            .throughput   = tp,
            .delta        = mtl && mtl->isDelta(),
        };

        // 3. Update the forward and backward pdf of the previous and next vertex if necessary
        if (path_emt.size() >= 2) {
            auto& pre        = path_emt[path_emt.size() - 2];
            auto& cur        = path_emt[path_emt.size() - 1];
            pre.backward_pdf = cur.pdf(scene, &nxt, pre);
            nxt.forward_pdf  = cur.pdf(scene, &pre, nxt);
        } else { // set the forward_pdf of path_emt[1] vertex
            nxt.forward_pdf = w2a(pdf / (emts->pdf() * prob), its.distance, dot(its.normal, ray.getDirection()));
        }
        path_emt.push_back(nxt);

        // 4. Sample bsdf to get a new ray
        if (!mtl) { break; }
        Vec3f wi_local(its.toLocal(-ray.getDirection())), wo_local(0.f); // light direction(wi) P->Eye, view direction(wo) P->Light
        tp *= mtl->sample(wi_local, wo_local, its.texcoord, true);
        ray = Ray(its.point, its.toWorld(wo_local), DIS_EPS, INFINITY);
    }

    return path_emt.size();
}

Vec3<float> BidirectionalPathTracer::connect(const Scene& scene, const std::vector<PathVertex>& path_emt, const std::vector<PathVertex>& path_cam, int s, int t) const {
    Vec3f radiance(0.f);

    if (s == 0) { // suggest the t-1 vertex in camera path should be emissive
        const auto& vt = path_cam[t - 1];
        if (true) {
            radiance = t >= 2 ? vt.eval(scene, path_cam[t - 2]) * vt.throughput : Vec3f(0.f);
        }
    } else if (s == 1) { // sample a new emitter and do NEE
        const auto& vt = path_cam[t - 1];
    } else if (t == 1) {
        const auto& vs = path_emt[s - 1];
        if (vs.delta) { return Vec3f(0.f); }

        const auto& cam = scene.getCamera();

    } else {
        const auto &vt = path_cam[t - 1], &vs = path_emt[s - 1];
        if (vt.delta || vs.delta) { return Vec3f(0.f); }

        Vec3f pt = vt.intersection.point, ps = vs.intersection.point;
        Vec3f nt = vt.intersection.normal, ns = vs.intersection.normal;
        bool vis  = V(scene, pt, ps);
        float geo = G(pt, nt, ps, ns);
        if (!vis || geo <= 0.f) { return Vec3f(0.f); }

        Vec3f bsdft = t >= 2 ? vt.eval(scene, path_cam[t - 2], vs) : Vec3f(1.f);
        Vec3f bsdfs = s >= 2 ? vs.eval(scene, path_emt[s - 2], vt) : Vec3f(1.f);
        radiance    = vt.throughput * bsdft * geo * bsdfs * vs.throughput;
    }

    float weight_mis = weight(scene, path_emt, path_cam, s, t);
    return radiance * weight_mis;
}

float BidirectionalPathTracer::weight(const Scene& scene, const std::vector<PathVertex>& path_emt, const std::vector<PathVertex>& path_cam, int s, int t) const {
    float sum_ri = 0.f, ri;
    auto remap0  = [](float f) -> float { return f != 0 ? f : 1; }; // helper function remap0 that deals with Dirac delta functions

    const PathVertex *vs1 = s >= 1 ? &path_emt[s - 1] : nullptr, *vs2 = s >= 2 ? &path_emt[s - 2] : nullptr;
    const PathVertex *vt1 = t >= 1 ? &path_cam[t - 1] : nullptr, *vt2 = t >= 2 ? &path_cam[t - 2] : nullptr;
    float vs1_bwd_pdf = 0.f, vs2_bwd_pdf = 0.f;
    float vt1_bwd_pdf = 0.f, vt2_bwd_pdf = 0.f;

    if (s + t == 2) { return 1.f; } // camera -> emitter

    if (vt1) {
        vt1_bwd_pdf = s > 0 ? vs1->pdf(scene, vs2, *vt1) : 0.01f; //pt->PdfLightOrigin(scene, *ptMinus, lightPdf, lightToIndex)
    }
    if (vt2) {
        vt2_bwd_pdf = s > 0 ? vt1->pdf(scene, vs1, *vt2) : 0.01f; //pt->PdfLight(scene, *ptMinus)
    }
    if (vs1) {
        vs1_bwd_pdf = vt1->pdf(scene, vt2, *vs1);
    }
    if (vs2) {
        vs2_bwd_pdf = vs1->pdf(scene, vt1, *vs2);
    }

    ri = 1.f;
    for (int i = t - 1; i > 0; --i) {
        float bwd_pdf = i == t - 1 ? vt1_bwd_pdf : (i == t - 2 ? vt2_bwd_pdf : path_cam[i].backward_pdf);
        float fwd_pdf = path_cam[i].forward_pdf;
        ri *= remap0(bwd_pdf) / remap0(fwd_pdf);
        if (!path_cam[i].delta && !path_cam[i - 1].delta) { sum_ri += ri; }
    }

    ri = 1.f;
    for (int i = s - 1; i >= 0; --i) {
        float bwd_pdf = i == s - 1 ? vs1_bwd_pdf : (i == s - 2 ? vs2_bwd_pdf : path_emt[i].backward_pdf);
        float fwd_pdf = path_emt[i].forward_pdf;
        ri *= remap0(bwd_pdf) / remap0(fwd_pdf);
        bool delta = i > 0 ? path_emt[i - 1].delta : path_emt[0].delta;
        if (!path_emt[i].delta && !delta) { sum_ri += ri; }
    }

    return 1 / (1 + sum_ri);
}

} // namespace spt
