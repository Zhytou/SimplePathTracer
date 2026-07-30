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

    // 1.1 Define spt::Vec2 and spt::Vec3 conversion lambda functions
    auto getVec2 = [](const rapidjson::Value& arr) -> Vec2<float> {
        if (!arr.IsArray() || arr.Size() != 2 || !arr[0].IsNumber() || !arr[1].IsNumber()) { throw std::runtime_error("Scene::initialize: Invalid array size or element type"); }

        return Vec2<float>{arr[0].GetFloat(), arr[1].GetFloat()};
    };
    auto getVec3 = [](const rapidjson::Value& arr) -> Vec3<float> {
        if (!arr.IsArray() || arr.Size() != 3 || !arr[0].IsNumber() || !arr[1].IsNumber() || !arr[2].IsNumber()) { throw std::runtime_error("Scene::initialize: Invalid array size or element type"); }

        return Vec3<float>{arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat()};
    };

    // 1.2 Define transform matrix and tinyobj::material_t conversion lambda function
    auto cvtTfm = [&](const rapidjson::Value& doc) -> Mat4x4f {
        Mat4x4f mat = Mat4x4f::eye();
        if (doc.HasMember("translate")) { mat = mat * translate(getVec3(doc["translate"])); }
        if (doc.HasMember("rotate")) { mat = mat * rotate(getVec3(doc["rotate"])); }
        if (doc.HasMember("scale")) { mat = mat * scale(getVec3(doc["scale"])); }
        return mat;
    };
    auto cvtMtl = [](rapidjson::Value& doc, const std::string& name, const fs::path& dir) -> tinyobj::material_t {
        tinyobj::material_t mtl;
        mtl.name              = doc.HasMember("name") ? doc["name"].GetString() : name + "_default";
        mtl.emission[0]       = doc.HasMember("emission") ? doc["emission"][0].GetFloat() : 0.f; // emission
        mtl.emission[1]       = doc.HasMember("emission") ? doc["emission"][1].GetFloat() : 0.f;
        mtl.emission[2]       = doc.HasMember("emission") ? doc["emission"][2].GetFloat() : 0.f;
        mtl.diffuse[0]        = doc.HasMember("albedo") ? doc["albedo"][0].GetFloat() : 0.f; // albedo
        mtl.diffuse[1]        = doc.HasMember("albedo") ? doc["albedo"][1].GetFloat() : 0.f;
        mtl.diffuse[2]        = doc.HasMember("albedo") ? doc["albedo"][2].GetFloat() : 0.f;
        mtl.metallic          = doc.HasMember("metallic") ? doc["metallic"].GetFloat() : 0.f;   // metallic
        mtl.roughness         = doc.HasMember("roughness") ? doc["roughness"].GetFloat() : 1.f; // roughness
        mtl.dissolve          = doc.HasMember("opacity") ? doc["opacity"].GetFloat() : 1.f;     // opacity
        mtl.ior               = doc.HasMember("ior") ? doc["ior"].GetFloat() : 1.f;             // ior
        mtl.diffuse_texname   = doc.HasMember("diffuse_texname") ? dir / doc["diffuse_texname"].GetString() : "";
        mtl.metallic_texname  = doc.HasMember("metallic_texname") ? dir / doc["metallic_texname"].GetString() : "";
        mtl.roughness_texname = doc.HasMember("roughness_texname") ? dir / doc["roughness_texname"].GetString() : "";
        return mtl;
    };

    // 2. Load analytic geometries
    if (doc.HasMember("geometry")) {
        for (int i = 0; i < doc["geometry"].HasMember("sphere") ? doc["geometry"]["sphere"].Size() : 0; i++) {
            auto& sphere_doc        = doc["geometry"]["sphere"][i];
            std::string sphere_name = sphere_doc["name"].GetString();
            Vec3<float> center      = getVec3(sphere_doc["center"]);
            float radius            = sphere_doc["radius"].GetFloat();
            auto& mtl_doc           = sphere_doc["default_mtl"];
            fs::path mtl_dir        = mtl_doc.HasMember("mtl_dir") ? mtl_doc["mtl_dir"].GetString() : "";

            auto spe = std::make_shared<Sphere>(m_shapes.size(), center, radius);
            auto mtl = std::make_shared<Material>(m_materials.size(), cvtMtl(mtl_doc, sphere_name, mtl_dir));
            auto tfm = sphere_doc.HasMember("transform") ? cvtTfm(sphere_doc["transform"]) : Mat4x4f::eye();
            auto prm = std::make_shared<Primitive>(m_primitives.size(), spe, mtl, tfm);

            m_shapes.push_back(spe);
            m_materials.push_back(mtl);
            m_primitives.push_back(prm);
        }
    }

    // 3. Load obj models
    if (doc.HasMember("model")) {
        for (int i = 0; i < doc["model"].Size(); i++) {
            auto& model_doc = doc["model"][i];
            // 3.1 Initialize obj file path and mtl file base dir
            fs::path obj_path    = model_doc["obj_path"].GetString();
            fs::path mtl_dir     = model_doc.HasMember("mtl_dir") ? model_doc["mtl_dir"].GetString() : obj_path.parent_path();
            std::string obj_name = obj_path.stem().string();

            // 3.2 Load obj file with tinyobjloader
            tinyobj::attrib_t attrib;
            std::vector<tinyobj::shape_t> shapes;
            std::vector<tinyobj::material_t> materials;
            std::string warn, err;
            if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, obj_path.c_str(), mtl_dir.c_str(), true)) {
                throw std::runtime_error("Scene::loadModel: " + err);
            }

            // 3.3 Convert tinyobj::shape_t and tinyobj::material_t into spt::Shape and spt::Material
            std::vector<std::shared_ptr<Shape>> nshapes       = loadShapes(obj_path, attrib, shapes);
            std::vector<std::shared_ptr<Material>> nmaterials = loadMaterials(mtl_dir, materials);

            // 3.4 Add default material to list tail if specified
            if (model_doc.HasMember("default_mtl")) {
                auto& mtl_doc    = model_doc["default_mtl"];
                fs::path mtl_dir = mtl_doc.HasMember("mtl_dir") ? mtl_doc["mtl_dir"].GetString() : "";
                nmaterials.push_back(std::make_shared<Material>(-1, cvtMtl(mtl_doc, obj_name, mtl_dir)));
            }

            // 3.5 Load transform matrix
            Mat4x4f transform = model_doc.HasMember("transform") ? cvtTfm(model_doc["transform"]) : Mat4x4f::eye();

            // 3.6 Create primitives
            int pid = m_primitives.size(), sid = 0;
            for (int j = 0; j < shapes.size(); j++) {
                for (int k = 0; k < shapes[j].mesh.material_ids.size(); k++) {
                    int mid = shapes[j].mesh.material_ids[k] == -1 ? nmaterials.size() - 1 : shapes[j].mesh.material_ids[k];
                    if (sid < 0 || sid >= nshapes.size() || mid < 0 || mid >= nmaterials.size()) { throw std::runtime_error(std::format("Scene::init: shape id {} or material id {} is out of range", sid, mid)); }
                    nshapes[sid]->setID(m_shapes.size());
                    nmaterials[mid]->setID(m_materials.size());
                    m_primitives.push_back(std::make_shared<Primitive>(pid++, nshapes[sid++], nmaterials[mid], transform));
                }
            }

            // 3.7 Add shapes and materials
            m_shapes.insert(m_shapes.end(), nshapes.begin(), nshapes.end());
            m_materials.insert(m_materials.end(), nmaterials.begin(), nmaterials.end());
        }
    }

    // 4. Load main camera
    if (doc.HasMember("camera")) {
        auto& camera_doc = doc["camera"];
        m_camera         = std::make_shared<PerspectiveCamera>();
        m_camera->setWidth(camera_doc["width"].GetInt());
        m_camera->setHeight(camera_doc["height"].GetInt());
        m_camera->setFovy(camera_doc["fovy"].GetFloat());
        m_camera->setEye(getVec3(camera_doc["eye"]));
        m_camera->setTarget(getVec3(camera_doc["target"]));
        m_camera->setUp(getVec3(camera_doc["up"]));
    }

    // 5. Load light sources
    if (doc.HasMember("light")) {
        int lid = 0;
        // 5.1 Load area light
        if (doc["light"].HasMember("area")) {
            // 5.1.1 Get area light emission
            std::unordered_map<std::string, Vec3<float>> emissions;
            std::unordered_map<std::string, std::vector<std::shared_ptr<Primitive>>> primitives; // emissive primitives
            for (int i = 0; i < doc["light"]["area"].Size(); i++) {
                auto& light_doc   = doc["light"]["area"][i];
                std::string name  = light_doc.HasMember("mtl_name") ? light_doc["mtl_name"].GetString() : std::format("light{}", i);
                Vec3<float> color = light_doc.HasMember("color") ? getVec3(light_doc["color"]) : Vec3<float>(0.f);
                emissions[name]   = color;
            }

            // 5.1.2 Set corresponding primitive's emission
            for (auto& prm : m_primitives) {
                auto mtl  = prm->getMaterial();
                auto name = mtl->getName();
                if (emissions.count(name)) {
                    primitives[name].push_back(prm);
                }
            }

            // 5.1.3 Create area emitters
            for (auto [name, color] : emissions) {
                for (auto prm : primitives[name]) {
                    auto emt = std::make_shared<AreaEmitter>(lid++, color, prm);
                    prm->setEmitter(emt);
                    m_emitters.push_back(emt);
                }
            }
        }

        // 5.2 Load point light
        for (int i = 0; i < doc["light"].HasMember("point") ? doc["light"]["point"].Size() : 0; i++) {
            auto& light_doc      = doc["light"]["point"][i];
            Vec3<float> position = light_doc.HasMember("position") ? getVec3(light_doc["position"]) : Vec3<float>(0.f);
            Vec3<float> color    = light_doc.HasMember("color") ? getVec3(light_doc["color"]) : Vec3<float>(0.f);
            m_emitters.push_back(std::make_shared<PointEmitter>(lid++, color, position));
        }

        // 5.3 Load directional light
        for (int i = 0; i < doc["light"].HasMember("directional") ? doc["light"]["directional"].Size() : 0; i++) {
            auto& light_doc       = doc["light"]["directional"][i];
            Vec3<float> direction = light_doc.HasMember("direction") ? getVec3(light_doc["direction"]) : Vec3<float>(0.f);
            Vec3<float> color     = light_doc.HasMember("color") ? getVec3(light_doc["color"]) : Vec3<float>(0.f);
            m_emitters.push_back(std::make_shared<DirectionalEmitter>(lid++, color, direction));
        }
    }

    // 6. Set bounding volume hierarchy and direct light sampler
    m_bvh = BVH::create(m_primitives, AABB::create(m_primitives), max_leaf_size, m_primitives.size() > 1000 ? 16 : -1); // init BVH with m_primitives copy, otherwise m_primitives have to reorder by ascending id to keep scene::getPrimitive correct
    m_des = DES::create(m_emitters);
    std::stable_sort(m_primitives.begin(), m_primitives.end(), [](std::shared_ptr<Primitive> prm1, std::shared_ptr<Primitive> prm2) {
        return prm1->getID() < prm2->getID();
    });

    // 7. Print scene info
    {
        constexpr int n     = 10;
        constexpr int k     = 5;
        constexpr int width = 110; // width for box logging

        // 7.0 Lambda functors and temperol variables
        std::stringstream ss;
        auto printPrmInfo = [&ss](std::shared_ptr<Primitive> prm) {
            if (!prm) { throw std::runtime_error("Primitive is null"); }
            auto mtl = prm->getMaterial();
            auto spe = prm->getShape();

            ss << std::left
               << std::setw(6) << prm->getID()
               << std::setw(25) << typeid(*spe).name()
               << std::setw(20) << (mtl ? mtl->getName() : "None") << '\n';
        };
        auto printMtlInfo = [&ss](std::shared_ptr<Material> mtl) {
            if (!mtl) { throw std::runtime_error("Material is null"); }
            Vec2<float> uv(0.5f);
            auto name          = mtl->getName();
            auto type          = mtl->getTypeStr();
            Vec3<float> albedo = mtl->getAlbedo(uv);
            float roughness    = mtl->getRoughness(uv);
            float metallic     = mtl->getMetallic(uv);
            float opacity      = mtl->getOpacity();
            float ior          = mtl->getIOR();
            if (name.size() >= 15) {
                name = name.substr(0, 10) + "...";
            }
            ss << std::left << std::fixed << std::setprecision(2)
               << std::setw(15) << name
               << std::setw(28) << type
               << std::setw(28) << albedo
               << std::setw(12) << roughness
               << std::setw(10) << metallic
               << std::setw(10) << opacity
               << std::setw(6) << ior << '\n';
        };

        // 7.1 Print camera configuration
        BOX_LOG("CAMERA SETTINGS", width)
            << std::left << std::setw(15) << " - Eye:    " << std::setw(25) << m_camera->getEye() << std::setw(15) << " - Target: " << std::setw(25) << m_camera->getTarget() << '\n'
            << std::setw(15) << " - Fovy: " << std::setw(25) << m_camera->getFovy() << std::setw(15) << " - Focus: " << std::setw(25) << m_camera->getFocus() << '\n';

        // 7.2 Print primitive list
        if (m_primitives.size() < n) {
            for (const auto& prm : m_primitives) { printPrmInfo(prm); }
        } else {
            for (const auto& prm : m_primitives | std::views::take(k)) { printPrmInfo(prm); }
            ss << "   ......\n";
            for (const auto& prm : m_primitives | std::views::drop(m_primitives.size() - k)) { printPrmInfo(prm); }
        }
        BOX_LOG("RENDERABLE PRIMITIVES", width)
            << std::left
            << std::setw(6) << "ID"
            << std::setw(25) << "Class Name"
            << std::setw(20) << "Assigned Material" << '\n'
            << std::string(width - 4, '-') << '\n'
            << ss.rdbuf(); //

        // 7.3 Print materials summary
        for (auto mtl : m_materials) {
            if (!mtl) { continue; }
            printMtlInfo(mtl);
        }
        BOX_LOG("RENDERABLE MATERIALS", width)
            << std::left
            << std::setw(15) << "Name"
            << std::setw(28) << "Material Type"
            << std::setw(28) << "Albedo (R, G, B)"
            << std::setw(12) << "Roughness"
            << std::setw(10) << "Metallic"
            << std::setw(10) << "Opacity"
            << std::setw(6) << "IOR" << '\n'
            << ss.rdbuf(); //
    }
}

void Scene::clear() {
    m_emitters.clear();
    m_shapes.clear();
    m_materials.clear();
}

std::vector<std::shared_ptr<Shape>> Scene::loadShapes(const std::filesystem::path& obj_path, const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes) {
    static std::unordered_map<std::string, std::vector<std::shared_ptr<Shape>>> cache;
    if (cache.count(obj_path.string())) {
        return cache[obj_path.string()];
    }

    std::vector<std::shared_ptr<Shape>> nshapes;
    for (const auto& shape : shapes) {
        for (int i = 0; i < shape.mesh.material_ids.size(); ++i) { // i is face index
            // 0. Initialize vertex attributes
            bool vn = true, vt = true;
            std::array<Vec3<float>, 3> vertex, normal;
            std::array<Vec2<float>, 3> uv;

            // 1. Get vertex info from tinyobj::attrib_t and tinyobj::shape_t
            for (int j = 0; j < 3; ++j) { // j is vertex index
                tinyobj::index_t index = shape.mesh.indices[3 * i + j];
                int v = index.vertex_index, n = index.normal_index, t = index.texcoord_index;

                vertex[j] = Vec3<float>(attrib.vertices[3 * v], attrib.vertices[3 * v + 1], attrib.vertices[3 * v + 2]);
                if (n >= 0) { normal[j] = Vec3<float>(attrib.normals[3 * n], attrib.normals[3 * n + 1], attrib.normals[3 * n + 2]); }
                if (t >= 0) { uv[j] = Vec2<float>(attrib.texcoords[2 * t], attrib.texcoords[2 * t + 1]); }
                vn = (n >= 0) && vn;
                vt = (t >= 0) && vt;
            }

            // 2. Calculate face normal if normal is not available
            if (!vn) {
                Vec3<float> edge1 = vertex[1] - vertex[0], edge2 = vertex[2] - vertex[0];
                Vec3<float> fnormal = normalize(cross(edge1, edge2));
                for (int j = 0; j < 3; ++j) { normal[j] = fnormal; }
            }

            // 3. Calculate texture coordinates if uv is not available
            if (!vt) {
                for (int j = 0; j < 3; ++j) {
                    Vec3<float> d = normalize(vertex[j]);
                    float u       = 0.5f + (std::atan2(d.z, d.x) / (2.0f * PI));
                    float v       = 0.5f - (std::asin(d.y) / PI);
                    uv[j]         = Vec2<float>(u, v);
                }
            }

            // 4. Create triangle shape
            nshapes.push_back(std::make_shared<Triangle>(-1, vertex, uv, normal[0])); // TODO: fix face normal
        }
    }
    cache[obj_path.string()] = nshapes;

    return nshapes;
}

std::vector<std::shared_ptr<Material>> Scene::loadMaterials(const fs::path& mtl_dir, const std::vector<tinyobj::material_t>& materials) {
    static std::unordered_map<std::string, std::vector<std::shared_ptr<Material>>> cache;
    if (cache.count(mtl_dir.string())) {
        return cache[mtl_dir.string()];
    }

    std::vector<std::shared_ptr<Material>> nmaterials;
    for (auto& mtl : materials) {
        auto nmtl = std::make_shared<Material>(-1, mtl);
        nmaterials.push_back(nmtl);
    }
    cache[mtl_dir.string()] = nmaterials;

    return nmaterials;
}

}; // namespace spt
