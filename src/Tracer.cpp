#include "Tracer.hpp"
#include "Material.hpp"
#include "Scene.hpp"
#include "Triangle.hpp"

#include <cassert>
#include <chrono>
#include <fstream>
#include <functional>
#include <omp.h>
#include <rapidjson/document.h>

namespace spt {
namespace fs = std::filesystem;

Tracer::Tracer(int d, int rrd, int s, float p)
    : m_depth(d), m_rrdepth(rrd), m_spp(s), m_rrp(p) {}

void Tracer::render(const Scene& scene, const std::filesystem::path& imgpath) {
    const auto beg = std::chrono::steady_clock::now();

    auto camera = scene.getCamera();
    int height  = camera->getHeight();
    int width   = camera->getWidth();
    auto img    = std::make_shared<Image<unsigned char>>(width, height, 3);

#pragma omp parallel for num_threads(32)
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            // 1. render the pixel color by path tracing
            Vec3<float> color(0.f);
            for (int k = 0; k < m_spp; k++) {
                Ray ray = camera->emit(row, col);
                color += cast(scene, ray);
            }
            color /= m_spp;

            // 2. postprocess the color and set the image element
            color   = postprocess(color, 255.f);
            int idx = (row * width + col) * 3;
            img->setElement(row, col, 0, color.x);
            img->setElement(row, col, 1, color.y);
            img->setElement(row, col, 2, color.z);

            // 3. show progress
            float percent                    = 100.f * (row * width + col) / (height * width - 1);
            auto cur                         = std::chrono::steady_clock::now();
            std::chrono::duration<float> dur = cur - beg;
            progress(percent, dur.count());
        }
    }

    Image<unsigned char>::write(img, imgpath);
    return;
}

Vec3<float> Tracer::cast(const Scene& scene, const Ray& ray) {
    HitRecord rec;
    if (scene.getBVH()->hit(ray, DIS_EPS, INFINITY, rec)) {
        return trace(scene, ray, rec, 0);
    }
    return Vec3<float>{0.f};
}

Vec3<float> Tracer::trace(const Scene& scene, const Ray& rayi, const HitRecord& reci, int depth) {
    // 0. Initialize scene to render and color to return
    auto bvh    = scene.getBVH();
    auto lights = scene.getLights();
    Vec3<float> color(0.f);
    Vec3<float> color_e(0.f), color_d(0.f), color_ind(0.f);

    // 1. Avoid no hit and infinite recursion
    if (reci.id < 0 || depth >= m_depth) {
        return color;
    }

    // 2. Apply russianian roulette
    float rpp      = depth >= m_rrdepth ? rand(0.0f, 1.0f) : 0.f;
    float rrweight = rpp <= m_rrp ? 1.f / m_rrp : 1.f;

    // 3. Get input hit info
    auto mtl       = reci.material;
    Vec3<float> n  = reci.normal;
    Vec3<float> p  = reci.point;
    Vec2<float> uv = reci.texcoord;
    Vec3<float> wi = -rayi.getDirection(); // view direction(wi) P -> Eye

    // 4. Initialize output ray and bsdf
    Vec3<float> wo(0.f);   // light direction(wo) P -> ight
    Vec3<float> bsdf(0.f); // material bsdf value at hit point P
    float cos = 0.f;       // cosine of the angle between normal and light direction(wo)

    // 5. Get emissive light color
    if (mtl->isEmissive()) {
        color_e = mtl->getEmission();
    }

    // 6. Calculate direct light color
    if (rpp < m_rrp && !mtl->isEmissive() && !mtl->isDelta()) { // only GLOSSY and DIFFUSE materials support light sampling
        // 6.1. Sample light
        auto light = lights[rand(0ul, lights.size() - 1)];
        wo         = light->sample(p);
        bsdf       = mtl->bsdf(wi, n, wo, uv);
        cos        = std::fabs(dot(n, wo)); // for both reflection and transmission

        // 6.2. Do hit test
        Vec3<float> pp = p + (dot(n, wo) > 0.f ? n : -n) * DIS_EPS;
        Ray rayo(pp, wo);
        HitRecord reco;
        bool hit = bvh->hit(rayo, DIS_EPS, INFINITY, reco);

        // 6.3 Calculate direct light color
        if (light->isDelta()) {
            color_d = hit ? Vec3<float>{0.f} : light->getColor() * bsdf; // delta light only contribute when no object block the light
        } else {
            float pdf_light = light->pdf(wo, reco.normal, reco.distance);
            float pdf_mtl   = mtl->pdf(wi, n, wo, uv);
            float weight    = mix(pdf_light, pdf_mtl);
            color_d         = hit && reco.material->isEmissive() && pdf_light > 0.f ? light->getColor() * bsdf * cos * weight / pdf_light : Vec3<float>{0.f}; // avoid division by zero when output direction(wo) and light normal are parallel or opposite
        }
    }

    // 7. Calculate indirect light color
    if (rpp < m_rrp && !mtl->isEmissive()) {
        // 7.1. Sample material
        wo   = mtl->scatter(wi, n, uv);
        bsdf = mtl->bsdf(wi, n, wo, uv);
        cos  = std::fabs(dot(n, wo));

        // 7.2. Do hit test
        Vec3<float> pp = p + (dot(n, wo) > 0.f ? n : -n) * DIS_EPS;
        Ray rayo(pp, wo);
        HitRecord reco;
        bool hit = bvh->hit(rayo, DIS_EPS, INFINITY, reco);

        // !NOTE: Monte Carlo in this framework is designed for continuous BSDFs integration(e.g., diffuse).
        // !      For perfect specular reflection or transmission (delta distributions), the direction is deterministic.
        // ! Consequently:
        // !  - The PDF is effectively infinite (Dirac delta), so we do NOT divide by PDF.
        // !  - The cosine term (N·L) is inherently handled by the delta function’s
        // !    integration property and should NOT be explicitly multiplied.
        // ! Instead, we directly evaluate the reflected/transmitted radiance scaled by
        // ! the Fresnel factor (and η² for transmission), ensuring energy conservation.
        if (mtl->isDelta()) {
            color_ind = hit ? trace(scene, rayo, reco, depth + 1) * bsdf : Vec3<float>{0.f}; // delta material only reflects or refract when specific output directions hit an object
        } else {
            float pdf_mtl   = mtl->pdf(wi, n, wo, uv);
            float pdf_light = 0.0f;
            float weight    = 1.0F;

            if (pdf_mtl > 0.f) {
                if (hit && reco.material->isEmissive()) { // avoid duplicate direct light calculation when sampling material's ray hit area light
                    for (auto light : lights) {
                        if (light->getID() == reco.id) {
                            pdf_light = light->pdf(wo, reco.normal, reco.distance);
                            break;
                        }
                    }
                    weight    = mix(pdf_mtl, pdf_light);
                    color_ind = reco.material->getEmission() * bsdf * cos * weight / pdf_mtl; // avoid trace recursion
                } else {
                    color_ind = trace(scene, rayo, reco, depth + 1) * bsdf * cos / pdf_mtl;
                }
            }
        }
    }

    // 8. Combine colors and apply Russianian roulette
    color = color_e + (color_d + color_ind) * rrweight;
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
