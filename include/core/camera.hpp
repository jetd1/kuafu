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
#include "image.hpp"

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
    Camera(const Camera &) = delete;
    Camera(const Camera &&) = delete;
    Camera & operator=(Camera) = delete;
    Camera & operator=(const Camera &) = delete;
    Camera & operator=(const Camera &&) = delete;

    /// Is used to update camera vectors etc.
    ///
    /// The user has to override this function for the camera to work like intended.
    /// @note The function will be called every tick.
    void update();

    void resetView();

    /// @return Returns the camera's position.
    auto getPosition() const -> const glm::vec3 & { return mPosition; }

    /// @return Returns the camera's front vector / viewing direction.
    auto getFront() const -> const glm::vec3 & { return mDirFront; }

    /// Is used to change the camera's position.
    /// @param position The new camera position.
    void setPosition(const glm::vec3 &position);

    void setFront(const glm::vec3 &front);

    void setUp(const glm::vec3 &up);

//    void setAperture(float aperture);
//
    [[nodiscard]] inline float getAperture() const { return mAperture; }

//    void setFocalLength(float focalLength);
//
    [[nodiscard]] inline float getFocalLength() const { return mFocalLength; }

    /// Is used to set a size for the camera that fits the viewport dimensions.
    /// @param width The width of the viewport.
    /// @param height The height of the viewport.
    void setSize(int width, int height);

    void setPose(glm::mat4 pose);

    /// Set the camera's projection matrix (and inv) using a full set of perspective parameters.
    void setFullPerspective(float width, float height, float fx, float fy, float cx, float cy, float skew);

    glm::mat4 getPose() const;

    [[nodiscard]] auto getWidth() const { return mWidth; }

    [[nodiscard]] auto getHeight() const { return mHeight; }

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
    void processMouse(float xOffset, float yOffset);

    /// Used to handle user-defined keyboard inputs or input events.
    /// This function is called inside update() and followed up by a call to updateViewMatrix() and does not have to be called by the user.
    /// @note Implementation of this function should be inside a user-defined inherited class.
    void processKeyboard();

    inline void clearRenderTargets() { mRenderTargets->clear(); }
    inline auto getRenderTargets() { return mRenderTargets; }

    [[nodiscard]] inline float getPrincipalPointX() const { return mCx; }
    [[nodiscard]] inline float getPrincipalPointY() const { return mCy; }
    [[nodiscard]] inline float getFocalX() const { return mFx; }
    [[nodiscard]] inline float getFocalY() const { return mFy; }
    [[nodiscard]] inline float getNear() const { return mNear; }
    [[nodiscard]] inline float getFar() const { return mFar; }
    [[nodiscard]] inline float getSkew() const { return mSkew; }

    std::shared_ptr<Frames> mFrames;
    vkCore::Sync mSync;

    std::vector<uint8_t> downloadLatestFrame();

    bool mFirst = true;

private:
    friend class Scene;

    /// @param width The width of the viewport.
    /// @param height The height of the viewport.
    /// @param position The position of your camera.
    Camera(int width, int height, const glm::vec3 &position = {0.0F, 0.0F, 3.0F});


    int mWidth;  ///< The width of the viewport.
    int mHeight; ///< The height of the viewport.

    glm::vec3 mPosition; ///< The camera's position.

    float mFx;
    float mFy;
    float mCx;
    float mCy;
    float mSkew = 0;

    glm::mat4 mViewMatrix = glm::mat4(1.0F); ///< The view matrix.
    glm::mat4 mProjMatrix = glm::mat4(1.0F); ///< The projection matrix

    glm::vec3 mDirUp = {0.0F, 0.0F, 1.0F};                      ///< The local up vector.
    glm::vec3 mDirRight = {0.0F, -1.0F, 0.0F};                  ///< The local right vector.
    glm::vec3 mDirFront = {1.0F, 0.0F, 0.0F};                   ///< The viewing direction.

    float mAperture = 0.0F;   // DOF disabled by default   // FIXME: modernize this
    float mFocalLength = 5.0F;

    const float mFar = 100.F;
    const float mNear = 0.1F;

    std::shared_ptr<RenderTargets> mRenderTargets;
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
