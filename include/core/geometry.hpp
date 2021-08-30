//
// Created by jet on 4/9/21.
//

#pragma once

#include "core/context/vertex.hpp"

namespace kuafu {
    /// Describes the rendering properties of a mesh.
    ///
    /// Property descriptions copied from https://www.loc.gov/preservation/digital/formats/fdd/fdd000508.shtml.
    /// @ingroup BASE
    struct Material {
        glm::vec3 kd = glm::vec3(0.0F); /// Diffuse color
        std::string diffuseTexPath = "";

        glm::vec3 emission = glm::vec3(0.0F);

        /// Illumination model.
        /// @todo documentation
        uint32_t illum = 2;

        /// Specifies a factor for dissolve, how much this material dissolves into the background. A factor of 1.0 is fully opaque. A factor of 0.0 is completely transparent.
        float d = 1.0F;

        /// Focus of the specular light (aka shininess). Ranges from 0 to 1000, with a high value resulting in a tight, concentrated highlight.
        float shininess = 0.0F;

        float roughness = 1000.0F;

        /// Optical density (aka index of refraction). Ranges from 0.001 to 10. A value of 1.0 means that light does not bend as it passes through an object.
        float ior = 1.4F;

        friend bool operator==(const Material &m1, const Material &m2);
    };

    /// Describes a geometry Kuafu can render.
    ///
    /// Even if a model consists out of multiple sub-meshes, all vertices and indices must be stored together in their respective containers.
    /// @warning geometryIndex must be incremented everytime a new model is created.
    /// @ingroup BASE
    struct Geometry {
        void setMaterial(const Material &material);
        void recalculateNormals();

        std::vector<Vertex> vertices;   ///< Contains all vertices of the geometry.
        std::vector<uint32_t> indices;  ///< Contains all indices of the geometry.
        std::vector<uint32_t> matIndex; ///< Contains all sub-meshes and their respective materials.
        uint32_t geometryIndex = 0;     ///< A unique index required by the acceleration structures.
        size_t subMeshCount = 0;
        std::string path = "";    ///< The model's path, relative to the path to assets.
        bool initialized = false; ///< Keeps track of whether or not the geometry was initialized.
        bool dynamic = false; ///< Keeps track of whether or not the geometry is dynamic or static.
        bool isOpaque = true;
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
    instance(std::shared_ptr<Geometry> geometry, const glm::mat4 &transform = glm::mat4(1.0F));

    /// A wrapper for MeshSSBO matching the buffer alignment requirements.
    /// @ingroup API
    struct MaterialSSBO {
        glm::vec4 diffuse = glm::vec4(1.0F, 1.0F, 1.0F, -1.0F); ///< vec3 diffuse  + vec1 texture index
        glm::vec4 emission = glm::vec4(0.0F, 0.0F, 0.0F, 64.0F); ///< vec3 emission + vec1 shininess

        uint32_t illum = 0;
        float dissolve = 1.0F;
        float roughness = 0.0F;
        float ior = 1.0F;
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

    std::shared_ptr<Geometry> createYZPlane(bool dynamic = true, std::shared_ptr<Material> mat = nullptr);
    std::shared_ptr<Geometry> createCube(bool dynamic = true, std::shared_ptr<Material> mat = nullptr);
    std::shared_ptr<Geometry> createSphere(bool dynamic = true, std::shared_ptr<Material> mat = nullptr);
    std::shared_ptr<Geometry> createCapsule(
            float halfHeight = 1., float radius = 1., bool dynamic = true, std::shared_ptr<Material> mat = nullptr);


}
