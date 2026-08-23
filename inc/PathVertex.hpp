#ifndef SPT_PATH_VERTEX_HPP
#define SPT_PATH_VERTEX_HPP

#include "Intersection.hpp"
#include "Scene.hpp"
#include "Utils.hpp"

namespace spt {

enum class PathVertexType {
    EMITTER,
    CAMERA,
    SURFACE,
};

struct PathVertex {
    Intersection intersection;
    Vec3f throughput    = Vec3f(1.f); // cumulative product of (bsdf * |cosθ| / pdf) along the path
    float forward_pdf   = 0.f;        // sampling pdf along the path for mis
    float backward_pdf  = 0.f;        // reverse sampling pdf along the path for mis
    bool delta          = false;      // delta bsdf(mirror/ideal glass)
    PathVertexType type = PathVertexType::SURFACE;

    Vec3f eval(const Scene& scene, const PathVertex& pre, const PathVertex& nxt) const {
        if (type == PathVertexType::SURFACE) { // only surface has material bsdf
            Vec3f wi = normalize(pre.intersection.point - intersection.point);
            Vec3f wo = normalize(nxt.intersection.point - intersection.point);
            auto mtl = scene.getPrimitive(intersection.id)->getMaterial();
            return mtl->eval(intersection.toLocal(wi), intersection.toLocal(wo), intersection.texcoord);
        }
        return Vec3f(0.f);
    }

    Vec3f le(const Scene& scene, const PathVertex& nxt) const {
        if (type == PathVertexType::EMITTER) { // only emitter emits radiance
            auto emt = scene.getPrimitive(intersection.id)->getEmitter();
            Vec3f w  = normalize(nxt.intersection.point - intersection.point);
            return emt->le(intersection.toLocal(w));
        }
        return Vec3f(0.f);
    }

    float pdf(const Scene& scene, const PathVertex* pre, const PathVertex& nxt, bool org = true) const {
        auto cam = scene.getCamera();
        auto des = scene.getDES();
        auto prm = intersection.id == -1 ? nullptr : scene.getPrimitive(intersection.id);
        auto mtl = intersection.id == -1 ? nullptr : prm->getMaterial();
        auto emt = intersection.id == -1 ? nullptr : prm->getEmitter();

        if (!pre && type == PathVertexType::SURFACE) { throw std::runtime_error("PathVertex::pdf: pre must be valid for surface vertex"); }
        Vec3f wi = pre ? normalize(pre->intersection.point - intersection.point) : Vec3f(0.f);
        Vec3f wo = normalize(nxt.intersection.point - intersection.point);

        float dis       = distance(intersection.point, nxt.intersection.point);
        float cos_theta = dot(wo, nxt.intersection.normal);
        switch (type) {
            case PathVertexType::SURFACE: {
                float pdf = mtl->pdf(intersection.toLocal(wi), intersection.toLocal(wo), intersection.texcoord);
                return w2a(pdf, dis, cos_theta); // convert into area pdf
            } break;
            case PathVertexType::EMITTER: {
                if (org) { // pdf_light_origin
                    float pdf  = emt->pdf();
                    float prob = des->prob(emt);
                    return pdf * prob;
                } else { // pdf_light_direction
                    float pdf       = emt->pdf(intersection.toLocal(wo));
                    float cos_theta = std::abs(dot(nxt.intersection.normal, wo));
                    return w2a(pdf, dis, cos_theta); // convert into area pdf
                }
            } break;
            case PathVertexType::CAMERA: {
                if (org) { // pdf_camera_origin
                    return cam->pdf();
                } else { // pdf_camera_direction
                    float pdf = cam->pdf(cam->toLocal(wo));
                    return w2a(pdf, dis, cos_theta);
                }
            } break;
            default:
                break;
        }
        return 0.f;
    }
};

} // namespace spt

#endif
