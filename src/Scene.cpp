#include "Scene.hpp"

#include <array>
#include <fstream>
#include <rapidjson/document.h>
#include <tiny_obj_loader.h>

namespace spt {

namespace fs = std::filesystem;

void Scene::init(const fs::path& path) {
    // 0. Read config file
    std::ifstream file(path);
    if (!file.is_open()) { throw std::runtime_error("Scene::initialize: Error opening config file"); }
    std::stringstream buffer;
    buffer << file.rdbuf();

    // 1. Parse config file
    rapidjson::Document doc;
    if (doc.Parse(buffer.str().c_str()).HasParseError()) { throw std::runtime_error("Scene::initialize: Error parsing JSON"); }
    auto getVec3 = [](rapidjson::Value& arr) -> Vec3<float> {
        if (!arr.IsArray() || arr.Size() != 3 || !arr[0].IsNumber() || !arr[1].IsNumber() || !arr[2].IsNumber()) { throw std::runtime_error("Scene::initialize: Invalid array size or element type"); }

        return Vec3<float>{arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat()};
    };

    // 2. Add triangles and materials
    if (doc.HasMember("models")) {
        m_triangles.resize(doc["models"].Size());
        m_materials.resize(doc["models"].Size());

        for (int i = 0; i < doc["models"].Size(); i++) {
            auto& modelDoc = doc["models"][i];
            // model base dir and name required
            fs::path dir        = fs::current_path();
            fs::path objpath    = modelDoc["obj_path"].GetString();
            fs::path mtldir     = modelDoc.HasMember("mtl_dir") ? modelDoc["mtl_dir"].GetString() : objpath.parent_path();
            std::string objname = objpath.stem().string();

            tinyobj::attrib_t attrib;
            std::vector<tinyobj::shape_t> shapes;
            std::vector<tinyobj::material_t> materials;
            std::string warn, err;
            if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objpath.c_str(), mtldir.c_str(), true)) {
                throw std::runtime_error("Scene::loadModel: " + err);
            }

            for (int j = 0; j < shapes.size(); j++) {
                for (int k = 0; k < shapes[j].mesh.material_ids.size(); k++) {
                    int l = shapes[j].mesh.material_ids[k];
                    m_triangles[i].push_back(loadTriangle(k, l, attrib, shapes[j]));
                }
            }

            for (int j = 0; j < materials.size(); j++) {
                m_materials[i].push_back(loadMaterial(mtldir, materials[j]));
            }

            // default material optional
            if (modelDoc.HasMember("default_mat")) {
                auto& matDoc = modelDoc["default_mat"];
                tinyobj::material_t material;
                material.name       = matDoc.HasMember("name") ? matDoc["name"].GetString() : objname + "_default";
                material.dissolve   = matDoc.HasMember("opacity") ? matDoc["opacity"].GetFloat() : 1.f;
                material.diffuse[0] = matDoc.HasMember("albedo") ? matDoc["albedo"][0].GetFloat() : 0.f;
                material.diffuse[1] = matDoc.HasMember("albedo") ? matDoc["albedo"][1].GetFloat() : 0.f;
                material.diffuse[2] = matDoc.HasMember("albedo") ? matDoc["albedo"][2].GetFloat() : 0.f;
                material.metallic   = matDoc.HasMember("metallic") ? matDoc["metallic"].GetFloat() : 0.f;
                material.roughness  = matDoc.HasMember("roughness") ? matDoc["roughness"].GetFloat() : 0.f;
                m_materials[i].push_back(loadMaterial(dir, material));
            } else {
                m_materials[i].push_back(nullptr); // raise exception when matid is -1
            }
        }
    }

    // 3. Correlate triangles and materials
    assert(m_triangles.size() == m_materials.size());
    int tid = 0;
    for (int i = 0; i < m_triangles.size(); i++) {
        for (int j = 0; j < m_triangles[i].size(); j++) {
            int mid = m_triangles[i][j]->getMaterialID() != -1 ? m_triangles[i][j]->getMaterialID() : m_materials[i].size() - 1;
            if (m_materials[i][mid] == nullptr) { throw std::runtime_error("Scene::initialize: Invalid material ID"); }

            m_triangles[i][j]->setID(tid);
            m_triangles[i][j]->setMaterial(m_materials[i][mid]);
            tid++;
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
    if (doc.HasMember("lights")) {
        m_light = std::make_shared<Light>();

        std::unordered_map<std::string, Vec3<float>> name2Colors;
        for (int i = 0; i < doc["lights"].Size(); i++) {
            auto& lightDoc    = doc["lights"][i];
            std::string name  = lightDoc.HasMember("mat_name") ? lightDoc["mat_name"].GetString() : "light" + std::to_string(i);
            Vec3<float> color = lightDoc.HasMember("color") ? getVec3(lightDoc["color"]) : Vec3<float>{0.f, 0.f, 0.f};
            name2Colors[name] = color;
        }

        for (int i = 0; i < m_materials.size(); i++) {
            for (auto mtl : m_materials[i]) {
                if (mtl == nullptr) { continue; } // avoid nullptr pointer(could be default material)
                if (name2Colors.count(mtl->getName())) { mtl->setEmission(name2Colors[mtl->getName()]); }
            }
        }

        for (int i = 0; i < m_triangles.size(); i++) {
            for (auto triangle : m_triangles[i]) {
                auto mtl = triangle->getMaterial();
                if (mtl->isEmissive()) { m_light->add(triangle); }
            }
        }
    }

    // 6. Set bounding volume hierarchy
    std::vector<std::shared_ptr<Triangle>> triangles;
    for (int i = 0; i < m_triangles.size(); i++) {
        triangles.insert(triangles.end(), m_triangles[i].begin(), m_triangles[i].end());
    }
    m_bvh = BVH::constructBVH(triangles, 0, triangles.size() - 1, 20);

    // 7. Print scene info
    // std::cout << " Camera:\n"
    //           << " Eye: " << m_camera->getEye() << '\n'
    //           << " Target: " << m_camera->getTarget() << '\n'
    //           << " Up: " << m_camera->getUp() << '\n'
    //           << " Axis: " << m_camera->getAxis(0) << '\n'
    //           << " Axis: " << m_camera->getAxis(1) << '\n'
    //           << " Axis: " << m_camera->getAxis(2) << '\n'
    //           << " Fovy: " << m_camera->getFovy() << '\n'
    //           << " Focus: " << m_camera->getFocus() << '\n'
    //           << " Pixel: " << m_camera->getPixel() << '\n';

    // std::cout << " Model:\n";
    // for (int i = 0; i < m_triangles.size(); i++) {
    //     for (auto triangle : m_triangles[i]) {
    //         auto mtl = triangle->getMaterial();
    //         std::cout << "  " << triangle->getID() << " " << triangle->getMaterialID() << " " << mtl->getName() << " " << mtl->getAlbedo(Vec2<float>{0.f, 0.f}) << '\n';
    //     }
    // }
    // for (int i = 0; i < m_materials.size(); i++) {
    //     for (auto mtl : m_materials[i]) {
    //         if (mtl == nullptr) { continue; } // avoid nullptr pointer(could be default material)
    //         std::cout << "name: " << mtl->getName() << " type: " << mtl->getTypeStr() << '\n';
    //     }
    // }
}

void Scene::destroy() {
    m_triangles.clear();
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
    std::shared_ptr<Material> mtl = std::make_shared<Material>(material.name);

    mtl->setEmission(material.emission);
    mtl->setAlbedo(material.diffuse);
    mtl->setIOR(material.ior);
    mtl->setRoughness(material.roughness);
    mtl->setMetallic(material.metallic);

    return mtl;
}

}; // namespace spt
