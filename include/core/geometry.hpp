//
// By Jet <i@jetd.me> 2021.
//
#pragma once

#include "core/context/vertex.hpp"

namespace kuafu {

//// Simplified Blender PrincipledBSDF
struct NiceMaterial {
    glm::vec3 diffuseColor = glm::vec3(1.0F); /// Diffuse color
    float alpha = 1.0F;

    std::string diffuseTexPath;
    std::string metallicTexPath;
    std::string roughnessTexPath;
    std::string transmissionTexPath;

    float metallic = 0.0F;
    float specular = 0.5F;
    float roughness = 0.5F;
    float ior = 1.4F;
    float transmission = 0.0F;

    glm::vec3 emission = glm::vec3(1.0F);
    float emissionStrength = 0.0F;

    friend bool operator==(const NiceMaterial &m1, const NiceMaterial &m2);
};

struct Geometry {
    void setMaterial(const NiceMaterial &material);

    void recalculateNormals();

    std::vector<Vertex> vertices;   ///< Contains all vertices of the geometry.
    std::vector<uint32_t> indices;  ///< Contains all indices of the geometry.
    std::vector<uint32_t> matIndex; ///< Contains all sub-meshes and their respective materials.
    std::string path;         ///< The model's path, relative to the path to assets.
    bool initialized = false; ///< Keeps track of whether or not the geometry was initialized.

    bool dynamic = false;     ///< Keeps track of whether or not the geometry is dynamic or static. // TODO: use this field
    bool isOpaque = true;
    bool hideRender = false;
};

struct GeometryInstance {
    void setTransform(const glm::mat4 &transform);

    glm::mat4 transform = glm::mat4(1.0F); ///< The instance's world transform matrix.
    int geometryIndex = -1; ///< Used to assign this instance a model.
    std::shared_ptr<Geometry> geometry = nullptr;
};

std::vector<std::shared_ptr<Geometry>> loadScene(std::string_view fname, bool dynamic);
std::shared_ptr<Geometry> loadObj(std::string_view path, bool dynamic = false);

/// A commodity function for allocating an instance from a given geometry and set its matrices.
///
/// @param geometry The geometry to create the instance from.
/// @param transform The world transform matrix of the instance.
/// @return Returns a pointer to a geometry instance.
/// @ingroup BASE
std::shared_ptr<GeometryInstance>
instance(const std::shared_ptr<Geometry>& geometry, const glm::mat4 &transform = glm::mat4(1.0F));

/// A wrapper for MeshSSBO matching the buffer alignment requirements.
/// @ingroup API
struct NiceMaterialSSBO {
    glm::vec4 diffuse = glm::vec4(1.0F, 1.0F, 1.0F, 0.0F); ///< vec3 diffuse  + padding
    glm::vec4 emission = glm::vec4(1.0F, 1.0F, 1.0F, 0.0F); ///< vec3 emission + vec1 emission strength
    float alpha = 1.0F;
    float metallic = 0.0F;
    float specular = 0.5F;
    float roughness = 0.5F;
    float ior = 1.4F;
    float transmission = 0.0F;

    int diffuseTexIdx = -1;
    int metallicTexIdx = -1;
    int roughnessTexIdx = -1;
    int transmissionTexIdx = -1;

    int padding0 = 0;
    int padding1 = 0;
};

/// A wrapper for GeometryInstanceSSBO matching the buffer alignment requirements.
/// @ingroup API
struct GeometryInstanceSSBO {
    glm::mat4 transform = glm::mat4(1.0F); ///< The instance's world transform matrix.
    //glm::mat4 padding   = glm::mat4( 1.0F ); ///< The inverse transpose of transform.
    uint32_t geometryIndex = 0;

    uint32_t padding0 = 0;
    uint32_t padding1 = 0;
    uint32_t padding2 = 0;
};

std::shared_ptr<Geometry> createYZPlane(bool dynamic = true, NiceMaterial mat = {});

std::shared_ptr<Geometry> createCube(bool dynamic = true, NiceMaterial mat = {});

std::shared_ptr<Geometry> createSphere(bool dynamic = true, NiceMaterial mat = {});

std::shared_ptr<Geometry> createCapsule(
        float halfHeight = 1., float radius = 1., bool dynamic = true, NiceMaterial mat = {});
}
