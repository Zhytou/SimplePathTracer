#ifndef SRE_LIGHT_HPP
#define SRE_LIGHT_HPP

#include<vector>
#include<map>
#include<memory>

#include "Triangle.hpp"
#include "BVH.hpp"

namespace spt
{
    class Light {
        std::vector<std::shared_ptr<Triangle>> lights;
        std::map<std::string, float> areas;

        public:
        void setLight(std::shared_ptr<Triangle> triangle) {
            assert(triangle->getMaterial().isEmissive());
            areas[triangle->getMaterial().getName()] += triangle->getSize();
            lights.push_back(triangle);
        }

        std::pair<Vec3<float>, float> sampleLight(const std::shared_ptr<BVH>& scene, const Vec3<float>& p) {
            size_t idx = rand<int>(lights.size()-1);
            assert(lights[idx] != nullptr);
            Vec3<float> pp = lights[idx]->getRandomPoint();
            
            Vec3<float> dir = normalize(pp - p);
            float pdf = 0.f;

            Ray ray(p, dir);
            HitResult res;
            scene->hit(ray, res);
            
            if (!res.isHit || res.material.getName() != lights[idx]->getMaterial().getName()) {
                dir = Vec3<float>(0, 0, 0);
            } else {
                std::string name = lights[idx]->getMaterial().getName();
                float area = areas[name];
                pdf = 1 / area;
            }

            return {dir, pdf};
        }
    };
} // namespace spt


#endif