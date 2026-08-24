#include "Tracer.hpp"
#include "BoxLogger.hpp"
#include "Distribution.hpp"
#include "Film.hpp"
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

    // 1. Split frame into multiple tiles
    Queue<std::pair<int, int>> tiles;
    for (int row = 0; row < height; row += m_ts) {
        for (int col = 0; col < width; col += m_ts) {
            tiles.push({row, col});
        }
    }

    // 2. Initialize timing/progress counter and synchronization
    const auto beg       = std::chrono::steady_clock::now();
    int tot              = tiles.size(); // total number of tiles
    std::atomic<int> cnt = 0;            // completed tile counter
    std::mutex mtx;

    // 3. Define tile worker routine
    auto renderTile = [&]() {
        std::pair<int, int> tile;
        while (tiles.pop(tile)) {
            // 3.1 render the pixel color by path tracing
            int r0 = tile.first, c0 = tile.second;
            int r1 = std::min(height, r0 + m_ts), c1 = std::min(width, c0 + m_ts);
            for (int r = r0; r < r1; r++) {
                for (int c = c0; c < c1; c++) {
                    for (int k = 0; k < m_spp; k++) {
                        Vec2f coord(r, c);
                        trace(scene, coord + jitter(k)); // always add jittering to avoid fixed subpixel samples, otherwise may cause aliasing and black pixels in the output image
                    }
                }
            }

            // 3.2 show progress
            float per = 100.f * ++cnt / tot;              // percentage of accomplished tiles
            auto cur  = std::chrono::steady_clock::now(); // current time

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

    // 5. Convert the rendered film into image and do postprocessing
    auto img = camera->getFilm()->resolve(m_spp);
    Image<unsigned char>::write(img, imgpath);

    return;
}

Vec2f Tracer::jitter(int k) {
    static int n   = std::max(2, m_spp / 16);
    static int n2  = n * n;
    static float d = 1.f / n;

    k       = k % n2;
    float x = (k / n) * d + rand(0.f, d);
    float y = (k % n) * d + rand(0.f, d);
    return {x, y};
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

float Tracer::G(const Vec3f& p, const Vec3f& n_p, const Vec3f& pp, const Vec3f& n_pp) {
    auto dir = pp - p;
    float d2 = dot(dir, dir);
    float d  = std::sqrt(d2);
    dir      = dir / d;

    return std::max(0.f, dot(n_p, dir)) * std::max(0.f, dot(n_pp, -dir)) / d2;
}

bool Tracer::V(const Scene& scene, const Vec3f& p, const Vec3f& pp) {
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

void PathTracer::trace(const Scene& scene, const Vec2f& coord) const {
    auto bvh = scene.getBVH();    // intersection accelerator of the scene
    auto cam = scene.getCamera(); // camera of the scene
    auto des = scene.getDES();    // direct emitter sampler of the scene

    Ray ray;
    Intersection its;
    Vec3f radiance(0.f);                         // accumulative sum of emitted and scattered radiance
    Vec3f throughput = cam->emit(coord, ray);    // initial cumulative product of (bsdf * |cosθ| / pdf) along path
    bool hit         = bvh->intersect(ray, its); // current hit primitive or not
    bool hit_spec    = false;                    // previous hit material is specular or not
    float pdf_bsdf   = 0.f;                      // previous pdf of the bsdf sampling
    float weight_rr  = 1.f;                      // current weight of russian roulette
    Vec3f wi_local(0.f);                         // local view direction(wi) P -> Eye
    Vec3f wo_local(0.f);                         // local light direction(wo) P -> Light

    for (int depth = 0; hit && depth < m_depth; depth++) {
        // 1. Collect hit info
        auto prm = scene.getPrimitive(its.id); // primitive hit by the ray
        auto mtl = prm->getMaterial();         // material of the hit primitive(could be nullptr if emissive)
        auto emt = prm->getEmitter();          // emitter of the hit primitive

        // 2. Initialize geometry info
        const Vec3f& o  = ray.getOrigin(); // ray origin
        const Vec3f& p  = its.point;       // hit point
        const Vec3f& n  = its.normal;      // normal at the hit point
        const Vec2f& uv = its.texcoord;    // texture coordinate at the hit point

        wi_local = its.toLocal(-ray.getDirection());
        // 3. Calculate emissive light radiance or do direct light sampling
        if (emt) { // hit emissive primitive
            auto emitted = emt->le(wi_local);

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
            Vec3f pp, nn;                   // sampled light point and normal
            auto emts    = des->sample();   // sampled emitter
            float prob   = des->prob(emts); // the corresponding selection probability
            auto emitted = emts->sample(p, pp, nn);

            // 3.2 Accept connection only if mutually visible and geometrically valid:
            //     V ensures no occlusion; G > 0 ensures both surfaces face each other
            bool vis  = V(scene, p, pp); // visibility term
            float geo = G(p, n, pp, nn); // geometry term
            if (vis && geo > 0.f) {
                wo_local         = its.toLocal(normalize(pp - p));
                Vec3f bsdf       = mtl->eval(wi_local, wo_local, uv);
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

    auto film = cam->getFilm();
    film->deposit(coord, radiance);
}

void BidirectionalPathTracer::trace(const Scene& scene, const Vec2f& coord) const {
    auto cam = scene.getCamera();
    auto des = scene.getDES();

    int size_emt = 0, size_cam = 0;
    std::vector<PathVertex> path_emt, path_cam;
    Ray ray;
    Intersection its;
    PathVertex pv;

    // 1. Trace the emitter path
    {
        auto emts = des->sample();        // sampled emitter
        auto prms = emts->getPrimitive(); // primitive of the emitter
        auto spes = prms->getShape();     // shape of the emitter
        auto prob = des->prob(emts);      // selection probability

        pv.throughput = emts->sample(ray, its.normal) / prob; // sample ray and get the initial throughput
        its.id        = prms->getID();
        its.distance  = 0.f;
        its.point     = ray.getOrigin();
        its.texcoord  = spes->parameterize(its.point);
        TBN(its.normal, its.tangent, its.bitangent);
        pv.intersection = its;
        pv.forward_pdf  = emts->pdf() * prob;
        pv.backward_pdf = 0.f;
        pv.delta        = false; // TODO: add indicator for delta emitter
        pv.type         = PathVertexType::EMITTER;

        path_emt.push_back(pv);
        size_emt = subtrace(scene, path_emt, ray, emts->pdf(its.toLocal(ray.getDirection())), true);
    }

    // 2. Trace the camera path
    {
        its.id          = -1;
        its.distance    = 0.f;
        its.point       = cam->getEye();
        its.normal      = cam->getAxis(2); // forward(target - eye)
        its.tangent     = cam->getAxis(0); // right
        its.bitangent   = cam->getAxis(1); // up
        pv.intersection = its;
        pv.throughput   = cam->emit(coord, ray);
        pv.forward_pdf  = cam->pdf();
        pv.backward_pdf = 0.f;
        pv.delta        = false;
        pv.type         = PathVertexType::CAMERA;

        path_cam.push_back(pv);
        size_cam = subtrace(scene, path_cam, ray, cam->pdf(cam->toLocal(ray.getDirection())), false);
    }

    // 3. Connect the emitter path and the camera path to form the bidirectional path tracer
    auto film = cam->getFilm();
    for (int s = 0; s <= size_emt; s++) {     // s is source emitter
        for (int t = 1; t <= size_cam; t++) { // t is target camera
            int depth = s + t - 2;
            // exclude (0, 1) and (1, 1)
            if ((s == 1 && t == 1) || depth < 0 || depth > m_depth) { continue; }
            // connect path_emt[0, s - 1] to path_cam[0, t - 1]
            Vec2f coord_splat = coord;
            Vec3f radiance    = connect(scene, path_emt, path_cam, {s, t}, coord_splat);
            if (t >= 2) {
                film->deposit(coord, radiance);
            } else {
                film->splat(coord_splat, radiance);
            }
        }
    }
}

int BidirectionalPathTracer::subtrace(const Scene& scene, std::vector<PathVertex>& path, Ray& ray, float pdf, bool is_emissive) const {
    auto cam = scene.getCamera(); // camera of the scene
    auto bvh = scene.getBVH();    // intersection accelerator of the scene
    auto des = scene.getDES();    // direct emitter sampler of the scene

    if (path.size() != 1) { throw std::runtime_error("subtrace: path size must be 1"); }
    Vec3f tp = path[0].throughput;

    for (int depth = 0; depth < m_depth; ++depth) {
        // 1. Collect the intersection info
        Intersection its;
        if (!bvh->intersect(ray, its)) { break; }
        auto prm = scene.getPrimitive(its.id);
        auto mtl = prm->getMaterial();
        auto emt = prm->getEmitter();

        // 2. Generate the new hit path vertex
        auto nxt = PathVertex{
            .intersection = its,
            .throughput   = tp,
            .delta        = mtl && mtl->isDelta(),
            .type         = emt ? PathVertexType::EMITTER : PathVertexType::SURFACE,
        };

        // 3. Update the forward and backward pdf of the previous and next vertex if necessary
        if (path.size() >= 2) {
            auto& pre        = path[path.size() - 2];
            auto& cur        = path[path.size() - 1];
            cur.backward_pdf = cur.pdf(scene, &nxt, pre);
            nxt.forward_pdf  = cur.pdf(scene, &pre, nxt);
        } else { // set the forward_pdf of path[1] vertex
            nxt.forward_pdf = w2a(pdf, its.distance, dot(its.normal, -ray.getDirection()));
        }
        path.push_back(nxt);

        // 4. Sample bsdf to get a new ray
        if (!mtl) { break; }
        Vec3f wi_local(its.toLocal(-ray.getDirection())), wo_local(0.f);
        tp *= mtl->sample(wi_local, wo_local, its.texcoord, is_emissive);
        ray = Ray(its.point, its.toWorld(wo_local), DIS_EPS, INFINITY);
    }

    return path.size();
}

Vec3f BidirectionalPathTracer::connect(const Scene& scene, const std::vector<PathVertex>& path_emt, const std::vector<PathVertex>& path_cam, const Vec2i& strategy, Vec2f& coord_raster) const {
    int s = strategy.x, t = strategy.y;
    Vec3f radiance(0.f);
    PathVertex vex; // new temporary vertex for mis-weight calculation

    if (s == 0) { // suggest the t-1 vertex in camera path should be emissive
        const auto& vt = path_cam[t - 1];
        if (vt.type != PathVertexType::EMITTER) { return Vec3f(0.f); }
        radiance = t >= 2 ? vt.le(scene, path_cam[t - 2]) * vt.throughput : Vec3f(0.f);
    } else if (s == 1) { // sample a new emitter vertex and do NEE
        const auto& vt = path_cam[t - 1];
        if (vt.delta) { return Vec3f(0.f); }

        auto des           = scene.getDES();
        auto emts          = des->sample();
        float prob         = des->prob(emts);
        auto& vs           = vex; // new temporary emitter vertex
        vs.intersection.id = emts->getPrimitive()->getID();
        vs.throughput      = emts->sample(vt.intersection.point, vs.intersection.point, vs.intersection.normal) / prob;
        vs.forward_pdf     = emts->pdf() * prob;
        vs.backward_pdf    = 0.f;
        vs.type            = PathVertexType::EMITTER;

        bool vis  = V(scene, vs.intersection.point, vt.intersection.point);
        float geo = G(vt.intersection.point, vt.intersection.normal, vs.intersection.point, vs.intersection.normal);
        if (!vis || geo <= 0.f) { return Vec3f(0.f); }
        radiance = vt.throughput * vt.eval(scene, path_cam[t - 2], vs) * geo;
    } else if (t == 1) { // sample a new camera vertex
        const auto& vs = path_emt[s - 1];
        if (vs.delta) { return Vec3f(0.f); }

        auto cam           = scene.getCamera();
        auto& vt           = vex; // new temporary camera vertex
        vt.intersection.id = -1;
        vt.throughput      = cam->sample(vs.intersection.point, vt.intersection.point, coord_raster);
        vt.forward_pdf     = cam->pdf();
        vt.backward_pdf    = 0.f;
        vt.type            = PathVertexType::CAMERA;

        bool vis  = V(scene, vs.intersection.point, vt.intersection.point);
        float geo = G(vt.intersection.point, vt.intersection.normal, vs.intersection.point, vs.intersection.normal);
        if (!vis || geo <= 0.f) { return Vec3f(0.f); }
        radiance = vt.throughput * geo * vs.eval(scene, path_emt[s - 2], vt) * vs.throughput;
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

    float weight_mis = weight(scene, path_emt, path_cam, vex, strategy);
    return radiance * weight_mis;
}

float BidirectionalPathTracer::weight(const Scene& scene, const std::vector<PathVertex>& path_emt, const std::vector<PathVertex>& path_cam, const PathVertex& vex, const Vec2i& strategy) const {
    int s = strategy.x, t = strategy.y;
    if (s + t == 2) { return 1.f; }

    float sum_ri = 0.f, ri;
    auto remap0  = [](float f) -> float { return f != 0 ? f : 1; }; // helper function remap0 that deals with Dirac delta functions

    const PathVertex *vs1 = s >= 1 ? (s == 1 ? &vex : &path_emt[s - 1]) : nullptr, *vs2 = s >= 2 ? &path_emt[s - 2] : nullptr;
    const PathVertex *vt1 = t >= 1 ? (t == 1 ? &vex : &path_cam[t - 1]) : nullptr, *vt2 = t >= 2 ? &path_cam[t - 2] : nullptr;
    float vs1_bwd_pdf = 0.f, vs2_bwd_pdf = 0.f;
    float vt1_bwd_pdf = 0.f, vt2_bwd_pdf = 0.f;
    // when s == 0, vt assume to be a emitter vertex.
    if (vt1) { vt1_bwd_pdf = s > 0 ? vs1->pdf(scene, vs2, *vt1) : vt1->pdf(scene, nullptr, *vt2, true); }  // pdf_light_origin
    if (vt2) { vt2_bwd_pdf = s > 0 ? vt1->pdf(scene, vs1, *vt2) : vt1->pdf(scene, nullptr, *vt2, false); } // pdf_light_direction
    if (vs1) { vs1_bwd_pdf = vt1->pdf(scene, vt2, *vs1); }
    if (vs2) { vs2_bwd_pdf = vs1->pdf(scene, vt1, *vs2); }

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
        auto emt   = scene.getPrimitive(path_emt[i].intersection.id)->getEmitter();
        bool delta = i > 0 ? path_emt[i - 1].delta : (emt == nullptr || emt->isDelta());
        if (!path_emt[i].delta && !delta) { sum_ri += ri; }
    }

    return 1 / (1 + sum_ri);
}

} // namespace spt
