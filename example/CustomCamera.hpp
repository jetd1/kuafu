#ifndef CUSTOM_CAMERA_HPP
#define CUSTOM_CAMERA_HPP

#include "Keys.hpp"
#include "kuafu.hpp"

class CustomCamera : public kuafu::Camera {
public:
    CustomCamera(int width, int height, const glm::vec3 &position) :
            kuafu::Camera(width, height, position) {
    }

    void processKeyboard() override {
        const float defaultSpeed = 2.5F;
        static float currentSpeed = defaultSpeed;
        float finalSpeed = currentSpeed * kuafu::Time::getDeltaTime();

        if (Key::eLeftShift) {
            currentSpeed = 10.0F;
        } else if (Key::eLeftCtrl) {
            currentSpeed = 0.5F;
        } else {
            currentSpeed = defaultSpeed;
        }

        if (Key::eW) {
            mPosition += mDirFront * finalSpeed;
            mViewNeedsUpdate = true;
        }

        if (Key::eS) {
            mPosition -= mDirFront * finalSpeed;
            mViewNeedsUpdate = true;
        }

        if ( Key::eA ) {
            mPosition -= mDirRight * finalSpeed;
            mViewNeedsUpdate = true;
        }

        if ( Key::eD ) {
            mPosition += mDirRight * finalSpeed;
            mViewNeedsUpdate = true;
        }
    }
};

#endif // CUSTOM_CAMERA_HPP
