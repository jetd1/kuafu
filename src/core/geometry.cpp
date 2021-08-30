//
// By Jet <i@jetd.me> 2021.
//
#include "core/geometry.hpp"
#include "core/context/global.hpp"
#include <assimp/Importer.hpp>
#include <assimp/pbrmaterial.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

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

std::vector<std::shared_ptr<Geometry> > loadScene(
        std::string_view fname, bool dynamic) {
    auto path = std::filesystem::absolute(fname);

    Assimp::Importer importer;
    uint32_t flags = aiProcess_CalcTangentSpace | aiProcess_Triangulate |
                     aiProcess_GenNormals | aiProcess_FlipUVs |
                     aiProcess_PreTransformVertices;
    importer.SetPropertyInteger(AI_CONFIG_PP_PTV_ADD_ROOT_TRANSFORMATION, 1);
    const aiScene *scene = importer.ReadFile(path, flags);

    if (!scene)
        throw std::runtime_error(
                "Failed to load scene: " + std::string(importer.GetErrorString()) +
                ", " + path.string());
    if (scene->mRootNode->mMetaData) {
        KF_ERROR("Failed to load mesh file: file contains unsupported metadata, " +
                 path.string());
    }


    auto parentDir = path.parent_path();

//    const uint32_t MIP_LEVEL = 3;

    std::vector<std::shared_ptr<Geometry>> geometries;
    std::vector<Material> materials;

    for (uint32_t mat_idx = 0; mat_idx < scene->mNumMaterials; ++mat_idx) {
        auto *m = scene->mMaterials[mat_idx];
        aiColor3D diffuse{0, 0, 0};
        aiColor3D specular{0, 0, 0};
        aiColor3D emission{0, 0, 0};
        float alpha = 1.f;
        float shininess = 0.f;
        float ior = 0.f;
        float transmission = 0.f;
        uint32_t illum = 2;
        m->Get(AI_MATKEY_OPACITY, alpha);
        m->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
        m->Get(AI_MATKEY_COLOR_SPECULAR, specular);
        m->Get(AI_MATKEY_SHININESS, shininess);         // TODO: strength
        m->Get(AI_MATKEY_COLOR_EMISSIVE, emission);
        m->Get(AI_MATKEY_REFRACTI, ior);
//      m->Get(AI_MATKEY_GLTF_MATERIAL_TRANSMISSION, transmission); // TODO: full glTF

        if (alpha < 1e-5 && (fname.ends_with("dae") ||    // TODO: dAE?
                             fname.ends_with("DAE"))) {
            KF_WARN("The DAE file " + path.string() +
                    " is fully transparent. This is probably "
                    "due to modeling error. Setting opacity to 1 instead...");
            alpha = 1.f;
        }

        float roughness;
        if (shininess <= 5.f)
            roughness = 1.f;
        else if (shininess >= 1605.f)
            roughness = 0.f;
        else
            roughness = 1.f - (std::sqrt(shininess - 5.f) * 0.025f);

        if (transmission > 0)
            illum = 1;

//      std::shared_ptr<SVTexture> baseColorTexture{};
//      std::shared_ptr<SVTexture> normalTexture{};
//      std::shared_ptr<SVTexture> roughnessTexture{};
//      std::shared_ptr<SVTexture> metallicTexture{};

        aiString tpath;
        std::string diffuseTexPath;
        if (m->GetTextureCount(aiTextureType_DIFFUSE) > 0 &&
            m->GetTexture(aiTextureType_DIFFUSE, 0, &tpath) == AI_SUCCESS) {
            KF_INFO("Trying to load texture {}", tpath.C_Str());
            if (auto texture = scene->GetEmbeddedTexture(tpath.C_Str())) {
                KF_ERROR("embedded texture not supported");
            } else {
                std::string p = std::string(tpath.C_Str());
                if (!std::filesystem::path(p).is_absolute())
                    p = (parentDir / p).string();
                diffuseTexPath = p;
            }
        }

        if (m->GetTextureCount(aiTextureType_METALNESS) > 0) {
            KF_WARN("metalness texture not supported");
        }

        if (m->GetTextureCount(aiTextureType_NORMALS) > 0) {
            KF_WARN("normals texture not supported");
        }

        if (m->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) > 0) {
            KF_WARN("diffuse roughness texture not supported");
        }

        if (m->GetTexture(
                AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE,
                &tpath) == AI_SUCCESS) {
            KF_WARN("roughness metallic texture not supported");
        }

        materials.push_back({
                                    .kd = {diffuse.r, diffuse.g, diffuse.b},
                                    .diffuseTexPath = std::move(diffuseTexPath),
                                    .emission = {emission.r, emission.g, emission.b},  // TODO: strength
                                    .illum = illum,
                                    .d = alpha,
                                    .shininess = shininess,
                                    .roughness = roughness,
                                    .ior = ior,
                            });
    }

    // Upload materials to global resources
    std::vector<uint32_t> matLocal2GlobalIdx(
            materials.size(), std::numeric_limits<uint32_t>::max());
    for (size_t i = 0; i < materials.size(); i++) {
        // lookup dups
        bool found = false;
        size_t materialIndex = 0;
        for (size_t j = 0; j < global::materials.size(); j++) {
            if (materials[i] == global::materials[j]) {
                found = true;
                materialIndex = j;
                break;
            }
        }

        if (found)
            matLocal2GlobalIdx[i] = materialIndex;
        else {
            global::materials.push_back(std::move(materials[i]));
            matLocal2GlobalIdx[i] = global::materialIndex++;
        }
    }

    // Meshes
    for (uint32_t mesh_idx = 0; mesh_idx < scene->mNumMeshes; ++mesh_idx) {
        auto mesh = scene->mMeshes[mesh_idx];
        if (!mesh->HasFaces())
            continue;

        // Vertices
        std::vector<Vertex> vertices;
        vertices.resize(mesh->mNumVertices);

        for (uint32_t v = 0; v < mesh->mNumVertices; v++) {
            vertices[v].pos = {mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z};
            if (mesh->HasNormals())
                vertices[v].normal = {mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z};
            if (mesh->HasTextureCoords(0))
                vertices[v].texCoord = {mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y};
            if (mesh->HasVertexColors(0))
                vertices[v].color = {mesh->mColors[0][v].r, mesh->mColors[0][v].g, mesh->mColors[0][v].b};
        }

        // Indices
        std::vector<uint32_t> indices;
        for (uint32_t f = 0; f < mesh->mNumFaces; f++) {
            auto face = mesh->mFaces[f];
            if (face.mNumIndices != 3) {
                KF_WARN("Mesh not triangulated!");
                continue;
            }
            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);
        }

        if (mesh->mNumVertices == 0 || indices.size() == 0) {
            KF_WARN("A mesh in the file has no triangles: " + path.string());
            continue;
        }


        // Create Geometry
        std::shared_ptr<Geometry> geometry = std::make_shared<Geometry>();
        geometry->path = path;
        geometry->geometryIndex = global::geometryIndex++;
        geometry->dynamic = dynamic;
        geometry->vertices = vertices;
        geometry->indices = indices;
        geometry->subMeshCount = 1;
        geometry->initialized = false;
        geometry->matIndex = std::vector<uint32_t>(
                indices.size() / 3, matLocal2GlobalIdx[mesh->mMaterialIndex]);
        geometry->isOpaque = global::materials[geometry->matIndex.front()].d >= 1.F;

        // Add to ret
        geometries.push_back(std::move(geometry));
    }

    return geometries;
}


std::shared_ptr<Geometry> loadObj(std::string_view path, bool dynamic) {
    return loadScene(path, dynamic).front();
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

    for (auto &v: vertices) {
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
            9, 21, 10,
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

// Modified from svulkan2 by Fanbo
// TODO: avoid copies
std::shared_ptr<Geometry> createCapsule(
        float halfLength, float radius, bool dynamic, std::shared_ptr<Material> mat) {
    auto ret = std::make_shared<Geometry>();

    int segments = 32;
    int halfRings = 8;

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<glm::ivec3> indices;

    for (int s = 0; s < segments; ++s) {
        vertices.emplace_back(radius + halfLength, 0.f, 0.f);
        normals.emplace_back(1.f, 0.f, 0.f);
        uvs.emplace_back((0.5f + s) / segments, 1.f);
    }
    int rings = 2 * halfRings;
    for (int r = 1; r <= halfRings; ++r) {
        float theta = glm::pi<float>() * r / rings;
        float x = glm::cos(theta);
        float yz = glm::sin(theta);
        for (int s = 0; s < segments + 1; ++s) {
            float phi = glm::pi<float>() * s * 2 / segments;
            float y = yz * glm::cos(phi);
            float z = yz * glm::sin(phi);
            vertices.emplace_back(glm::vec3{x, y, z} * radius +
                                  glm::vec3{halfLength, 0, 0});
            normals.emplace_back(x, y, z);
            uvs.emplace_back(static_cast<float>(s) / segments,
                             1.f - 0.5f * static_cast<float>(r) / rings);
        }
    }
    for (int r = halfRings; r < rings; ++r) {
        float theta = glm::pi<float>() * r / rings;
        float x = glm::cos(theta);
        float yz = glm::sin(theta);
        for (int s = 0; s < segments + 1; ++s) {
            float phi = glm::pi<float>() * s * 2 / segments;
            float y = yz * glm::cos(phi);
            float z = yz * glm::sin(phi);
            vertices.emplace_back(glm::vec3{x, y, z} * radius -
                                  glm::vec3{halfLength, 0, 0});
            normals.emplace_back(x, y, z);
            uvs.emplace_back(static_cast<float>(s) / segments,
                             0.5f - 0.5f * static_cast<float>(r) / rings);
        }
    }

    for (int s = 0; s < segments; ++s) {
        vertices.emplace_back(-radius - halfLength, 0.f, 0.f);
        normals.emplace_back(-1.f, 0.f, 0.f);
        uvs.emplace_back((0.5f + s) / segments, 0.f);
    }

    for (int s = 0; s < segments; ++s) {
        indices.emplace_back(s, s + segments, s + segments + 1);
    }

    for (int r = 0; r < rings - 1; ++r) {
        for (int s = 0; s < segments; ++s) {
            indices.emplace_back(
                    segments + (segments + 1) * r + s,
                    segments + (segments + 1) * (r + 1) + s,
                    segments + (segments + 1) * (r + 1) + s + 1);
            indices.emplace_back(
                    segments + (segments + 1) * r + s,
                    segments + (segments + 1) * (r + 1) + s + 1,
                    segments + (segments + 1) * r + s + 1);
        }
    }
    for (int s = 0; s < segments; ++s) {
        indices.emplace_back(
                segments + (segments + 1) * (rings - 1) + s,
                segments + (segments + 1) * (rings) + s,
                segments + (segments + 1) * (rings - 1) + s + 1);
    }

    auto nV = vertices.size();
    ret->vertices.resize(nV);
    for (size_t i = 0; i < nV; ++i) {
        ret->vertices[i].pos = vertices[i];
        ret->vertices[i].normal = normals[i];
        ret->vertices[i].texCoord = uvs[i];
    }

    auto nF = indices.size();
    ret->indices.resize(3 * nF);
    for (size_t i = 0; i < nF; ++i)
        for (int j = 0; j < 3; ++j)
            ret->indices[3 * i + j] = indices[i][j];

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
}
