//
// Created by jet on 4/9/21.
//

#include "core/geometry.hpp"
#include "core/context/global.hpp"

#define TINYOBJLOADER_IMPLEMENTATION

#include "tinyObj/tiny_obj_loader.h"

namespace kuafu {
    bool operator==(const Material &m1, const Material &m2) {
        return (m1.kd == m2.kd) &&
               (m1.emission == m2.emission) &&
               (m1.diffuseTexPath == m2.diffuseTexPath) &&
               (m1.illum == m2.illum) &&
               (m1.d == m2.d) &&
               (m1.ns == m2.ns) &&
               (m1.ni == m2.ni) &&
               (m1.fuzziness == m2.fuzziness);
    }

    std::shared_ptr<Geometry> loadObj(std::string_view path, bool dynamic) {
        std::shared_ptr<Geometry> geometry = std::make_shared<Geometry>();
        geometry->path = path;
        geometry->geometryIndex = global::geometryIndex++;
        geometry->dynamic = dynamic;

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;

        std::string warn;
        std::string err;

        bool res = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, std::string(path).c_str());

        if (!warn.empty()) {
//            KF_WARN( warn );
        }

        if (!err.empty()) {
            throw std::runtime_error("Failed to load model");
        }

        if (!res) {
            throw std::runtime_error("Failed to load model");
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices;

//        bool firstRun = true;

        // Size of matIndex should equal the amount of triangles of the entire object (all sub-meshes)
        size_t totalAmountOfTriangles = 0;
        geometry->subMeshCount = shapes.size();

        for (const auto &shape : shapes) {
            totalAmountOfTriangles += shape.mesh.num_face_vertices.size();
        }

        geometry->matIndex.reserve(totalAmountOfTriangles);

        // Assuming that any given model does assign a material per sub-mesh
        global::materials.reserve(shapes.size());

        // Loop over shapes.
        for (const auto &shape : shapes) {
            // Set up the material.
            Material mat;

            int materialIndex = shape.mesh.material_ids[0];
            // This shape (sub-mesh) has a material.
            if (materialIndex != -1) {
                auto diffuse = materials[materialIndex].diffuse;
                mat.kd = glm::vec3(diffuse[0], diffuse[1], diffuse[2]);
                auto emission = materials[materialIndex].emission;
                mat.emission = glm::vec3(emission[0], emission[1], emission[2]);
                mat.illum = static_cast<uint32_t>( materials[materialIndex].illum );
                mat.d = materials[materialIndex].dissolve;
                if (mat.d < 1.0F) {
                    geometry->isOpaque = false;
                }
                mat.ns = materials[materialIndex].shininess;
                mat.ni = materials[materialIndex].ior;
                mat.fuzziness = materials[materialIndex].roughness;
                // @todo Add relative path here instead of inside the .mtl file.
                mat.diffuseTexPath = materials[materialIndex].diffuse_texname;

                bool found = false;
                size_t materialIndex = 0;
                for (size_t i = 0; i < global::materials.size(); ++i) {
                    if (mat == global::materials[i]) {
                        found = true;
                        materialIndex = i;
                        break;
                    }
                }

                // The material is new and unique
                if (!found) {
                    global::materials.push_back(mat);

                    // Increment the material as the next material will be a new one and, thus, will need a new index.
                    ++global::materialIndex;

                    for (size_t i = 0; i < shape.mesh.num_face_vertices.size(); ++i) {
                        geometry->matIndex.push_back(global::materialIndex - 1);
                    }
                }
                    // The material already exists, so let's reuse it
                else {
                    for (size_t i = 0; i < shape.mesh.num_face_vertices.size(); ++i) {
                        geometry->matIndex.push_back(materialIndex);
                    }
                }
            }
                // There is no material. Use the highest value as its index. Surely nobody will ever access the buffer at this index ... right?
            else {
                for (size_t i = 0; i < shape.mesh.num_face_vertices.size(); ++i) {
                    geometry->matIndex.push_back(std::numeric_limits<uint32_t>::max());
                }
            }

            for (const auto &index : shape.mesh.indices) {
                Vertex vertex = {};
                if (static_cast<int>(attrib.vertices.size()) > 3 * index.vertex_index + 0) {
                    vertex.pos.x = attrib.vertices[3 * index.vertex_index + 0];
                    vertex.pos.y = attrib.vertices[3 * index.vertex_index + 1];
                    vertex.pos.z = attrib.vertices[3 * index.vertex_index + 2];
                }

                if (static_cast<int>(attrib.normals.size()) > 3 * index.normal_index + 0) {
                    vertex.normal.x = attrib.normals[3 * index.normal_index + 0];
                    vertex.normal.y = attrib.normals[3 * index.normal_index + 1];
                    vertex.normal.z = attrib.normals[3 * index.normal_index + 2];
                }

                if (static_cast<int>(attrib.texcoords.size()) > 2 * index.texcoord_index + 1) {
                    vertex.texCoord.x = attrib.texcoords[2 * index.texcoord_index + 0];
                    vertex.texCoord.y = 1.0F - attrib.texcoords[2 * index.texcoord_index + 1];
                }

                if (static_cast<int>(attrib.colors.size()) > 3 * index.vertex_index + 2) {
                    vertex.color.x = attrib.colors[3 * index.vertex_index + 0];
                    vertex.color.y = attrib.colors[3 * index.vertex_index + 1];
                    vertex.color.z = attrib.colors[3 * index.vertex_index + 2];
                }

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>( geometry->vertices.size());
                    geometry->vertices.push_back(vertex);
                }

                geometry->indices.push_back(uniqueVertices[vertex]);
            }
        }

        return geometry;
    }

    void Geometry::setMaterial(const Material &material) {
        if (material.d < 1.0F) {
            isOpaque = false;
        } else {
            isOpaque = true;
        }

        global::materials.push_back(material);
        global::materialIndex++;

        for (auto &it : matIndex) {
            it = global::materialIndex - 1;
        }
    }

    std::shared_ptr<GeometryInstance> instance(std::shared_ptr<Geometry> geometry, const glm::mat4 &transform) {
        assert(geometry != nullptr);

        std::shared_ptr<GeometryInstance> result = std::make_shared<GeometryInstance>();
        result->geometryIndex = geometry->geometryIndex;
        result->transform = transform;
        // result->transformIT                      = glm::transpose( glm::inverse( transform ) );

        return result;
    }

    void GeometryInstance::setTransform(const glm::mat4 &transform) {
        this->transform = transform;
        //transformIT     = glm::transpose( glm::inverse( transform ) );
    }
}
