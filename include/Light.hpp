#ifndef SRE_LIGHT_HPP
#define SRE_LIGHT_HPP

#include<vector>
#include<map>
#include<memory>

#include "Vec.hpp"
#include "Triangle.hpp"
#include "BVH.hpp"

namespace spt
{
    class Light {
        std::vector<std::shared_ptr<Triangle>> lights;
        std::map<std::string, float> areas;

        public:
        void setLight(std::shared_ptr<Triangle> triangle) {
            areas[triangle->getMaterial().getName()] += triangle->getSize();
            lights.push_back(triangle);
        }

        std::pair<Vec3<float>, float> sampleLight(const std::shared_ptr<BVH>& scene, const Vec3<float>& p) {
            size_t idx = randInt(lights.size());
            Vec3<float> pp = lights[idx]->getRandomPoint();
            
            Vec3<float> wo = pp - p;
            float pdf = 0.f;

            Ray ray(p, wo);
            HitResult res;
            scene->hit(ray, res);
            
            if (!res.isHit || res.id != lights[idx]->getId()) {
                wo = Vec3<float>(0, 0, 0);
            } else {
                float area = lights[idx]->getSize();
                pdf = 1 / area;
            }

            return {wo, pdf};
        }
    };
} // namespace spt


#endif