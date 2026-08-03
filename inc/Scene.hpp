#ifndef SPT_SCENE_HPP
#define SPT_SCENE_HPP

#include "BVH.hpp"
#include "Camera.hpp"
#include "DES.hpp"
#include "Emitter.hpp"
#include "Material.hpp"
#include "Shape.hpp"

namespace spt {

class Scene {
   public:
    Scene() {}
    Scene(const std::filesystem::path& path, int max_leaf_size) { init(path, max_leaf_size); }
    ~Scene() { clear(); }

    /**
     * @brief Initialize the scene from a json file.
     * 
     * @param path Path to the scene file.
     * @param max_leaf_size Maximum node number for a leaf node in the BVH.
     */
    void init(const std::filesystem::path& path, int max_leaf_size);
    /**
     * @brief Clear/Reset the scene.
     */
    void clear();

    std::span<std::shared_ptr<Primitive>> loadPrimitives(const std::filesystem::path& obj_path, const std::filesystem::path& mtl_dir);
    std::span<std::shared_ptr<Primitive>> loadPrimitives(const std::vector<int>& spe_ids, const std::vector<int>& mtl_ids);
    std::span<std::shared_ptr<Shape>> loadShapes(const std::filesystem::path& obj_path, const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes);
    std::span<std::shared_ptr<Material>> loadMaterials(const std::filesystem::path& mtl_dir, const std::vector<tinyobj::material_t>& materials);

    std::shared_ptr<BVH> getBVH() const { return m_bvh; }
    std::shared_ptr<DES> getDES() const { return m_des; }
    std::shared_ptr<Camera> getCamera() const { return m_camera; }
    std::shared_ptr<Emitter> getEmitter(int id) const {
        if (id < 0 || id >= m_emitters.size()) { throw std::out_of_range("Scene::getEmitter: Invalid emitter id"); }
        return m_emitters[id];
    }
    std::shared_ptr<Shape> getShape(int id) const {
        if (id < 0 || id >= m_shapes.size()) { throw std::out_of_range("Scene::getShape: Invalid shape id"); }
        return m_shapes[id];
    }
    std::shared_ptr<Material> getMaterial(int id) const {
        if (id < 0 || id >= m_materials.size()) { throw std::out_of_range("Scene::getMaterial: Invalid material id"); }
        return m_materials[id];
    }
    std::shared_ptr<Primitive> getPrimitive(int id) const {
        if (id < 0 || id >= m_primitives.size()) { throw std::out_of_range("Scene::getPrimitive: Invalid primitive id"); }
        return m_primitives[id];
    }
    const std::vector<std::shared_ptr<Primitive>>& getDeltaPrimitives() const { return m_delta_primitives; }

   private:
    std::shared_ptr<BVH> m_bvh       = nullptr;
    std::shared_ptr<DES> m_des       = nullptr;
    std::shared_ptr<Camera> m_camera = nullptr;

    std::vector<std::shared_ptr<Emitter>> m_emitters;
    std::vector<std::shared_ptr<Shape>> m_shapes;
    std::vector<std::shared_ptr<Material>> m_materials;
    std::vector<std::shared_ptr<Medium>> m_mediums;
    std::vector<std::shared_ptr<Primitive>> m_primitives;
    std::vector<std::shared_ptr<Primitive>> m_delta_primitives;
};

}; // namespace spt

#endif