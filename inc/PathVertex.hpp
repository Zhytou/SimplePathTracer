#ifndef SPT_PATH_VERTEX_HPP
#define SPT_PATH_VERTEX_HPP

#include "Intersection.hpp"
#include "Scene.hpp"
#include "Utils.hpp"

namespace spt {

struct PathVertex {
    Intersection intersection;
    Vec3f throughput   = Vec3f(1.f); // cumulative product of (bsdf * |cosθ| / pdf) along the path
    float forward_pdf  = 0.f;        // sampling pdf along the path for mis
    float backward_pdf = 0.f;        // reverse sampling pdf along the path for mis
    bool delta         = false;      // delta bsdf(mirror/ideal glass)

    Vec3f eval(const Scene& scene, const PathVertex& pre, const PathVertex& nxt) const {
        Vec3f wi = normalize(pre.intersection.point - intersection.point);
        Vec3f wo = normalize(nxt.intersection.point - intersection.point);
        auto mtl = scene.getPrimitive(intersection.id)->getMaterial();
        if (mtl) {
            return mtl->eval(intersection.toLocal(wi), intersection.toLocal(wo), intersection.texcoord);
        }
        return Vec3f(0.f);
    }

    Vec3f eval(const Scene& scene, const PathVertex& nxt) const {
        auto emt = scene.getPrimitive(intersection.id)->getEmitter();
        if (emt) {
            Vec3f w = normalize(nxt.intersection.point - intersection.point);
            return emt->eval(intersection.toLocal(w));
        }
        return Vec3f(0.f);
    }

    float pdf(const Scene& scene, const PathVertex* pre, const PathVertex& nxt) const {
        if (pre) {
            Vec3f wi = normalize(pre->intersection.point - intersection.point);
            Vec3f wo = normalize(nxt.intersection.point - intersection.point);
            auto mtl = scene.getPrimitive(intersection.id)->getMaterial();
            if (mtl) {
                float pdf_w = mtl->pdf(intersection.toLocal(wi), intersection.toLocal(wo), intersection.texcoord);
                float pdf_a = w2a(pdf_w, distance(intersection.point, nxt.intersection.point), dot(wo, nxt.intersection.normal));
                return pdf_a;
            }
        } else {
        }

        return 0.f;
    }
};

} // namespace spt

#endif
