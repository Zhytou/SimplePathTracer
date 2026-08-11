#include "Scene.hpp"

#include <rapidjson/document.h>

#include "BoxLogger.hpp"
#include "Image.hpp"
#include "Material.hpp"
#include "Medium.hpp"
#include "Sphere.hpp"
#include "Triangle.hpp"

namespace spt {

namespace fs = std::filesystem;

void Scene::init(const fs::path& path, int max_leaf_size) {
    // 0. Read config file
    std::ifstream file(path);
    if (!file.is_open()) { throw std::runtime_error(std::format("Scene::init: Invalid config file path {}", path.string())); }
    std::stringstream buffer;
    buffer << file.rdbuf();

    // 1. Parse config file and define lambda functions
    rapidjson::Document doc;
    if (doc.Parse(buffer.str().c_str()).HasParseError()) { throw std::runtime_error("Scene::initialize: Error parsing JSON"); }
    auto getVec2 = [](const rapidjson::Value& arr) -> Vec2<float> {
        if (!arr.IsArray() || arr.Size() != 2 || !arr[0].IsNumber() || !arr[1].IsNumber()) { throw std::runtime_error("Scene::init:getVec2: Invalid array size or element type"); }

        return Vec2<float>{arr[0].GetFloat(), arr[1].GetFloat()};
    };
    auto getVec3 = [](const rapidjson::Value& arr) -> Vec3<float> {
        if (!arr.IsArray() || arr.Size() != 3 || !arr[0].IsNumber() || !arr[1].IsNumber() || !arr[2].IsNumber()) { throw std::runtime_error("Scene::init:getVec3: Invalid array size or element type"); }

        return Vec3<float>{arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat()};
    };
    auto getArr = [](const rapidjson::Value& arr) -> std::vector<int> {
        if (!arr.IsArray()) { throw std::runtime_error("Scene::init:getArr: Invalid array size or element type"); }

        std::vector<int> res;
        for (int i = 0; i < arr.Size(); i++) {
            if (!arr[i].IsNumber()) { throw std::runtime_error("Scene::init:getArr: Invalid array element type"); }
            res.push_back(arr[i].GetInt());
        }
        return res;
    };

    // 2. Load global materials
    if (doc.HasMember("material")) {
        for (int i = 0; i < (doc["material"].HasMember("diffuse") ? doc["material"]["diffuse"].Size() : 0); i++) {
            auto& mtl_doc                            = doc["material"]["diffuse"][i];
            std::string name                         = mtl_doc.HasMember("name") ? mtl_doc["name"].GetString() : std::format("global_material_{}", i);
            fs::path dir                             = mtl_doc.HasMember("directory") ? fs::path(mtl_doc["directory"].GetString()) : fs::path();
            Vec3<float> albedo                       = mtl_doc.HasMember("albedo") ? getVec3(mtl_doc["albedo"]) : Vec3<float>(0.f);
            fs::path albedo_texpath                  = mtl_doc.HasMember("albedo_texture") ? dir / mtl_doc["albedo_texture"].GetString() : dir;
            std::shared_ptr<Image<float>> albedo_map = fs::is_regular_file(albedo_texpath) ? Image<float>::read(albedo_texpath, 0, true) : nullptr;
            auto mtl                                 = std::make_shared<Diffuse>(m_materials.size(), name, albedo, albedo_map);
            m_materials.push_back(mtl);
        }

        for (int i = 0; i < (doc["material"].HasMember("mirror") ? doc["material"]["mirror"].Size() : 0); i++) {
            auto& mtl_doc    = doc["material"]["mirror"][i];
            std::string name = mtl_doc.HasMember("name") ? mtl_doc["name"].GetString() : std::format("global_material_{}", i);
            auto mtl         = std::make_shared<Mirror>(m_materials.size(), name);
            m_materials.push_back(mtl);
        }

        for (int i = 0; i < (doc["material"].HasMember("dielectric") ? doc["material"]["dielectric"].Size() : 0); i++) {
            auto& mtl_doc    = doc["material"]["dielectric"][i];
            std::string name = mtl_doc.HasMember("name") ? mtl_doc["name"].GetString() : std::format("global_material_{}", i);
            float int_ior    = mtl_doc.HasMember("interior_ior") ? mtl_doc["interior_ior"].GetFloat() : 0.f;
            float ext_ior    = mtl_doc.HasMember("exterior_ior") ? mtl_doc["exterior_ior"].GetFloat() : 0.f;
            auto mtl         = std::make_shared<Dielectric>(m_materials.size(), name, int_ior, ext_ior);
            m_materials.push_back(mtl);
        }

        for (int i = 0; i < (doc["material"].HasMember("microfacet_conductor") ? doc["material"]["microfacet_conductor"].Size() : 0); i++) {
            auto& mtl_doc                               = doc["material"]["microfacet_conductor"][i];
            std::string name                            = mtl_doc.HasMember("name") ? mtl_doc["name"].GetString() : std::format("global_material_{}", i);
            fs::path dir                                = mtl_doc.HasMember("directory") ? fs::path(mtl_doc["directory"].GetString()) : fs::path();
            float real_ior                              = mtl_doc.HasMember("real_ior") ? mtl_doc["real_ior"].GetFloat() : 0.f;
            float imag_ior                              = mtl_doc.HasMember("imag_ior") ? mtl_doc["imag_ior"].GetFloat() : 0.f;
            float roughness                             = mtl_doc.HasMember("roughness") ? mtl_doc["roughness"].GetFloat() : 0.f;
            fs::path roughness_texpath                  = mtl_doc.HasMember("roughness_texture") ? dir / mtl_doc["roughness_texture"].GetString() : dir;
            std::shared_ptr<Image<float>> roughness_map = fs::is_regular_file(roughness_texpath) ? Image<float>::read(roughness_texpath, 0, true) : nullptr;
            auto mtl                                    = std::make_shared<MicrofacetConductor>(m_materials.size(), name, real_ior, imag_ior, roughness, roughness_map);
            m_materials.push_back(mtl);
        }

        for (int i = 0; i < (doc["material"].HasMember("microfacet_dielectric") ? doc["material"]["microfacet_dielectric"].Size() : 0); i++) {
            auto& mtl_doc                               = doc["material"]["microfacet_dielectric"][i];
            std::string name                            = mtl_doc.HasMember("name") ? mtl_doc["name"].GetString() : std::format("global_material_{}", i);
            fs::path dir                                = mtl_doc.HasMember("directory") ? fs::path(mtl_doc["directory"].GetString()) : fs::path();
            float int_ior                               = mtl_doc.HasMember("interior_ior") ? mtl_doc["interior_ior"].GetFloat() : 0.f;
            float ext_ior                               = mtl_doc.HasMember("exterior_ior") ? mtl_doc["exterior_ior"].GetFloat() : 0.f;
            float roughness                             = mtl_doc.HasMember("roughness") ? mtl_doc["roughness"].GetFloat() : 0.f;
            fs::path roughness_texpath                  = mtl_doc.HasMember("roughness_texture") ? dir / mtl_doc["roughness_texture"].GetString() : dir;
            std::shared_ptr<Image<float>> roughness_map = fs::is_regular_file(roughness_texpath) ? Image<float>::read(roughness_texpath, 0, true) : nullptr;
            auto mtl                                    = std::make_shared<MicrofacetDielectric>(m_materials.size(), name, int_ior, ext_ior, roughness, roughness_map);
            m_materials.push_back(mtl);
        }
    }

    // 3. Load global mediums
    if (doc.HasMember("medium")) {
        for (int i = 0; i < doc["medium"].Size(); i++) {
            auto& med_doc          = doc["medium"][i];
            std::string name       = med_doc.HasMember("name") ? med_doc["name"].GetString() : "global_medium_" + std::to_string(i);
            Vec3<float> absorption = med_doc.HasMember("absorption") ? getVec3(med_doc["absorption"]) : Vec3<float>(0.f);
            Vec3<float> scattering = med_doc.HasMember("scattering") ? getVec3(med_doc["scattering"]) : Vec3<float>(0.f);
            Vec3<float> extinction = med_doc.HasMember("extinction") ? getVec3(med_doc["extinction"]) : Vec3<float>(0.f);

            auto med = std::make_shared<Medium>(m_mediums.size());
            med->setName(name);
            med->setAbsorption(absorption);
            med->setScattering(scattering);
            med->setExtinction(extinction);
            m_mediums.push_back(med);
        }
    }

    // 4. Load global shapes
    if (doc.HasMember("shape")) {
        for (int i = 0; i < (doc["shape"].HasMember("sphere") ? doc["shape"]["sphere"].Size() : 0); i++) {
            auto& spe_doc      = doc["shape"]["sphere"][i];
            Vec3<float> center = getVec3(spe_doc["center"]);
            float radius       = spe_doc["radius"].GetFloat();
            auto spe           = std::make_shared<Sphere>(m_shapes.size(), center, radius);
            m_shapes.push_back(spe);
        }
    }

    // 5. Load primitives form .obj models or self-defined geometry shapes
    if (doc.HasMember("primitive")) {
        for (int i = 0; i < doc["primitive"].Size(); i++) {
            auto& prm_doc = doc["primitive"][i];
            std::span<std::shared_ptr<Primitive>> prms;
            // 5.1 Initialize primitive list
            if (prm_doc.HasMember("object_path")) {
                fs::path obj_path = prm_doc["object_path"].GetString();
                fs::path mtl_dir  = prm_doc.HasMember("material_directory") ? fs::path(prm_doc["material_directory"].GetString()) : fs::path();
                int mtl_id        = prm_doc.HasMember("material_id") ? prm_doc["material_id"].GetInt() : -1;
                prms              = loadPrimitives(obj_path, mtl_dir, mtl_id);
            } else {
                auto spe_ids = getArr(prm_doc["shape_ids"]);
                auto mtl_ids = getArr(prm_doc["material_ids"]);
                prms         = loadPrimitives(spe_ids, mtl_ids);
            }

            // 5.2 Load interior and exterior medium
            std::shared_ptr<Medium> int_med = nullptr, ext_med = nullptr;
            if (prm_doc.HasMember("interior_medium_id")) {
                int int_med_id = prm_doc["interior_medium_id"].GetInt();
                assert(int_med_id >= 0 && int_med_id < m_mediums.size());
                int_med = m_mediums[int_med_id];
            }
            if (prm_doc.HasMember("exterior_medium_id")) {
                int ext_med_id = prm_doc["exterior_medium_id"].GetInt();
                assert(ext_med_id >= 0 && ext_med_id < m_mediums.size());
                ext_med = m_mediums[ext_med_id];
            }

            // 5.3 Load area emitter emission
            bool is_ems     = prm_doc.HasMember("emission");
            Vec3<float> emn = is_ems ? getVec3(prm_doc["emission"]) : Vec3<float>(0.f);

            // 5.4 Load transform matrix
            Mat4x4f tfm = Mat4x4f::eye();
            if (prm_doc.HasMember("transform")) {
                auto& tfm_doc = prm_doc["transform"];
                if (tfm_doc.HasMember("translate")) { tfm = tfm * translate(getVec3(tfm_doc["translate"])); }
                if (tfm_doc.HasMember("rotate")) { tfm = tfm * rotate(getVec3(tfm_doc["rotate"])); }
                if (tfm_doc.HasMember("scale")) { tfm = tfm * scale(getVec3(tfm_doc["scale"])); }
            }

            // 5.5 Set primitives properties
            for (auto prm : prms) {
                auto mtl = prm->getMaterial();
                auto emt = is_ems ? std::make_shared<AreaEmitter>(m_emitters.size(), emn) : nullptr;
                if (emt) {
                    prm->setEmitter(emt);
                    emt->setPrimitive(prm);
                    m_emitters.push_back(emt);
                }
                prm->setInteriorMedium(int_med);
                prm->setExteriorMedium(ext_med);
                prm->setTransform(tfm);
            }
        }
    }

    // 6. Load main camera
    if (doc.HasMember("camera")) {
        bool is_perspective = doc["camera"].HasMember("perspective");
        auto& cam_doc       = is_perspective ? doc["camera"]["perspective"] : doc["camera"]["orthographic"];
        if (is_perspective) {
            m_camera = std::make_shared<PerspectiveCamera>();
        } else {
            m_camera = std::make_shared<OrthographicCamera>();
        }

        int width  = cam_doc["width"].GetInt();
        int height = cam_doc["height"].GetInt();
        float fovy = cam_doc.HasMember("fovy") ? cam_doc["fovy"].GetFloat() : 0.f;
        float fovx = cam_doc.HasMember("fovx") ? cam_doc["fovx"].GetFloat() : 0.f;
        auto eye   = getVec3(cam_doc["eye"]);
        auto tar   = getVec3(cam_doc["target"]);
        auto up    = getVec3(cam_doc["up"]);
        if (fovy == 0.f && fovx != 0.f) {
            fovy = 2 * std::atan(std::tan(fovx * 0.5f * PI / 180.f) * height / width) / PI * 180.f;
        }
        m_camera->setWidth(width);
        m_camera->setHeight(height);
        m_camera->setFovy(fovy);
        m_camera->setEye(eye);
        m_camera->setTarget(tar);
        m_camera->setUp(up);
    }

    // 7. Load light sources
    if (doc.HasMember("light")) {
        // 7.1 Load area light by material name
        if (doc["light"].HasMember("area")) {
            // 7.1.1 Get area light emission
            std::unordered_map<std::string, Vec3<float>> emissions;
            std::unordered_map<std::string, std::vector<std::shared_ptr<Primitive>>> primitives; // emissive primitives
            for (int i = 0; i < doc["light"]["area"].Size(); i++) {
                auto& light_doc   = doc["light"]["area"][i];
                std::string name  = light_doc.HasMember("material_name") ? light_doc["material_name"].GetString() : std::format("light_{}", i);
                Vec3<float> color = light_doc.HasMember("color") ? getVec3(light_doc["color"]) : Vec3<float>(0.f);
                emissions[name]   = color;
            }

            // 7.1.2 Set corresponding primitive's emission
            for (auto& prm : m_primitives) {
                if (prm->getEmitter()) { continue; } // skip if already set emitter
                auto mtl  = prm->getMaterial();
                auto name = mtl->getName();
                if (emissions.count(name)) { primitives[name].push_back(prm); }
            }

            // 7.1.3 Create area emitters
            for (auto [name, color] : emissions) {
                for (auto prm : primitives[name]) {
                    auto emt = std::make_shared<AreaEmitter>(m_emitters.size(), color);
                    emt->setPrimitive(prm);
                    prm->setEmitter(emt);
                    m_emitters.push_back(emt);
                }
            }
        }
    }

    // 8. Set bounding volume hierarchy and direct light sampler
    m_bvh = BVH::create(m_primitives, AABB::create(m_primitives), max_leaf_size, m_primitives.size() > 1000 ? 16 : -1); // init BVH with m_primitives copy, otherwise m_primitives have to reorder by ascending id to keep scene::getPrimitive correct
    m_des = DES::create(m_emitters);
    std::stable_sort(m_primitives.begin(), m_primitives.end(), [](std::shared_ptr<Primitive> prm1, std::shared_ptr<Primitive> prm2) {
        return prm1->getID() < prm2->getID();
    });

    // 9. Print scene info
    {
        constexpr int n     = 10;
        constexpr int k     = 5;
        constexpr int width = 115; // width for box logging

        // 9.0 Lambda functors and temperol variables
        std::stringstream ss;
        auto printPrmInfo = [&ss](std::shared_ptr<Primitive> prm) {
            if (!prm) { throw std::runtime_error("Primitive is null"); }
            auto mtl     = prm->getMaterial();
            auto spe     = prm->getShape();
            auto emt     = prm->getEmitter();
            auto int_med = prm->getInteriorMedium();
            auto ext_med = prm->getExteriorMedium();

            ss << std::left
               << std::setw(10) << prm->getID()
               << std::setw(15) << (spe ? typeid(*spe).name() : "None")
               << std::setw(15) << (mtl ? mtl->getName() : "None")
               << std::setw(15) << (mtl ? typeid(*mtl).name() : "None")
               << std::setw(15) << (emt ? typeid(*emt).name() : "None")
               << std::setw(15) << (int_med ? int_med->getName() : "None")
               << std::setw(15) << (ext_med ? ext_med->getName() : "None")
               << std::setw(15) << (prm->isTransformIdentity() ? "Yes" : "No")
               << '\n';
        };
        auto printMtlInfo = [&ss](std::shared_ptr<Material> mtl) {
            if (!mtl) { throw std::runtime_error("Material is null"); }
            Vec2<float> uv(0.5f);
            auto name = mtl->getName();
            auto type = typeid(*mtl).name();
            if (name.size() >= 25) {
                name = name.substr(0, 22) + "...";
            }
            ss << std::left << std::fixed << std::setprecision(2)
               << std::setw(25) << name
               << std::setw(25) << type
               << '\n';
        };

        // 9.1 Print camera configuration
        BOX_LOG("CAMERA SETTINGS", width)
            << std::left << std::setw(15) << " - Eye:    " << std::setw(25) << m_camera->getEye() << std::setw(15) << " - Target: " << std::setw(25) << m_camera->getTarget() << '\n'
            << std::setw(15) << " - Fovy: " << std::setw(25) << m_camera->getFovy() << std::setw(15) << " - Focus: " << std::setw(25) << m_camera->getFocus() << '\n';

        // 9.2 Print primitive list
        if (m_primitives.size() < 100) {
            for (const auto& prm : m_primitives) { printPrmInfo(prm); }
        } else {
            for (const auto& prm : m_primitives | std::views::take(k)) { printPrmInfo(prm); }
            ss << "   ......\n";
            for (const auto& prm : m_primitives | std::views::drop(m_primitives.size() - k)) { printPrmInfo(prm); }
        }
        BOX_LOG("RENDERABLE PRIMITIVES", width)
            << std::left
            << std::setw(10) << "ID"
            << std::setw(15) << "Shape"
            << std::setw(15) << "Material"
            << std::setw(15) << "Material"
            << std::setw(15) << "Emitter"
            << std::setw(15) << "Int Medium"
            << std::setw(15) << "Ext Medium"
            << std::setw(15) << "Transform Identity"
            << '\n'
            << std::string(width - 4, '-') << '\n'
            << ss.rdbuf(); //
    }
}

void Scene::clear() {
    m_emitters.clear();
    m_shapes.clear();
    m_materials.clear();
}

std::span<std::shared_ptr<Primitive>> Scene::loadPrimitives(const std::vector<int>& spe_ids, const std::vector<int>& mtl_ids) {
    int start = m_primitives.size(), count = 0;
    assert(spe_ids.size() == mtl_ids.size());
    for (int i = 0; i < spe_ids.size(); i++) {
        auto sid = spe_ids[i];
        auto mid = mtl_ids[i];
        auto spe = m_shapes[sid];
        auto mtl = mid != -1 ? m_materials[mid] : nullptr;
        auto prm = std::make_shared<Primitive>(m_primitives.size(), spe, mtl);
        m_primitives.push_back(prm);
        count++;
    }

    return std::span<std::shared_ptr<Primitive>>(m_primitives.data() + start, count);
}

std::span<std::shared_ptr<Primitive>> Scene::loadPrimitives(const std::filesystem::path& obj_path, const std::filesystem::path& mtl_dir, int dft_mtl_id) {
    // 1. Load obj file with tinyobj
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, obj_path.c_str(), mtl_dir.c_str(), true)) {
        throw std::runtime_error("Scene::loadModel: " + err);
    }

    // 2. Convert tinyobj::shape_t into spt::Shape and cache the result
    int spe_start = m_shapes.size(), spe_count = 0;
    static std::unordered_map<std::string, std::pair<int, int>> spe_cache;
    if (spe_cache.count(obj_path.string())) {
        spe_start = spe_cache[obj_path.string()].first;
        spe_count = spe_cache[obj_path.string()].second;
    } else {
        for (const auto& shape : shapes) {
            for (int i = 0; i < shape.mesh.material_ids.size(); ++i) { // i is face index
                // 2.0 Initialize vertex attributes
                bool vn = true, vt = true;
                std::array<Vec3<float>, 3> vertex, normal;
                std::array<Vec2<float>, 3> uv;

                // 2.1 Get vertex info from tinyobj::attrib_t and tinyobj::shape_t
                for (int j = 0; j < 3; ++j) { // j is vertex index
                    tinyobj::index_t index = shape.mesh.indices[3 * i + j];
                    int v = index.vertex_index, n = index.normal_index, t = index.texcoord_index;

                    vertex[j] = Vec3<float>(attrib.vertices[3 * v], attrib.vertices[3 * v + 1], attrib.vertices[3 * v + 2]);
                    if (n >= 0) { normal[j] = Vec3<float>(attrib.normals[3 * n], attrib.normals[3 * n + 1], attrib.normals[3 * n + 2]); }
                    if (t >= 0) { uv[j] = Vec2<float>(attrib.texcoords[2 * t], attrib.texcoords[2 * t + 1]); }
                    vn = (n >= 0) && vn;
                    vt = (t >= 0) && vt;
                }

                // 2.2 Get face normal if normal is not available
                if (!vn) {
                    Vec3<float> edge1 = vertex[1] - vertex[0], edge2 = vertex[2] - vertex[0];
                    Vec3<float> fnormal = normalize(cross(edge1, edge2));
                    for (int j = 0; j < 3; ++j) { normal[j] = fnormal; }
                }

                // 2.3 Get texture coordinates if uv is not available
                if (!vt) {
                    for (int j = 0; j < 3; ++j) {
                        Vec3<float> d = normalize(vertex[j]);
                        float u       = 0.5f + (std::atan2(d.z, d.x) / (2.0f * PI));
                        float v       = 0.5f - (std::asin(d.y) / PI);
                        uv[j]         = Vec2<float>(u, v);
                    }
                }

                // 2.4 Create triangle shape
                auto spe = std::make_shared<Triangle>(m_shapes.size(), vertex, normal, uv);
                m_shapes.push_back(spe);
                spe_count++;
            }
        }
        spe_cache[obj_path.string()] = std::make_pair(spe_start, spe_count);
    }
    auto spes = std::span<std::shared_ptr<Shape>>(m_shapes.data() + spe_start, spe_count);

    // 3. Convert tinyobj::shape_t into spt::Shape and cache the result
    int mtl_start = m_materials.size(), mtl_count = 0;
    static std::unordered_map<std::string, std::pair<int, int>> mtl_cache;
    if (mtl_cache.count(obj_path.string())) {
        mtl_start = mtl_cache[obj_path.string()].first;
        mtl_count = mtl_cache[obj_path.string()].second;
    } else {
        for (const auto& material : materials) {
            auto name       = material.name;
            auto albedo     = Vec3<float>{material.diffuse[0], material.diffuse[1], material.diffuse[2]};
            auto albedo_map = fs::is_regular_file(mtl_dir / material.diffuse_texname) ? Image<float>::read(mtl_dir / material.diffuse_texname) : nullptr;

            auto mtl = std::make_shared<Diffuse>(m_materials.size(), name, albedo, albedo_map);
            mtl_count++;
            m_materials.push_back(mtl);
        }
        mtl_cache[obj_path.string()] = std::make_pair(mtl_start, mtl_count);
    }
    auto mtls    = std::span<std::shared_ptr<Material>>(m_materials.data() + mtl_start, mtl_count);
    auto dft_mtl = dft_mtl_id != -1 ? m_materials[dft_mtl_id] : nullptr;

    // 4. Create primitives
    int prm_start = m_primitives.size(), prm_count = 0;
    for (const auto& shape : shapes) {
        for (auto mtl_id : shape.mesh.material_ids) {
            auto spe = spes[prm_count];
            auto mtl = mtl_id == -1 ? dft_mtl : mtls[mtl_id];
            auto prm = std::make_shared<Primitive>(m_primitives.size(), spe, mtl);
            m_primitives.push_back(prm);
            prm_count++;
        }
    }

    // 5. Return primitives view
    return std::span<std::shared_ptr<Primitive>>(m_primitives.data() + prm_start, prm_count);
}

}; // namespace spt
