//
// Created by jet on 4/9/21.
//

#include "core/camera.hpp"
#include "core/context/global.hpp"

namespace kuafu {
    Camera::Camera(int width, int height, const glm::vec3 &position) :
            _width(width),
            _height(height),
            mPosition(position) {
        reset();
    }

    void Camera::reset() {
        static glm::vec3 position = mPosition;
        static float yaw = _yaw;
        static float pitch = _pitch;

        mPosition = position;
        _yaw = yaw;
        _pitch = pitch;

        _fov = 45.0F;

        updateVectors();
        updateViewMatrix();
        updateProjectionMatrix();

        mViewNeedsUpdate = true;
        mProjNeedsUpdate = true;

        global::frameCount = -1;
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
        _aperture = aperture;
    }

    void Camera::setFocalDistance(float focalDistance) {
        _focalDistance = focalDistance;
    }

    void Camera::setPosition(const glm::vec3 &position) {
        mPosition = position;

        updateViewMatrix();
    }

    void Camera::setFront(const glm::vec3 &front) {
        mDirFront = front;

        updateViewMatrix();
    }

    void Camera::setSize(int width, int height) {
        _width = width;
        _height = height;

        updateProjectionMatrix();
    }

    void Camera::setFov(float fov) {
        _fov = fov;

        updateProjectionMatrix();
    }

    void Camera::setSensitivity(float sensitivity) {
        _sensitivity = sensitivity;
    }

    void Camera::updateViewMatrix() {
        _view = glm::lookAt(mPosition, mPosition + mDirFront, _worldUp);

        _viewInverse = glm::inverse(_view);

        mViewNeedsUpdate = true;
    }

    void Camera::updateProjectionMatrix() {
        _projection = glm::perspective(glm::radians(_fov), static_cast<float>( _width ) / static_cast<float>( _height ),
                                       0.1F, 100.0F);
        _projection[1][1] *= -1;

        _projectionInverse = glm::inverse(_projection);

        mProjNeedsUpdate = true;
    }

    void Camera::processMouse(float xOffset, float yOffset) {
        mViewNeedsUpdate = true;

        xOffset *= _sensitivity;
        yOffset *= _sensitivity;

        _yaw += xOffset;
        _pitch += yOffset;

        if (_yaw > 360.0F) {
            _yaw = fmod(_yaw, 360.0F);
        }

        if (_yaw < 0.0F) {
            _yaw = 360.0F + fmod(_yaw, 360.0F);
        }

        if (_pitch > 89.0F) {
            _pitch = 89.0F;
        }

        if (_pitch < -89.0F) {
            _pitch = -89.0F;
        }

        updateVectors();

        // Frame counter for jitter cam needs to be reset.
        global::frameCount = -1;
    }

    void Camera::updateVectors() {
        glm::vec3 t_front;
        t_front.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
        t_front.y = sin(glm::radians(_pitch));
        t_front.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));

        mDirFront = glm::normalize(t_front);
        mDirRight = glm::normalize(glm::cross(mDirFront, _worldUp));
        _up = glm::normalize(glm::cross(mDirRight, mDirFront));
    }
}
