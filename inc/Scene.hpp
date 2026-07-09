#ifndef SPT_SCENE_HPP
#define SPT_SCENE_HPP

#include "BVH.hpp"
#include "Camera.hpp"
#include "Light.hpp"
#include "Material.hpp"
#include "Triangle.hpp"

#include <filesystem>
#include <memory>
#include <vector>

namespace spt {

class Scene {
   public:
    Scene()  = default;
    ~Scene() = default;

    void init(const std::filesystem::path& path);
    void destroy();

    // Load triangle from obj file
    //@param tid index of current triangle in the shape
    //@param mid index of current material in obj file
    //@param attrib attributes of the triangle
    //@param shape shape of the triangle
    //@return: shared pointer to the loaded triangle
    std::shared_ptr<Triangle> loadTriangle(int tid, int mid, const tinyobj::attrib_t& attrib, const tinyobj::shape_t& shape);
    // Load material from obj file
    //@param oid index of current obj file in the json configuration
    //@param mtldir directory of the material file in the obj file
    //@param material material of the triangle
    //@return: shared pointer to the loaded material
    std::shared_ptr<Material> loadMaterial(const std::filesystem::path& mtldir, const tinyobj::material_t& material);

    std::shared_ptr<BVH> getBVH() const { return m_bvh; }
    std::shared_ptr<Camera> getCamera() const { return m_camera; }
    std::shared_ptr<Light> getLight() const { return m_light; }
    const std::vector<std::vector<std::shared_ptr<Triangle>>> getTriangles() const { return m_triangles; }
    const std::vector<std::vector<std::shared_ptr<Material>>> getMaterials() const { return m_materials; }

   private:
    std::shared_ptr<BVH> m_bvh;
    std::shared_ptr<Camera> m_camera;
    std::shared_ptr<Light> m_light;
    std::vector<std::vector<std::shared_ptr<Triangle>>> m_triangles;
    std::vector<std::vector<std::shared_ptr<Material>>> m_materials;
};

}; // namespace spt

#endif