#include "Scene.hpp"

#include <array>
#include <format>
#include <fstream>
#include <ranges>
#include <rapidjson/document.h>
#include <tiny_obj_loader.h>
#include <typeinfo>

namespace spt {

namespace fs = std::filesystem;

void Scene::init(const fs::path& path) {
    // 0. Read config file
    std::ifstream file(path);
    if (!file.is_open()) { throw std::runtime_error(std::format("Scene::init: Invalid config file path {}", path.string())); }
    std::stringstream buffer;
    buffer << file.rdbuf();

    // 1. Parse config file
    rapidjson::Document doc;
    if (doc.Parse(buffer.str().c_str()).HasParseError()) { throw std::runtime_error("Scene::initialize: Error parsing JSON"); }
    auto getVec2 = [](rapidjson::Value& arr) -> Vec2<float> {
        if (!arr.IsArray() || arr.Size() != 2 || !arr[0].IsNumber() || !arr[1].IsNumber()) { throw std::runtime_error("Scene::initialize: Invalid array size or element type"); }

        return Vec2<float>{arr[0].GetFloat(), arr[1].GetFloat()};
    };
    auto getVec3 = [](rapidjson::Value& arr) -> Vec3<float> {
        if (!arr.IsArray() || arr.Size() != 3 || !arr[0].IsNumber() || !arr[1].IsNumber() || !arr[2].IsNumber()) { throw std::runtime_error("Scene::initialize: Invalid array size or element type"); }

        return Vec3<float>{arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat()};
    };
    auto getMat = [](rapidjson::Value& doc, const std::string& name) -> tinyobj::material_t {
        tinyobj::material_t material;
        material.name        = doc.HasMember("name") ? doc["name"].GetString() : name + "_default";
        material.emission[0] = doc.HasMember("emission") ? doc["emission"][0].GetFloat() : 0.f; // emission
        material.emission[1] = doc.HasMember("emission") ? doc["emission"][1].GetFloat() : 0.f;
        material.emission[2] = doc.HasMember("emission") ? doc["emission"][2].GetFloat() : 0.f;
        material.diffuse[0]  = doc.HasMember("albedo") ? doc["albedo"][0].GetFloat() : 0.f; // albedo
        material.diffuse[1]  = doc.HasMember("albedo") ? doc["albedo"][1].GetFloat() : 0.f;
        material.diffuse[2]  = doc.HasMember("albedo") ? doc["albedo"][2].GetFloat() : 0.f;
        material.metallic    = doc.HasMember("metallic") ? doc["metallic"].GetFloat() : 0.f;   // metallic
        material.roughness   = doc.HasMember("roughness") ? doc["roughness"].GetFloat() : 1.f; // roughness
        material.dissolve    = doc.HasMember("opacity") ? doc["opacity"].GetFloat() : 1.f;     // opacity
        material.ior         = doc.HasMember("ior") ? doc["ior"].GetFloat() : 1.f;             // ior
        return material;
    };

    // 2. Add analytic geometries
    if (doc.HasMember("geometry")) {
        int gid = m_objects.size();
        for (int i = 0; i < doc["geometry"].HasMember("sphere") ? doc["geometry"]["sphere"].Size() : 0; i++) {
            auto& sphereDoc    = doc["geometry"]["sphere"][i];
            std::string name   = sphereDoc["name"].GetString();
            Vec3<float> center = getVec3(sphereDoc["center"]);
            float radius       = sphereDoc["radius"].GetFloat();

            auto& mtlDoc    = sphereDoc["default_mtl"];
            fs::path mtldir = mtlDoc.HasMember("mtl_dir") ? mtlDoc["mtl_dir"].GetString() : ""; // default material directory for texture
            auto material   = loadMaterial(mtldir, getMat(mtlDoc, name));

            auto sphere = std::make_shared<Sphere>(gid);
            sphere->setCenter(center);
            sphere->setRadius(radius);
            sphere->setMaterial(material);
            m_objects.push_back(sphere);
            gid++;
        }
    }

    // 3. Load obj models
    if (doc.HasMember("model")) {
        for (int i = 0; i < doc["model"].Size(); i++) {
            auto& modelDoc = doc["model"][i];
            // 3.1 Initialize obj file path and mtl file base dir
            fs::path objpath    = modelDoc["obj_path"].GetString();
            fs::path mtldir     = modelDoc.HasMember("mtl_dir") ? modelDoc["mtl_dir"].GetString() : objpath.parent_path();
            std::string objname = objpath.stem().string();

            // 3.2 Load obj file with tinyobjloader
            tinyobj::attrib_t attrib;
            std::vector<tinyobj::shape_t> shapes;
            std::vector<tinyobj::material_t> materials;
            std::string warn, err;
            if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objpath.c_str(), mtldir.c_str(), true)) {
                throw std::runtime_error("Scene::loadModel: " + err);
            }

            // 3.3 Convert tinyobj::shape_t and tinyobj::material_t into spt::Triangle and spt::Material
            std::vector<std::shared_ptr<Triangle>> triangles;
            std::vector<std::shared_ptr<Material>> nmaterials;
            for (int j = 0; j < shapes.size(); j++) {
                for (int k = 0; k < shapes[j].mesh.material_ids.size(); k++) {
                    int l = shapes[j].mesh.material_ids[k];
                    triangles.push_back(loadTriangle(k, l, attrib, shapes[j]));
                }
            }
            for (int j = 0; j < materials.size(); j++) {
                nmaterials.push_back(loadMaterial(mtldir, materials[j]));
            }

            // 3.4 Load default material if specified
            if (modelDoc.HasMember("default_mtl")) {
                auto& mtlDoc     = modelDoc["default_mtl"];
                fs::path dmtldir = mtlDoc.HasMember("mtl_dir") ? mtlDoc["mtl_dir"].GetString() : ""; // default material directory for texture
                auto material    = getMat(mtlDoc, objname);
                nmaterials.push_back(loadMaterial(dmtldir, material));
            }

            // 3.4 Correlate triangles and materials
            int tid = m_objects.size();
            for (int j = 0; j < triangles.size(); j++) {
                int mid = triangles[j]->getMaterialID() != -1 ? triangles[j]->getMaterialID() : nmaterials.size() - 1;
                if (mid == -1 || nmaterials[mid] == nullptr) { throw std::runtime_error(std::format("Scene::initialize: Invalid material ID {} for triangle {} under group {}", mid, j, i)); }
                triangles[j]->setID(tid);
                triangles[j]->setMaterial(nmaterials[mid]);
                tid++;
            }

            // 3.5 Add triangles to scene objects
            m_objects.insert(m_objects.end(), triangles.begin(), triangles.end());
        }
    }

    // 4. Add camera
    if (doc.HasMember("camera")) {
        auto& cameraDoc = doc["camera"];
        m_camera        = std::make_shared<PerspectiveCamera>();
        m_camera->setWidth(cameraDoc["width"].GetInt());
        m_camera->setHeight(cameraDoc["height"].GetInt());
        m_camera->setFovy(cameraDoc["fovy"].GetFloat());
        m_camera->setEye(getVec3(cameraDoc["eye"]));
        m_camera->setTarget(getVec3(cameraDoc["target"]));
        m_camera->setUp(getVec3(cameraDoc["up"]));
    }

    // 5. Set light
    if (doc.HasMember("light")) {
        // 5.1 Load area light
        if (doc["light"].HasMember("area")) {
            // 5.1.1 Get area light emission
            std::unordered_map<std::string, Vec3<float>> emissions;
            for (int i = 0; i < doc["light"]["area"].Size(); i++) {
                auto& alDoc       = doc["light"]["area"][i];
                std::string name  = alDoc.HasMember("mtl_name") ? alDoc["mtl_name"].GetString() : std::format("light{}", i);
                Vec3<float> color = alDoc.HasMember("color") ? getVec3(alDoc["color"]) : Vec3<float>(0.f);
                emissions[name]   = color;
            }

            // 5.1.2 Set corresponding materials's emission
            for (auto& [name, mtl] : m_materials) {
                if (emissions.count(name) == 0) { continue; }
                mtl.lock()->setEmission(emissions[name]);
            }

            // 5.1.3 Add area light object to scene light
            for (auto object : m_objects) {
                auto mtl = object->getMaterial();
                if (mtl->isEmissive()) {
                    m_lights.push_back(std::make_shared<AreaLight>(mtl->getEmission(), object));
                }
            }
        }

        // 5.2 Load point light
        for (int i = 0; i < doc["light"].HasMember("point") ? doc["light"]["point"].Size() : 0; i++) {
            auto& plDoc          = doc["light"]["point"][i];
            Vec3<float> position = plDoc.HasMember("position") ? getVec3(plDoc["position"]) : Vec3<float>(0.f);
            Vec3<float> color    = plDoc.HasMember("color") ? getVec3(plDoc["color"]) : Vec3<float>(0.f);
            m_lights.push_back(std::make_shared<PointLight>(color, position));
        }

        // 5.3 Load directional light
        for (int i = 0; i < doc["light"].HasMember("directional") ? doc["light"]["directional"].Size() : 0; i++) {
            auto& dlDoc           = doc["light"]["directional"][i];
            Vec3<float> direction = dlDoc.HasMember("direction") ? getVec3(dlDoc["direction"]) : Vec3<float>(0.f);
            Vec3<float> color     = dlDoc.HasMember("color") ? getVec3(dlDoc["color"]) : Vec3<float>(0.f);
            m_lights.push_back(std::make_shared<DirectionalLight>(color, direction));
        }
    }

    // 6. Set bounding volume hierarchy
    auto aabb = BVH::mergeAABBs(m_objects, 0, m_objects.size());
    m_bvh     = BVH::constructBVH(m_objects, aabb, 0, m_objects.size(), 50);

    // 7. Print scene info
    {
        std::cout << "\n=============================================================================================================\n";
        std::cout << "                                                 SCENE HIERARCHY INFO                                          \n";
        std::cout << "===============================================================================================================\n";

        constexpr int n = 10;
        constexpr int k = 5;

        // 7.1 Print camera configuration
        std::cout << " [ CAMERA CONFIGURATION ]\n";
        std::cout << "   - Eye:    " << m_camera->getEye() << '\n'
                  << "   - Target: " << m_camera->getTarget() << '\n'
                  << "   - Up:     " << m_camera->getUp() << '\n'
                  << "   - Fovy:   " << m_camera->getFovy() << '\n'
                  << "   - Focus: " << m_camera->getFocus() << '\n'
                  << "-------------------------------------------------------------------------------------------------------------------------\n";

        // 7.2 Print objects list
        std::cout << " [ OBJECTS ]\n";
        std::cout << "   " << std::left << std::setw(6) << "ID"
                  << std::setw(25) << "Type"
                  << std::setw(20) << "Assigned Material" << '\n';
        std::cout << std::string(123, '-') << '\n';

        if (m_objects.size() < n) {
            for (const auto& obj : m_objects) {
                auto mtl  = obj->getMaterial();
                auto name = typeid(*obj).name();
                std::cout << "   " << std::left << std::setw(6) << obj->getID()
                          << std::setw(25) << name
                          << std::setw(20) << (mtl ? mtl->getName() : "None") << '\n';
            }
        } else {
            for (const auto& obj : m_objects | std::views::take(k)) {
                auto mtl  = obj->getMaterial();
                auto name = typeid(*obj).name();
                std::cout << "   " << std::left << std::setw(6) << obj->getID()
                          << std::setw(25) << name
                          << std::setw(20) << (mtl ? mtl->getName() : "None") << '\n';
            }
            std::cout << "   ......\n";
            for (const auto& obj : m_objects | std::views::drop(m_objects.size() - k)) {
                auto mtl  = obj->getMaterial();
                auto name = typeid(*obj).name();
                std::cout << "   " << std::left << std::setw(6) << obj->getID()
                          << std::setw(25) << name
                          << std::setw(20) << (mtl ? mtl->getName() : "None") << '\n';
            }
        }
        std::cout << "-------------------------------------------------------------------------------------------------------------------------\n";

        // 7.3 Print materials summary
        std::cout << " [ MATERIALS SUMMARY ]\n";
        std::cout << "   " << std::left << std::setw(15) << "Name"
                  << std::setw(40) << "Type"
                  << std::setw(30) << "Albedo (R, G, B)"
                  << std::setw(12) << "Roughness"
                  << std::setw(10) << "Metallic"
                  << std::setw(10) << "Opacity"
                  << std::setw(8) << "IOR" << '\n';
        std::cout << std::string(123, '-') << '\n';

        for (auto [name, mtlw] : m_materials) {
            if (mtlw.expired()) { continue; }

            auto mtl = mtlw.lock();
            Vec2<float> uv(0.5f);
            auto type          = mtl->getTypeStr();
            Vec3<float> albedo = mtl->getAlbedo(uv);
            float roughness    = mtl->getRoughness(uv);
            float metallic     = mtl->getMetallic(uv);
            float opacity      = mtl->getOpacity();
            float ior          = mtl->getIOR();

            std::cout << "   " << std::left << std::setw(15) << mtl->getName()
                      << std::setw(40) << type
                      << std::setw(30) << albedo
                      << std::setw(12) << std::fixed << std::setprecision(4) << roughness
                      << std::setw(10) << metallic
                      << std::setw(10) << opacity
                      << std::setw(8) << ior << '\n';
        }
        std::cout << "-------------------------------------------------------------------------------------------------------------------------\n";

        // 7.4 Print light sources list
        std::cout << " [ LIGHT SOURCES ]\n";
        if (m_lights.size() < n) {
            for (const auto& light : m_lights) {
                std::cout << "   - Light ID: " << light->getID() << '\n';
            }
        } else {
            for (const auto& light : m_lights | std::views::take(k)) {
                std::cout << "   - Light ID: " << light->getID() << '\n';
            }
            std::cout << "   ......\n";
            for (const auto& light : m_lights | std::views::drop(m_lights.size() - k)) {
                std::cout << "   - Light ID: " << light->getID() << '\n';
            }
        }
        std::cout << "============================================================================================================================\n";
    }
}

void Scene::destroy() {
    m_objects.clear();
    m_materials.clear();
}

std::shared_ptr<Triangle> Scene::loadTriangle(int tid, int mid, const tinyobj::attrib_t& attrib, const tinyobj::shape_t& shape) {
    bool vn = true, vt = true; // whether vertex normal and uv are available
    std::array<Vec3<float>, 3> vertex, normal;
    std::array<Vec2<float>, 3> uv;

    for (int vid = 0; vid < 3; ++vid) { // vid is vertex index
        tinyobj::index_t index = shape.mesh.indices[3 * tid + vid];
        int v = index.vertex_index, n = index.normal_index, t = index.texcoord_index;

        vertex[vid] = Vec3<float>(attrib.vertices[3 * v], attrib.vertices[3 * v + 1], attrib.vertices[3 * v + 2]);
        if (n >= 0) { normal[vid] = Vec3<float>(attrib.normals[3 * n], attrib.normals[3 * n + 1], attrib.normals[3 * n + 2]); }
        if (t >= 0) { uv[vid] = Vec2<float>(attrib.texcoords[2 * t], attrib.texcoords[2 * t + 1]); }
        vn = (n >= 0) && vn;
        vt = (t >= 0) && vt;
    }

    if (!vn) {
        Vec3<float> edge1 = vertex[1] - vertex[0], edge2 = vertex[2] - vertex[0];
        Vec3<float> fnormal = normalize(cross(edge1, edge2));
        for (int vid = 0; vid < 3; ++vid) { normal[vid] = fnormal; }
    }

    if (!vt) {
        for (int vid = 0; vid < 3; ++vid) {
            Vec3<float> d = normalize(vertex[vid]);
            float u       = 0.5f + (std::atan2(d.z, d.x) / (2.0f * PI));
            float v       = 0.5f - (std::asin(d.y) / PI);
            uv[vid]       = Vec2<float>(u, v);
        }
    }

    std::shared_ptr<Triangle> triangle = std::make_shared<Triangle>(-1);
    triangle->setVertex(vertex);
    triangle->setTexCoord(uv);
    triangle->setNormal(normal[0]); // TODO: fix face normal
    triangle->setMaterialID(mid);

    return triangle;
}

std::shared_ptr<Material> Scene::loadMaterial(const fs::path& mtldir, const tinyobj::material_t& material) {
    if (m_materials.count(material.name) && m_materials[material.name].lock() != nullptr) { return m_materials[material.name].lock(); }

    // Determine material type with following rules:
    // - roughness difference is the key factor to distinguish between diffuse, glossy, and specular materials
    // - metallic distinguishes between conductive, semiconductive, and dielectric materials
    //   - for conductive materials, only reflection happens
    //   - while semiconductive and dielectric materials support both reflection and transmission, using opacity to indicate transparency and ior to indicate the real part of the index of refraction
    MaterialType surface = material.roughness > 0.9 ? MaterialType::MATERIAL_SURFACE_DIFFUSE : (material.roughness < 0.1 ? MaterialType::MATERIAL_SURFACE_SPECULAR : MaterialType::MATERIAL_SURFACE_GLOSSY);
    MaterialType physics = material.metallic > 0.9 ? MaterialType::MATERIAL_PHYSICS_CONDUCTIVE : (material.metallic < 0.1 ? MaterialType::MATERIAL_PHYSICS_DIELECTRIC : MaterialType::MATERIAL_PHYSICS_SEMICONDUCTIVE);

    std::shared_ptr<Material> mtl = std::make_shared<Material>(material.name);
    mtl->setType(static_cast<MaterialType>(surface | physics));
    mtl->setEmission(Vec3<float>(material.emission));
    mtl->setAlbedo(Vec3<float>(material.diffuse));
    mtl->setRoughness(material.roughness);
    mtl->setMetallic(material.metallic);
    mtl->setOpacity(material.dissolve);
    mtl->setIOR(material.ior);
    m_materials[material.name] = mtl;

    return mtl;
}

}; // namespace spt
