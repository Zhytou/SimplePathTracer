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
        std::map<std::string, ulong> mtlnames; // mtlname -> group index
        std::vector<std::vector<ulong>> groups; // group index -> light index
        std::vector<float> areas; // group index -> group area sum

        public:
        void setLight(std::shared_ptr<Triangle> triangle) {
            // basic info
            Material mtl = triangle->getMaterial();
            assert(mtl.isEmissive());
            std::string name = mtl.getName();
            float area = triangle->getSize();

            // group based on mtl
            if (mtlnames.find(name) == mtlnames.end()) {
                mtlnames[name] = groups.size();
                groups.push_back({lights.size()});
                areas.push_back(area);
            } else {
                int gidx = mtlnames[name];
                groups[gidx].push_back(lights.size());
                areas[gidx] += area;
            }

            lights.push_back(triangle);
        }

        std::vector<std::pair<Vec3<float>, float>> sampleAllLights(const std::shared_ptr<BVH>& scene, const Vec3<float>& p) {
            std::vector<std::pair<Vec3<float>, float>> ret;
            
            for (int gidx = 0; gidx < groups.size(); gidx++) {
                int lidx = groups[gidx][rand(groups[gidx].size() - 1)];
                
                Vec3<float> pp = lights[lidx]->getRandomPoint();
                Vec3<float> dir = normalize(pp - p);
                float pdf = 0.f;

                Ray ray(p, dir);
                HitResult res;
                scene->hit(ray, res);
                
                if (!res.hit || res.material.getName() != lights[lidx]->getMaterial().getName()) {
                    dir = Vec3<float>(0, 0, 0);
                } else {
                    float area = areas[gidx];
                    pdf = 1 / area;
                }

                ret.emplace_back(dir, pdf);
            }
            return ret;
        }

        std::pair<Vec3<float>, float> sampleLight(const std::shared_ptr<BVH>& scene, const Vec3<float>& p) {
            ulong gidx = rand(groups.size() - 1);
            ulong lidx = groups[gidx][rand(groups[gidx].size() - 1)];
            
            Vec3<float> pp = lights[lidx]->getRandomPoint();
            Vec3<float> dir = normalize(pp - p);
            float pdf = 0.f;

            Ray ray(p, dir);
            HitResult res;
            scene->hit(ray, res);
            
            if (!res.hit || res.material.getName() != lights[lidx]->getMaterial().getName()) {
                dir = Vec3<float>(0, 0, 0);
            } else {
                float area = areas[gidx];
                pdf = 1 / area;
            }

            return {dir, pdf};
        }
    };
} // namespace spt


#endif