#ifndef SPT_SCENE_HPP
#define SPT_SCENE_HPP

#include "BVH.hpp"
#include "Camera.hpp"
#include "Hittable.hpp"
#include "Light.hpp"
#include "Material.hpp"
#include "Renderable.hpp"
#include "Sphere.hpp"
#include "Triangle.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace spt {

class Scene {
   public:
    Scene()  = default;
    ~Scene() = default;

    void init(const std::filesystem::path& path);
    void destroy();

    std::shared_ptr<Triangle> loadTriangle(int tid, int mid, const tinyobj::attrib_t& attrib, const tinyobj::shape_t& shape);
    std::shared_ptr<Material> loadMaterial(const std::filesystem::path& mtldir, const tinyobj::material_t& material);
    std::shared_ptr<Light> sampleLight(bool delta) const;
    std::shared_ptr<Light> findLight(int objID) const {
        if (m_ids.count(objID) == 0) { return nullptr; }
        int lightID = m_ids.at(objID);
        return m_lights[lightID];
    }
    float calLightProb(std::shared_ptr<Light> light) const {
        if (light == nullptr) { throw std::runtime_error("Scene::calLightProb: light is nullptr"); }
        if (light->isDelta()) {
            return 1.0f / (m_lights.size() - m_cdf.size());
        } else {
            return light->getObjectArea() / m_cdf.back();
        }
    }

    std::shared_ptr<BVH> getBVH() const { return m_bvh; }
    std::shared_ptr<Camera> getCamera() const { return m_camera; }
    const std::vector<std::shared_ptr<Light>>& getLights() const { return m_lights; }

   private:
    // object hierarchy
    std::shared_ptr<BVH> m_bvh;
    std::vector<std::shared_ptr<Renderable>> m_objects;
    std::unordered_map<std::string, std::weak_ptr<Material>> m_materials;

    // camera and light
    std::shared_ptr<Camera> m_camera;
    std::vector<std::shared_ptr<Light>> m_lights;

    // non-delta lights sampling concerned info
    std::vector<float> m_cdf;           // cumulative distribution function for non-delta lights sampling
    std::unordered_map<int, int> m_ids; // object ID to light ID mappings
};

}; // namespace spt

#endif