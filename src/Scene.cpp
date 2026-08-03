#include "Scene.hpp"

#include <rapidjson/document.h>

#include "BoxLogger.hpp"
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
        for (int i = 0; i < doc["material"].Size(); i++) {
            auto& mtl_doc              = doc["material"][i];
            std::string name           = mtl_doc.HasMember("name") ? mtl_doc["name"].GetString() : "global_material_" + std::to_string(i);
            fs::path dir               = mtl_doc.HasMember("directory") ? fs::path(mtl_doc["directory"].GetString()) : fs::path();
            Vec3<float> albedo         = mtl_doc.HasMember("albedo") ? getVec3(mtl_doc["albedo"]) : Vec3<float>(0.f);
            float metallic             = mtl_doc.HasMember("metallic") ? mtl_doc["metallic"].GetFloat() : 0.f;
            float roughness            = mtl_doc.HasMember("roughness") ? mtl_doc["roughness"].GetFloat() : 0.f;
            fs::path albedo_texpath    = mtl_doc.HasMember("albedo_texture") ? dir / mtl_doc["albedo_texture"].GetString() : dir;
            fs::path roughness_texpath = mtl_doc.HasMember("roughness_texture") ? dir / mtl_doc["roughness_texture"].GetString() : dir;
            fs::path metallic_texpath  = mtl_doc.HasMember("metallic_texture") ? dir / mtl_doc["metallic_texture"].GetString() : dir;

            auto mtl = std::make_shared<Material>(m_materials.size());
            mtl->setName(name);
            mtl->setAlbedo(albedo);
            mtl->setRoughness(roughness);
            mtl->setMetallic(metallic);
            m_materials.push_back(mtl);
        }
    }

    // 3. Load global mediums
    if (doc.HasMember("medium")) {
        for (int i = 0; i < doc["medium"].Size(); i++) {
            auto& med_doc          = doc["medium"][i];
            std::string name       = med_doc.HasMember("name") ? med_doc["name"].GetString() : "global_medium_" + std::to_string(i);
            float ior              = med_doc.HasMember("ior") ? med_doc["ior"].GetFloat() : 1.f;
            Vec3<float> absorption = med_doc.HasMember("absorption") ? getVec3(med_doc["absorption"]) : Vec3<float>(0.f);

            auto med = std::make_shared<Medium>(m_mediums.size());
            med->setName(name);
            med->setIOR(ior);
            med->setAbsorption(absorption);
            m_mediums.push_back(med);
        }
    }
    // Set air as default medium
    if (m_mediums.empty()) {
        auto air = std::make_shared<Medium>(m_mediums.size());
        air->setName("air");
        air->setIOR(1.f);
        air->setAbsorption(Vec3<float>(0.f));
        m_mediums.push_back(air);
    }

    // 4. Load global shapes
    if (doc.HasMember("shape")) {
        for (int i = 0; i < doc["shape"].HasMember("sphere") ? doc["shape"]["sphere"].Size() : 0; i++) {
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
                // 5.1.1 Initialize obj file path and mtl file base dir
                fs::path obj_path    = prm_doc["object_path"].GetString();
                fs::path mtl_dir     = prm_doc.HasMember("material_directory") ? prm_doc["material_directory"].GetString() : obj_path.parent_path();
                std::string obj_name = obj_path.stem().string();

                // 5.1.2 Load primitives with tinyobjloader
                prms = loadPrimitives(obj_path, mtl_dir);
            } else {
                // 5.1.3 Load self-defined geometry primitives with global shapes/materials ids
                auto shape_ids = getArr(prm_doc["shape_ids"]);
                auto mat_ids   = getArr(prm_doc["material_ids"]);
                prms           = loadPrimitives(shape_ids, mat_ids);
            }

            // 5.2 Add default material to list tail if specified
            std::shared_ptr<Material> dft_mtl = nullptr;
            if (prm_doc.HasMember("default_material_id")) {
                int dft_mtl_id = prm_doc["default_material_id"].GetInt();
                assert(dft_mtl_id >= 0 && dft_mtl_id < m_materials.size());
                dft_mtl = m_materials[dft_mtl_id];
            }

            // 5.3 Load interior and exterior media
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

            // 5.4 Load area emitter emission
            bool is_ems     = prm_doc.HasMember("emission");
            Vec3<float> emn = is_ems ? getVec3(prm_doc["emission"]) : Vec3<float>(0.f);

            // 5.5 Load transform matrix
            Mat4x4f tfm = Mat4x4f::eye();
            if (prm_doc.HasMember("transform")) {
                auto& tfm_doc = prm_doc["transform"];
                if (tfm_doc.HasMember("translate")) { tfm = tfm * translate(getVec3(tfm_doc["translate"])); }
                if (tfm_doc.HasMember("rotate")) { tfm = tfm * rotate(getVec3(tfm_doc["rotate"])); }
                if (tfm_doc.HasMember("scale")) { tfm = tfm * scale(getVec3(tfm_doc["scale"])); }
            }

            // 5.6 Set primitives properties
            for (auto prm : prms) {
                auto mtl = prm->getMaterial();
                if (mtl == nullptr && dft_mtl != nullptr) { prm->setMaterial(dft_mtl); }
                auto emt = is_ems ? std::make_shared<AreaEmitter>(m_emitters.size(), emn) : nullptr;
                if (emt) {
                    prm->setEmitter(emt);
                    emt->setPrimitive(prm);
                    m_emitters.push_back(emt);
                }
                assert(mtl != nullptr || dft_mtl != nullptr || emt != nullptr); // each primitive must have a material or emitter set
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
                std::string name  = light_doc.HasMember("material_name") ? light_doc["material_name"].GetString() : std::format("light{}", i);
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

        // 7.2 Load point light
        for (int i = 0; i < doc["light"].HasMember("point") ? doc["light"]["point"].Size() : 0; i++) {
            auto& light_doc      = doc["light"]["point"][i];
            Vec3<float> position = light_doc.HasMember("position") ? getVec3(light_doc["position"]) : Vec3<float>(0.f);
            Vec3<float> color    = light_doc.HasMember("color") ? getVec3(light_doc["color"]) : Vec3<float>(0.f);
            m_emitters.push_back(std::make_shared<PointEmitter>(m_emitters.size(), color, position));
        }

        // 7.3 Load directional light
        for (int i = 0; i < doc["light"].HasMember("directional") ? doc["light"]["directional"].Size() : 0; i++) {
            auto& light_doc       = doc["light"]["directional"][i];
            Vec3<float> direction = light_doc.HasMember("direction") ? getVec3(light_doc["direction"]) : Vec3<float>(0.f);
            Vec3<float> color     = light_doc.HasMember("color") ? getVec3(light_doc["color"]) : Vec3<float>(0.f);
            m_emitters.push_back(std::make_shared<DirectionalEmitter>(m_emitters.size(), color, direction));
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
            auto mtl = prm->getMaterial();
            auto spe = prm->getShape();
            auto emt = prm->getEmitter();

            ss << std::left
               << std::setw(15) << prm->getID()
               << std::setw(23) << (spe ? typeid(*spe).name() : "None")
               << std::setw(23) << (mtl ? mtl->getName() : "None")
               << std::setw(23) << (emt ? typeid(*emt).name() : "None")
               << '\n';
        };
        auto printMtlInfo = [&ss](std::shared_ptr<Material> mtl) {
            if (!mtl) { throw std::runtime_error("Material is null"); }
            Vec2<float> uv(0.5f);
            auto name          = mtl->getName();
            auto type          = mtl->getTypeStr();
            Vec3<float> albedo = mtl->getAlbedo(uv);
            float roughness    = mtl->getRoughness(uv);
            float metallic     = mtl->getMetallic(uv);
            float ior          = mtl->getIOR();
            if (name.size() >= 15) {
                name = name.substr(0, 10) + "...";
            }
            ss << std::left << std::fixed << std::setprecision(2)
               << std::setw(15) << name
               << std::setw(25) << type
               << std::setw(25) << albedo
               << std::setw(12) << roughness
               << std::setw(12) << metallic
               << std::setw(12) << ior << '\n';
        };

        // 9.1 Print camera configuration
        BOX_LOG("CAMERA SETTINGS", width)
            << std::left << std::setw(15) << " - Eye:    " << std::setw(25) << m_camera->getEye() << std::setw(15) << " - Target: " << std::setw(25) << m_camera->getTarget() << '\n'
            << std::setw(15) << " - Fovy: " << std::setw(25) << m_camera->getFovy() << std::setw(15) << " - Focus: " << std::setw(25) << m_camera->getFocus() << '\n';

        // 9.2 Print primitive list
        // if (m_primitives.size() < n) {
        //     for (const auto& prm : m_primitives) { printPrmInfo(prm); }
        // } else {
        //     for (const auto& prm : m_primitives | std::views::take(k)) { printPrmInfo(prm); }
        //     ss << "   ......\n";
        //     for (const auto& prm : m_primitives | std::views::drop(m_primitives.size() - k)) { printPrmInfo(prm); }
        // }
        for (const auto& prm : m_primitives) { printPrmInfo(prm); }
        BOX_LOG("RENDERABLE PRIMITIVES", width)
            << std::left
            << std::setw(15) << "ID"
            << std::setw(23) << "Shape Name"
            << std::setw(23) << "Material Name"
            << std::setw(23) << "Emitter Name" << '\n'
            << std::string(width - 4, '-') << '\n'
            << ss.rdbuf(); //

        // 9.3 Print materials summary
        for (auto mtl : m_materials) {
            if (!mtl) { continue; }
            printMtlInfo(mtl);
        }
        BOX_LOG("RENDERABLE MATERIALS", width)
            << std::left
            << std::setw(15) << "Name"
            << std::setw(25) << "Material Type"
            << std::setw(25) << "Albedo (R, G, B)"
            << std::setw(12) << "Roughness"
            << std::setw(12) << "Metallic"
            << std::setw(12) << "IOR" << '\n'
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
        if (mtl && mtl->isDelta()) { m_delta_primitives.push_back(prm); }
        m_primitives.push_back(prm);
        count++;
    }

    return std::span<std::shared_ptr<Primitive>>(m_primitives.data() + start, count);
}

std::span<std::shared_ptr<Primitive>> Scene::loadPrimitives(const std::filesystem::path& obj_path, const std::filesystem::path& mtl_dir) {
    // 1. Load obj file with tinyobj
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, obj_path.c_str(), mtl_dir.c_str(), true)) {
        throw std::runtime_error("Scene::loadModel: " + err);
    }

    // 2 Convert tinyobj::shape_t and tinyobj::material_t into spt::Shape and spt::Material
    std::span<std::shared_ptr<Shape>> spes    = loadShapes(obj_path, attrib, shapes);
    std::span<std::shared_ptr<Material>> mtls = loadMaterials(mtl_dir, materials);

    // 3. Create primitives
    int start = m_primitives.size(), count = 0;
    int sid = -1, mid = -1;
    for (int j = 0; j < shapes.size(); j++) {
        for (int k = 0; k < shapes[j].mesh.material_ids.size(); k++) {
            sid = sid + 1;
            mid = shapes[j].mesh.material_ids[k];
            assert(sid >= 0 && sid < spes.size()); // primitive must have a valid shape

            auto spe = spes[sid];
            auto mtl = mid != -1 ? mtls[mid] : nullptr;
            auto prm = std::make_shared<Primitive>(m_primitives.size(), spe, mtl);

            if (mtl && mtl->isDelta()) { m_delta_primitives.push_back(prm); }
            m_primitives.push_back(prm);
            count++;
        }
    }

    // 4. Return primitives view
    return std::span<std::shared_ptr<Primitive>>(m_primitives.data() + start, count);
}

std::span<std::shared_ptr<Shape>> Scene::loadShapes(const std::filesystem::path& obj_path, const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes) {
    // 0. Cache shapes
    int start = 0, count = 0;
    static std::unordered_map<std::string, std::pair<int, int>> cache;
    if (cache.count(obj_path.string())) {
        start = cache[obj_path.string()].first;
        count = cache[obj_path.string()].second;
        return std::span<std::shared_ptr<Shape>>(m_shapes.data() + start, count);
    }

    // 1. Convert tinyobj::shape_t into spt::Shape
    start = m_shapes.size();
    for (const auto& shape : shapes) {
        for (int i = 0; i < shape.mesh.material_ids.size(); ++i) { // i is face index
            // 1.0 Initialize vertex attributes
            bool vn = true, vt = true;
            std::array<Vec3<float>, 3> vertex, normal;
            std::array<Vec2<float>, 3> uv;

            // 1.1 Get vertex info from tinyobj::attrib_t and tinyobj::shape_t
            for (int j = 0; j < 3; ++j) { // j is vertex index
                tinyobj::index_t index = shape.mesh.indices[3 * i + j];
                int v = index.vertex_index, n = index.normal_index, t = index.texcoord_index;

                vertex[j] = Vec3<float>(attrib.vertices[3 * v], attrib.vertices[3 * v + 1], attrib.vertices[3 * v + 2]);
                if (n >= 0) { normal[j] = Vec3<float>(attrib.normals[3 * n], attrib.normals[3 * n + 1], attrib.normals[3 * n + 2]); }
                if (t >= 0) { uv[j] = Vec2<float>(attrib.texcoords[2 * t], attrib.texcoords[2 * t + 1]); }
                vn = (n >= 0) && vn;
                vt = (t >= 0) && vt;
            }

            // 1.2 Get face normal if normal is not available
            if (!vn) {
                Vec3<float> edge1 = vertex[1] - vertex[0], edge2 = vertex[2] - vertex[0];
                Vec3<float> fnormal = normalize(cross(edge1, edge2));
                for (int j = 0; j < 3; ++j) { normal[j] = fnormal; }
            }

            // 1.3 Get texture coordinates if uv is not available
            if (!vt) {
                for (int j = 0; j < 3; ++j) {
                    Vec3<float> d = normalize(vertex[j]);
                    float u       = 0.5f + (std::atan2(d.z, d.x) / (2.0f * PI));
                    float v       = 0.5f - (std::asin(d.y) / PI);
                    uv[j]         = Vec2<float>(u, v);
                }
            }

            // 1.4 Create triangle shape
            auto spe = std::make_shared<Triangle>(m_shapes.size(), vertex, uv, normal[0]); // TODO: fix face normal
            m_shapes.push_back(spe);
            count++;
        }
    }
    cache[obj_path.string()] = {start, count};

    return std::span<std::shared_ptr<Shape>>(m_shapes.data() + start, count);
}

std::span<std::shared_ptr<Material>> Scene::loadMaterials(const fs::path& mtl_dir, const std::vector<tinyobj::material_t>& materials) {
    // 0. Cache materials
    int start = 0, count = 0;
    static std::unordered_map<std::string, std::pair<int, int>> cache;
    if (cache.count(mtl_dir.string())) {
        start = cache[mtl_dir.string()].first;
        count = cache[mtl_dir.string()].second;
        return std::span<std::shared_ptr<Material>>(m_materials.data() + start, count);
    }

    // 1. Convert tinyobj::material_t into spt::Material
    start = m_materials.size();
    for (auto& mtl : materials) {
        auto nmtl = std::make_shared<Material>(m_materials.size(), mtl);
        m_materials.push_back(nmtl);
    }
    cache[mtl_dir.string()] = {start, count};

    return std::span<std::shared_ptr<Material>>(m_materials.data() + start, count);
}
}; // namespace spt
