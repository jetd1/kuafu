//
// Created by jet on 4/9/21.
//

#include "core/camera.hpp"
#include "core/context/global.hpp"

namespace kuafu {
    Camera::Camera(int width, int height, const glm::vec3 &position) : mWidth(width), mHeight(height),
            mPosition(position) {
        reset();
    }

    void Camera::reset() {
        static glm::vec3 position = mPosition;

        mPosition = position;
        mDirUp = {0.0F, 0.0F, 1.0F};
        mDirRight = {0.0F, -1.0F, 0.0F};
        mDirFront = {1.0F, 0.0F, 0.0F};

        mFov = 90.0F;

        updateViewMatrix();
        updateProjectionMatrix();

        mViewNeedsUpdate = true;
        mProjNeedsUpdate = true;

        global::frameCount = -1;
    }

    glm::mat4 Camera::getPose() const {
      return {
          {mDirFront[0], mDirFront[1], mDirFront[2], 0},
          {-mDirRight[0], -mDirRight[1], -mDirRight[2], 0},
          {mDirUp[0], mDirUp[1], mDirUp[2], 0},
          {mPosition[0], mPosition[1], mPosition[2], 1}
      };
    }

    void Camera::update() {
        // If position has changed, reset frame counter for jitter cam.
        static glm::vec3 prevPosition = mPosition;
        if (prevPosition != mPosition) {
            global::frameCount = -1;
            prevPosition = mPosition;
        }

        processKeyboard();
        updateViewMatrix();
    }

    void Camera::setAperture(float aperture) {
        mAperture = aperture;
    }

    void Camera::setFocalLength(float focalLength) {
        mFocalLength = focalLength;
    }

    void Camera::setPosition(const glm::vec3 &position) {
        mPosition = position;

        updateViewMatrix();
    }

    void Camera::setFront(const glm::vec3 &front) {
        mDirFront = front;

        updateViewMatrix();
    }

    void Camera::setUp(const glm::vec3 &up) {
      mDirUp = up;

      updateViewMatrix();
    }

    void Camera::setSize(int width, int height) {
      mWidth = width;
      mHeight = height;

        updateProjectionMatrix();
    }

    void Camera::setFov(float fov) {
      mFov = fov;

        updateProjectionMatrix();
    }

    void Camera::updateViewMatrix() {
      mViewMatrix = glm::lookAt(mPosition, mPosition + mDirFront, mDirUp);
      mViewNeedsUpdate = true;
    }

    void Camera::updateProjectionMatrix() {
      mProjMatrix = glm::perspective(
          glm::radians(mFov),
          static_cast<float>(mWidth) / static_cast<float>(mHeight),
          0.1F, 100.0F);
      mProjMatrix[1][1] *= -1;
      mProjNeedsUpdate = true;
    }

    void Camera::setPose(glm::mat4 pose) {
      setPosition({pose[3][0], pose[3][1], pose[3][2]});
      mDirFront = {-pose[2][0], -pose[2][1], -pose[2][2]};
      mDirUp = {pose[1][0], pose[1][1], pose[1][2]};

      mViewNeedsUpdate = true;
      global::frameCount = -1;
    }


    void Camera::processMouse(float xOffset, float yOffset) {
          mViewNeedsUpdate = true;
          global::frameCount = -1;
      }
}
