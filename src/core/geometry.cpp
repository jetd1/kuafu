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
               (m1.shininess == m2.shininess) &&
               (m1.ior == m2.ior) &&
               (m1.roughness == m2.roughness);
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
            KF_WARN(warn);
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
        global::materials.reserve(global::materials.size() + shapes.size());

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
                mat.shininess = materials[materialIndex].shininess;
                mat.ior = materials[materialIndex].ior;
                mat.roughness = materials[materialIndex].roughness;
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

    // From optifuser by Fanbo
    void Geometry::recalculateNormals() {
        for (auto &v : vertices) {
            v.normal = glm::vec3(0);
        }
        for (size_t i = 0; i < indices.size(); i += 3) {
            unsigned int i1 = indices[i];
            unsigned int i2 = indices[i + 1];
            unsigned int i3 = indices[i + 2];
            Vertex &v1 = vertices[i1];
            Vertex &v2 = vertices[i2];
            Vertex &v3 = vertices[i3];

            glm::vec3 normal = glm::normalize(
                    glm::cross(v2.pos - v1.pos, v3.pos - v1.pos));
            if (std::isnan(normal.x)) {
                continue;
            }
            v1.normal += normal;
            v2.normal += normal;
            v3.normal += normal;
        }

        for (auto& v: vertices) {
            if (v.normal == glm::vec3(0))
                continue;
            v.normal = glm::normalize(v.normal);
        }
    }

    std::shared_ptr<Geometry> createYZPlane(bool dynamic, std::shared_ptr<Material> mat) {
        auto ret = std::make_shared<Geometry>();
        ret->vertices = {
                {.pos = {0, 1, 1}, .normal = {1, 0, 0}, .texCoord = {1, 0}},
                {.pos = {0, -1, 1}, .normal = {1, 0, 0}, .texCoord = {0, 0}},
                {.pos = {0, -1, -1}, .normal = {1, 0, 0}, .texCoord = {1, 1}},
                {.pos = {0, 1, -1}, .normal = {1, 0, 0}, .texCoord = {0, 1}}
        };
        ret->indices = {
                0, 1, 2,
                0, 2, 3
        };
        ret->matIndex = std::vector<uint32_t>(ret->indices.size(), kuafu::global::materialIndex);

        kuafu::global::materials.push_back(*mat);  // copy
        ++kuafu::global::materialIndex;            // TODO: check existing dup mat

        ret->geometryIndex = kuafu::global::geometryIndex++;
        ret->subMeshCount = 1;
        ret->path = "";
        ret->initialized = false;
        ret->dynamic = dynamic;
        ret->isOpaque = (mat->d >= 1.0F);
        return ret;
    }

    std::shared_ptr<Geometry> createCube(bool dynamic, std::shared_ptr<Material> mat) {
        auto ret = std::make_shared<Geometry>();

        ret->vertices = {
                {.pos = {-1, 1, -1}, .normal = {0, 1, 0}, .texCoord = {0.875, 0.5}},
                {.pos = {1, 1, 1}, .normal = {0, 1, 0}, .texCoord = {0.625, 0.25}},
                {.pos = {1, 1, -1}, .normal = {0, 1, 0}, .texCoord = {0.625, 0.5}},
                {.pos = {1, 1, 1}, .normal = {0, 0, 1}, .texCoord = {0.625, 0.25}},
                {.pos = {-1, -1, 1}, .normal = {0, 0, 1}, .texCoord = {0.375, 0}},
                {.pos = {1, -1, 1}, .normal = {0, 0, 1}, .texCoord = {0.375, 0.25}},
                {.pos = {-1, 1, 1}, .normal = {-1, 0, 0}, .texCoord = {0.625, 1}},
                {.pos = {-1, -1, -1}, .normal = {-1, 0, 0}, .texCoord = {0.375, 0.75}},
                {.pos = {-1, -1, 1}, .normal = {-1, 0, 0}, .texCoord = {0.375, 1}},
                {.pos = {1, -1, -1}, .normal = {0, -1, 0}, .texCoord = {0.375, 0.5}},
                {.pos = {-1, -1, 1}, .normal = {0, -1, 0}, .texCoord = {0.125, 0.25}},
                {.pos = {-1, -1, -1}, .normal = {0, -1, 0}, .texCoord = {0.125, 0.5}},
                {.pos = {1, 1, -1}, .normal = {1, 0, 0}, .texCoord = {0.625, 0.5}},
                {.pos = {1, -1, 1}, .normal = {1, 0, 0}, .texCoord = {0.375, 0.25}},
                {.pos = {1, -1, -1}, .normal = {1, 0, 0}, .texCoord = {0.375, 0.5}},
                {.pos = {-1, 1, -1}, .normal = {0, 0, -1}, .texCoord = {0.625, 0.75}},
                {.pos = {1, -1, -1}, .normal = {0, 0, -1}, .texCoord = {0.375, 0.5}},
                {.pos = {-1, -1, -1}, .normal = {0, 0, -1}, .texCoord = {0.375, 0.75}},
                {.pos = {-1, 1, 1}, .normal = {0, 1, 0}, .texCoord = {0.875, 0.25}},
                {.pos = {-1, 1, 1}, .normal = {0, 0, 1}, .texCoord = {0.625, 0}},
                {.pos = {-1, 1, -1}, .normal = {-1, 0, 0}, .texCoord = {0.625, 0.75}},
                {.pos = {1, -1, 1}, .normal = {0, -1, 0}, .texCoord = {0.375, 0.25}},
                {.pos = {1, 1, 1}, .normal = {1, 0, 0}, .texCoord = {0.625, 0.25}},
                {.pos = {1, 1, -1}, .normal = {0, 0, -1}, .texCoord = {0.625, 0.5}},
        };
        ret->indices = {
                0, 1, 2,
                3, 4, 5,
                6, 7, 8,
                9, 10, 11,
                12, 13, 14,
                15, 16, 17,
                0, 18, 1,
                3, 19, 4,
                6, 20, 7,
                9, 21 ,10,
                12, 22, 13,
                15, 23, 16
        };
        ret->matIndex = std::vector<uint32_t>(ret->indices.size(), kuafu::global::materialIndex);

        kuafu::global::materials.push_back(*mat);  // copy
        ++kuafu::global::materialIndex;            // TODO: check existing dup mat

        ret->geometryIndex = kuafu::global::geometryIndex++;
        ret->subMeshCount = 1;
        ret->path = "";
        ret->initialized = false;
        ret->dynamic = dynamic;
        ret->isOpaque = (mat->d >= 1.0F);
        return ret;
    }

    // Modified from optifuser by Fanbo
    std::shared_ptr<Geometry> createSphere(bool dynamic, std::shared_ptr<Material> mat) {
        auto ret = std::make_shared<Geometry>();

        uint32_t stacks = 50;
        uint32_t slices = 50;
        float radius = 1.f;

        for (uint32_t i = 1; i < stacks; ++i) {
            float phi = glm::pi<float>() / stacks * i - glm::pi<float>() / 2;
            for (uint32_t j = 0; j < slices; ++j) {
                float theta = glm::pi<float>() * 2 / slices * j;
                float x = sinf(phi) * radius;
                float y = cosf(theta) * cosf(phi) * radius;
                float z = sinf(theta) * cosf(phi) * radius;
                ret->vertices.push_back({.pos = {x, y, z}});
            }
        }

        for (uint32_t i = 0; i < (stacks - 2) * slices; ++i) {
            uint32_t right = (i + 1) % slices + i / slices * slices;
            uint32_t up = i + slices;
            uint32_t rightUp = right + slices;

            ret->indices.push_back(i);
            ret->indices.push_back(rightUp);
            ret->indices.push_back(up);

            ret->indices.push_back(i);
            ret->indices.push_back(right);
            ret->indices.push_back(rightUp);
        }

        ret->vertices.push_back({.pos = {-radius, 0, 0}});
        ret->vertices.push_back({.pos = {radius, 0, 0}});

        for (uint32_t i = 0; i < slices; ++i) {
            uint32_t right = (i + 1) % slices + i / slices * slices;
            ret->indices.push_back(ret->vertices.size() - 2);
            ret->indices.push_back(right);
            ret->indices.push_back(i);
        }
        for (uint32_t i = (stacks - 2) * slices; i < (stacks - 1) * slices; ++i) {
            uint32_t right = (i + 1) % slices + i / slices * slices;
            ret->indices.push_back(ret->vertices.size() - 1);
            ret->indices.push_back(i);
            ret->indices.push_back(right);
        }

        ret->matIndex = std::vector<uint32_t>(ret->indices.size(), kuafu::global::materialIndex);
        kuafu::global::materials.push_back(*mat);  // copy
        ++kuafu::global::materialIndex;            // TODO: check existing dup mat

        ret->geometryIndex = kuafu::global::geometryIndex++;
        ret->subMeshCount = 1;
        ret->path = "";
        ret->initialized = false;
        ret->dynamic = dynamic;
        ret->isOpaque = (mat->d >= 1.0F);

        ret->recalculateNormals();

        return ret;
    }

    // Modified from optifuser by Fanbo
    std::shared_ptr<Geometry> createCapsule(
            float halfHeight, float radius, bool dynamic, std::shared_ptr<Material> mat) {
        auto ret = std::make_shared<Geometry>();

        std::vector<Vertex> vertices1;
        std::vector<Vertex> vertices2;
        std::vector<uint32_t> indices1;
        std::vector<uint32_t> indices2;

        uint32_t stacks = 10;
        uint32_t slices = 20;

        for (uint32_t i = 0; i < stacks; ++i) {
            float phi = glm::pi<float>() / 2 / stacks * i;
            for (uint32_t j = 0; j < slices; ++j) {
                float theta = glm::pi<float>() * 2 / slices * j;
                float x = sinf(phi) * radius;
                float y = cosf(theta) * cosf(phi) * radius;
                float z = sinf(theta) * cosf(phi) * radius;
                vertices1.push_back({.pos = {x + halfHeight, y, z}});
                vertices2.push_back({.pos = {-x - halfHeight, y, z}});
            }
        }
        vertices1.push_back({.pos = {radius + halfHeight, 0, 0}});
        vertices2.push_back({.pos = {-radius - halfHeight, 0, 0}});

        for (uint32_t i = 0; i < (stacks - 1) * slices; ++i) {
            uint32_t right = (i + 1) % slices + i / slices * slices;
            uint32_t up = i + slices;
            uint32_t rightUp = right + slices;

            indices1.push_back(i);
            indices1.push_back(rightUp);
            indices1.push_back(up);

            indices1.push_back(i);
            indices1.push_back(right);
            indices1.push_back(rightUp);

            indices2.push_back(i);
            indices2.push_back(up);
            indices2.push_back(rightUp);

            indices2.push_back(i);
            indices2.push_back(rightUp);
            indices2.push_back(right);
        }

        for (uint32_t i = 0; i < slices; ++i) {
            uint32_t curr = (stacks - 1) * slices + i;
            uint32_t right = (curr + 1) % slices + curr / slices * slices;
            uint32_t up = vertices1.size() - 1;

            indices1.push_back(curr);
            indices1.push_back(right);
            indices1.push_back(up);

            indices2.push_back(curr);
            indices2.push_back(up);
            indices2.push_back(right);
        }

        ret->vertices = vertices1;
        ret->vertices.insert(ret->vertices.end(), vertices2.begin(), vertices2.end());

        ret->indices = indices1;
        ret->indices.reserve(indices1.size() + indices2.size());
        for (auto i : indices2)
            ret->indices.push_back(vertices1.size() + i);

        for (uint32_t i = 0; i < slices; ++i) {
            uint32_t right = (i + 1) % slices;
            uint32_t up = i + vertices1.size();
            uint32_t rightUp = right + vertices1.size();

            ret->indices.push_back(i);
            ret->indices.push_back(up);
            ret->indices.push_back(rightUp);

            ret->indices.push_back(i);
            ret->indices.push_back(rightUp);
            ret->indices.push_back(right);
        }

        ret->matIndex = std::vector<uint32_t>(ret->indices.size(), kuafu::global::materialIndex);
        kuafu::global::materials.push_back(*mat);  // copy
        ++kuafu::global::materialIndex;            // TODO: check existing dup mat

        ret->geometryIndex = kuafu::global::geometryIndex++;
        ret->subMeshCount = 1;
        ret->path = "";
        ret->initialized = false;
        ret->dynamic = dynamic;
        ret->isOpaque = (mat->d >= 1.0F);

        ret->recalculateNormals();

        return ret;
    }
}
