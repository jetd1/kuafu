//
// Modified by Jet <i@jetd.me> based on Rayex source code.
// Original copyright notice:
//
// Copyright (c) 2021 Christian Hilpert
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the author be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose
// and to alter it and redistribute it freely, subject to the following
// restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
#pragma once

#include "core/context/vertex.hpp"

namespace kuafu {

//// Simplified Blender PrincipledBSDF
struct NiceMaterial {
    glm::vec3 diffuseColor = glm::vec3(1.0F); /// Diffuse color
    float alpha = 1.0F;

    std::string diffuseTexPath;

    float metallic = 0.0F;
    float specular = 0.5F;
    float roughness = 0.5F;
    float ior = 1.4F;
    float transmission = 0.0F;


    glm::vec3 emission = glm::vec3(1.0F);
    float emissionStrength = 0.0F;


    friend bool operator==(const NiceMaterial &m1, const NiceMaterial &m2);
};

/// Describes a geometry Kuafu can render.
///
/// Even if a model consists out of multiple sub-meshes, all vertices and indices must be stored together in their respective containers.
/// @warning geometryIndex must be incremented everytime a new model is created.
/// @ingroup BASE
struct Geometry {
    void setMaterial(const NiceMaterial &material);

    void recalculateNormals();

    std::vector<Vertex> vertices;   ///< Contains all vertices of the geometry.
    std::vector<uint32_t> indices;  ///< Contains all indices of the geometry.
    std::vector<uint32_t> matIndex; ///< Contains all sub-meshes and their respective materials.
    uint32_t geometryIndex = 0;     ///< A unique index required by the acceleration structures.
    std::string path;         ///< The model's path, relative to the path to assets.
    bool initialized = false; ///< Keeps track of whether or not the geometry was initialized.

    bool dynamic = false;     ///< Keeps track of whether or not the geometry is dynamic or static. // TODO: use this field
    bool isOpaque = true;
    bool hideRender = false;
};

/// Describes an instance of some geometry.
/// @warning To assign a specific geometry to an instance, both must have the same value for geometryIndex.
/// @ingroup BASE
struct GeometryInstance {
    void setTransform(const glm::mat4 &transform);

    glm::mat4 transform = glm::mat4(1.0F); ///< The instance's world transform matrix.
    //glm::mat4 transformIT  = glm::mat4( 1.0F ); ///< The inverse transpose of transform.
    uint32_t geometryIndex = 0; ///< Used to assign this instance a model.
};

std::vector<std::shared_ptr<Geometry> > loadScene(std::string_view fname, bool dynamic);

/// A commodity function for loading a wavefront (.obj) model file and allocate a geometry object from it.
///
/// The function will attempt to find sub-meshes in the file and retrieve all materials.
/// A user is encouraged to create their own model loader function or classes.
/// @param path The model's path, relative to the path to assets.
/// @param dynamic If true, the geometry can be animated. Otherwise the geometry is static throughout its entire lifetime.
/// @return Returns a pointer to a geometry object.
/// @ingroup BASE
std::shared_ptr<Geometry> loadObj(std::string_view path, bool dynamic = false);

/// A commodity function for allocating an instance from a given geometry and set its matrices.
///
/// The function will also automatically set the inverse transpose matrix.
/// @param geometry The geometry to create the instance from.
/// @param transform The world transform matrix of the instance.
/// @return Returns a pointer to a geometry instance.
/// @ingroup BASE
std::shared_ptr<GeometryInstance>
instance(const std::shared_ptr<Geometry>& geometry, const glm::mat4 &transform = glm::mat4(1.0F));

/// A wrapper for MeshSSBO matching the buffer alignment requirements.
/// @ingroup API
struct NiceMaterialSSBO {
    glm::vec4 diffuse = glm::vec4(1.0F, 1.0F, 1.0F, 0.0F); ///< vec3 diffuse  + hasTex
    glm::vec4 emission = glm::vec4(1.0F, 1.0F, 1.0F, 0.0F); ///< vec3 emission + vec1 emission strength
    float alpha = 1.0F;
    float metallic = 0.0F;
    float specular = 0.5F;
    float roughness = 0.5F;
    float ior = 1.4F;
    float transmission = 0.0F;

    uint32_t texIdx = 0;
    uint32_t padding0 = 0;
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
