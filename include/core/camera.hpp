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

#include "stdafx.hpp"

namespace kuafu {
/// A minimal camera implementation.
///
/// This class acts like an interface for the user by providing the most important camera-related matrices as well as the camera's position, which are required by the rendering API.
/// ### Example
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~.cpp
/// // This example requires the user to implement a custom camera class that inherits from Camera.
/// auto myCam = std::make_shared<CustomCamera>( 600, 500, glm::vec3{ 0.0f, 0.0f, 3.0f } );
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// @note The user has to handle keyboard related camera changes inside update().
/// @warning Do not forget to re-calculate the view or projection matrix if the camera or the window have changed. See updateViewMatrix(), updateProjectionMatrix() or updateView and updateProj respectively.
/// @ingroup BASE
class Camera {
public:
    /// @param width The width of the viewport.
    /// @param height The height of the viewport.
    /// @param position The position of your camera.
    Camera(int width, int height, const glm::vec3 &position = {0.0F, 0.0F, 3.0F});

    virtual ~Camera() = default;

    Camera(const Camera &) = default;

    Camera(const Camera &&) = delete;

    auto operator=(const Camera &) -> Camera & = default;

    auto operator=(const Camera &&) -> Camera & = delete;

    /// Is used to update camera vectors etc.
    ///
    /// The user has to override this function for the camera to work like intended.
    /// @note The function will be called every tick.
    virtual void update();

    void reset();

    /// @return Returns the camera's position.
    auto getPosition() const -> const glm::vec3 & { return mPosition; }

    /// @return Returns the camera's front vector / viewing direction.
    auto getFront() const -> const glm::vec3 & { return mDirFront; }

    /// Is used to change the camera's position.
    /// @param position The new camera position.
    void setPosition(const glm::vec3 &position);

    void setFront(const glm::vec3 &front);

    void setUp(const glm::vec3 &up);

    void setAperture(float aperture);

    inline float getAperture() const { return mAperture; }

    void setFocalLength(float focalLength);

    inline float getFocalLength() const { return mFocalLength; }

    /// Is used to set a size for the camera that fits the viewport dimensions.
    /// @param width The width of the viewport.
    /// @param height The height of the viewport.
    void setSize(int width, int height);

    /// Is used to set the camera's field of view.
    /// @param fov The new field of view.
    void setFov(float fov);

    void setPose(glm::mat4 pose);

    glm::mat4 getPose() const;

    [[nodiscard]] auto getWidth() const { return mWidth; }

    [[nodiscard]] auto getHeight() const { return mHeight; }

    [[nodiscard]] auto getFov() const { return mFov; }

    /// @return The view matrix.
    auto getViewMatrix() const -> const glm::mat4 & { return mViewMatrix; }

    /// @return The projection matrix.
    auto getProjectionMatrix() const -> const glm::mat4 & { return mProjMatrix; }

    /// @return The view matrix inversed.
    auto getViewInverseMatrix() const -> glm::mat4 { return glm::inverse(mViewMatrix); }

    /// @return The projection matrix inversed.
    auto getProjectionInverseMatrix() const -> glm::mat4 { return glm::inverse(mProjMatrix); }

    /// Re-calculates the camera's view matrix as well as the inversed view matrix.
    void updateViewMatrix();

    /// Re-calculates the camera's projection matrix as well as the inversed projection matrix.
    void updateProjectionMatrix();

    /// Processes mouse input (default implementation).
    /// @param xOffset The difference of the current offset on the x-axis and the previous offset.
    /// @param yOffset The difference of the current offset on the y-axis and the previous offset.
    virtual void processMouse(float xOffset, float yOffset);

    /// Used to handle user-defined keyboard inputs or input events.
    /// This function is called inside update() and followed up by a call to updateViewMatrix() and does not have to be called by the user.
    /// @note Implementation of this function should be inside a user-defined inherited class.
    virtual void processKeyboard() {}

    bool mViewNeedsUpdate = true; ///< Keeps track of whether or not to udpate the view matrix.
    bool mProjNeedsUpdate = true; ///< Keeps track of whether or not to udpate the projection matrix.

protected:
    int mWidth;  ///< The width of the viewport.
    int mHeight; ///< The height of the viewport.

    glm::vec3 mPosition; ///< The camera's position.

    glm::mat4 mViewMatrix = glm::mat4(1.0F); ///< The view matrix.
    glm::mat4 mProjMatrix = glm::mat4(1.0F); ///< The projection matrix

//        glm::mat4 _viewInverse = glm::mat4(1.0F); ///< The view matrix inversed.
//        glm::mat4 _projectionInverse = glm::mat4(1.0F); ///< The projection matrix inversed.

//        glm::vec3 _worldUp = {0.0F, 0.0F, 1.0F}; ///< The world up vector.
    glm::vec3 mDirUp = {0.0F, 0.0F, 1.0F};                  ///< The local up vector.
    glm::vec3 mDirRight = {0.0F, -1.0F, 0.0F};                  ///< The local right vector.
    glm::vec3 mDirFront = {1.0F, 0.0F, 0.0F};                  ///< The viewing direction.

//        float _yaw = -90.0F; ///< The yaw (left and right).
//        float _pitch = 0.0F;   ///< The pitch (down and up).
//        float _sensitivity = 0.06F;  ///< The mouse sensitivity.
    float mFov = 90.0F;  ///< The field of view.
    float mAperture = 0.0F;   // DOF disabled by default
    float mFocalLength = 5.0F;
};

/*
/// A memory aligned uniform buffer object for camera data.\
/// @todo poorly aligned
/// @ingroup API
 */
struct CameraUBO {
    glm::mat4 view = glm::mat4(1.0F);
    glm::mat4 projection = glm::mat4(1.0F);
    glm::mat4 viewInverse = glm::mat4(1.0F);
    glm::mat4 projectionInverse = glm::mat4(1.0F);
    glm::vec4 position = glm::vec4(1.0F); // vec3 pos + vec1 aperture
    glm::vec4 front = glm::vec4(1.0F); // vec3 front + vec1 focalDistance

private:
    glm::vec4 padding1 = glm::vec4(1.0F); ///< Padding (ignore).
    glm::vec4 padding2 = glm::vec4(1.0F); ///< Padding (ignore).
};
}
