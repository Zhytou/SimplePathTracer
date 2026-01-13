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
        std::vector<float> cprobs; // group index -> cumulative probabilty range

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

        // set light cumulative distribution function
        void setCDF() {
            cprobs.resize(areas.size());
            std::partial_sum(areas.begin(), areas.end(), cprobs.begin());

            float sum = std::accumulate(areas.begin(), areas.end(), 0);
            for (float &prob : cprobs) {
                prob /= sum;
            }
        }

        // get random light group index based on cprobs
        ulong getGroup() {
            float prob = rand(1.f);
            for (ulong gidx = 0; gidx < groups.size(); gidx++) {
                if (cprobs[gidx] > prob) {
                    return gidx;
                }
            }
            return groups.size() - 1;
        }

        std::tuple<Vec3<float>, Vec3<float>, float> sample(const std::shared_ptr<BVH>& scene, const Vec3<float>& p) {
            ulong gidx = getGroup();
            ulong lidx = groups[gidx][rand(groups[gidx].size() - 1)];
            
            Vec3<float> n = lights[lidx]->getNormal();
            Vec3<float> pp = lights[lidx]->getRandomPoint();
            Vec3<float> lum(0.f, 0.f, 0.f);
            Vec3<float> dir = normalize(pp - p);
            float pdf = 0.f;
            float cos = std::max(dot(dir, -n), 0.f);

            Ray ray(p, dir);
            HitResult res;
            scene->hit(ray, res);
            
            if (res.hit && res.id == lights[lidx]->getId()) {
                float nom = res.distance * res.distance;
                float denom = std::max(areas[gidx] * cos, 0.00001f);
                lum = lights[lidx]->getMaterial().getEmission();
                pdf = nom / denom;
            }

            return {lum, dir, pdf};
        }

        float pdf(const std::shared_ptr<BVH>& scene, const Ray& ray) {
            HitResult res;
            scene->hit(ray, res);

            float pdf = 0.f;
            Material mtl = res.material;
            Vec3<float> n = res.normal;
            Vec3<float> dir = ray.getDirection();
            float cos = std::max(dot(dir, -n), 0.f);

            if (res.hit && mtl.isEmissive()) {
                ulong gidx = mtlnames[mtl.getName()];
                float nom = res.distance * res.distance;
                float denom = std::max(areas[gidx] * cos, 0.00001f);
                pdf = nom / denom;
            }

            return pdf;
        }
    };
} // namespace spt


#endif