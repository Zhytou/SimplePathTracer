#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <set>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

int main() {
    std::string input_filename = "../example/staircase/stairscase.obj";   // 原始模型文件
    std::string output_filename = "panel.obj";  // 输出文件
    std::string target_material = "Glass";      // 要提取的材质名

    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = "../example/staircase/"; // 可选：材质文件搜索路径

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(input_filename, reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "Failed to load OBJ file: " << reader.Error() << std::endl;
        }
        return -1;
    }

    if (!reader.Warning().empty()) {
        std::cout << "Warning: " << reader.Warning() << std::endl;
    }

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();
    const auto& materials = reader.GetMaterials();

    // 构建材质名到索引的映射
    std::unordered_map<std::string, int> material_name_to_id;
    for (size_t i = 0; i < materials.size(); ++i) {
        material_name_to_id[materials[i].name] = static_cast<int>(i);
    }

    // 查找目标材质ID
    int target_material_id = -1;
    if (material_name_to_id.find(target_material) != material_name_to_id.end()) {
        target_material_id = material_name_to_id[target_material];
    } else {
        std::cerr << "Material '" << target_material << "' not found in the model." << std::endl;
        return -1;
    }

    // 收集所有使用目标材质的面（indices）
    std::vector<tinyobj::index_t> filtered_indices;
    std::set<unsigned int> used_v, used_vt, used_vn; // 用于记录用到的顶点索引

    for (const auto& shape : shapes) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
            int fv = shape.mesh.num_face_vertices[f];
            int face_material_id = shape.mesh.material_ids[f];

            if (face_material_id == target_material_id) {
                for (size_t v = 0; v < fv; ++v) {
                    tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                    filtered_indices.push_back(idx);

                    // 记录使用的索引（注意：tinyobj 使用 0-based，且负数表示未使用）
                    if (idx.vertex_index >= 0) used_v.insert(idx.vertex_index);
                    if (idx.texcoord_index >= 0) used_vt.insert(idx.texcoord_index);
                    if (idx.normal_index >= 0) used_vn.insert(idx.normal_index);
                }
            }
            index_offset += fv;
        }
    }

    if (filtered_indices.empty()) {
        std::cerr << "No faces found with material '" << target_material << "'." << std::endl;
        return -1;
    }

    // 重建局部索引映射（新obj中顶点从1开始编号）
    std::unordered_map<unsigned int, unsigned int> v_old_to_new;
    std::unordered_map<unsigned int, unsigned int> vt_old_to_new;
    std::unordered_map<unsigned int, unsigned int> vn_old_to_new;

    // 写入文件
    std::ofstream out(output_filename);
    if (!out.is_open()) {
        std::cerr << "Cannot open output file: " << output_filename << std::endl;
        return -1;
    }

    // 写入顶点 v
    std::vector<float> vertices = attrib.vertices;
    for (unsigned int old_idx : used_v) {
        v_old_to_new[old_idx] = static_cast<unsigned int>(v_old_to_new.size()) + 1;
        out << "v "
        << vertices[old_idx * 3 + 0] << " "
        << vertices[old_idx * 3 + 1] << " "
        << vertices[old_idx * 3 + 2] << "\n";
    }

    // 写入纹理坐标 vt（如果存在）
    if (!attrib.texcoords.empty()) {
        for (unsigned int old_idx : used_vt) {
            vt_old_to_new[old_idx] = static_cast<unsigned int>(vt_old_to_new.size()) + 1;
            out << "vt "
            << attrib.texcoords[old_idx * 2 + 0] << " "
            << attrib.texcoords[old_idx * 2 + 1] << "\n";
        }
    }

    // 写入法线 vn（如果存在）
    if (!attrib.normals.empty()) {
        for (unsigned int old_idx : used_vn) {
            vn_old_to_new[old_idx] = static_cast<unsigned int>(vn_old_to_new.size()) + 1;
            out << "vn "
            << attrib.normals[old_idx * 3 + 0] << " "
            << attrib.normals[old_idx * 3 + 1] << " "
            << attrib.normals[old_idx * 3 + 2] << "\n";
        }
    }

    // 写入材质库和材质（简化处理：只写材质名）
    out << "mtllib " << target_material << ".mtl\n"; // 可选，若你有对应mtl
    out << "usemtl " << target_material << "\n";

    // 写入面 f
    size_t idx_offset = 0;
    for (const auto& shape : shapes) {
        size_t face_start = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
            int fv = shape.mesh.num_face_vertices[f];
            int face_material_id = shape.mesh.material_ids[f];

            if (face_material_id == target_material_id) {
                out << "f";
                for (int v = 0; v < fv; ++v) {
                    tinyobj::index_t idx = filtered_indices[idx_offset++];
                    out << " ";
                    // 顶点索引（必须）
                    out << v_old_to_new.at(static_cast<unsigned int>(idx.vertex_index));
                    // 纹理坐标（如果有）
                    if (!used_vt.empty() && idx.texcoord_index >= 0) {
                        out << "/" << vt_old_to_new.at(static_cast<unsigned int>(idx.texcoord_index));
                    } else if (!used_vn.empty() || !used_vt.empty()) {
                        out << "/"; // 即使没有vt，若有vn也要保留斜杠占位
                    }
                    // 法线（如果有）
                    if (!used_vn.empty() && idx.normal_index >= 0) {
                        if (used_vt.empty() || idx.texcoord_index < 0) {
                            out << "/"; // 若无vt，则前面已有"/"，这里再加一个
                        }
                        out << "/" << vn_old_to_new.at(static_cast<unsigned int>(idx.normal_index));
                    }
                }
                out << "\n";
            }
            face_start += fv;
        }
    }

    out.close();
    std::cout << "Successfully extracted material '" << target_material 
              << "' to '" << output_filename << "'." << std::endl;

    return 0;
}