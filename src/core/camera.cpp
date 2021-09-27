//
// By Jet <i@jetd.me> 2021.
//
#include "core/camera.hpp"
#include "core/context/global.hpp"

namespace kuafu {
Camera::Camera(int width, int height, const glm::vec3 &position) : mWidth(width), mHeight(height),
                                                                   mPosition(position) {
    mRenderTargets = std::make_shared<RenderTargets>();

    mCx = mWidth / 2.f;
    mCy = mHeight / 2.f;
    mFx = mFy = mWidth * 0.5;   // Some reasonable initialization
    updateProjectionMatrix();
    mProjNeedsUpdate = true;

    resetView();
}

void Camera::resetView() {
    static glm::vec3 position = mPosition;

    mPosition = position;
    mDirUp = {0.0F, 0.0F, 1.0F};
    mDirRight = {0.0F, -1.0F, 0.0F};
    mDirFront = {1.0F, 0.0F, 0.0F};

    updateViewMatrix();

    mViewNeedsUpdate = true;

    global::frameCount = -1;
}

glm::mat4 Camera::getPose() const {
    return {
            {mDirFront[0],  mDirFront[1],  mDirFront[2],  0},
            {-mDirRight[0], -mDirRight[1], -mDirRight[2], 0},
            {mDirUp[0],     mDirUp[1],     mDirUp[2],     0},
            {mPosition[0],  mPosition[1],  mPosition[2],  1}
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

//void Camera::setAperture(float aperture) {
//    mAperture = aperture;
//}
//
//void Camera::setFocalLength(float focalLength) {
//    mFocalLength = focalLength;
//}

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

    mCx = mWidth / 2.f;
    mCy = mHeight / 2.f;

    updateProjectionMatrix();
}

void Camera::updateViewMatrix() {
    mViewMatrix = glm::lookAt(mPosition, mPosition + mDirFront, mDirUp);
    mViewNeedsUpdate = true;
}

void Camera::setFullPerspective(
        float width, float height, float fx, float fy, float cx, float cy, float skew) {

    mWidth = static_cast<int>(width);
    mHeight = static_cast<int>(height);

    mFx = fx;
    mFy = fy;
    mCx = cx;
    mCy = cy;
    mSkew = skew;

    updateProjectionMatrix();
}

void Camera::updateProjectionMatrix() {
    mProjMatrix = glm::mat4(0);
    mProjMatrix[0][0] = (2.f * mFx) / mWidth;

    mProjMatrix[1][0] = -2 * mSkew / mWidth;
    mProjMatrix[1][1] = -(2.f * mFy) / mHeight;

    mProjMatrix[2][0] = -2.f * mCx / mWidth + 1;
    mProjMatrix[2][1] = -2.f * mCy / mHeight + 1;
    mProjMatrix[2][2] = -mFar / (mFar - mNear);
    mProjMatrix[2][3] = -1.f;

    mProjMatrix[3][2] = -mFar * mNear / (mFar - mNear);
    mProjMatrix[3][3] = 0.f;

    mProjNeedsUpdate = true;
    clearRenderTargets();
}


void Camera::setPose(glm::mat4 pose) {
    // TODO: kuafu_urgent: change check to avoid View update

    mPosition = {pose[3][0], pose[3][1], pose[3][2]};
    mDirFront = {-pose[2][0], -pose[2][1], -pose[2][2]};
    mDirUp = {pose[1][0], pose[1][1], pose[1][2]};

    mViewNeedsUpdate = true;
    global::frameCount = -1;
}


void Camera::processMouse(float xOffset, float yOffset) {
    float upRot = xOffset * 0.01;
    float rightRot = yOffset * 0.01;

    glm::mat4 urot = glm::rotate(glm::mat4(1.0), upRot, mDirUp);
    glm::mat4 rrot = glm::rotate(glm::mat4(1.0), rightRot, mDirRight);

    auto tmp = urot * glm::vec4(mDirFront, 0);
    mDirFront = {tmp[0], tmp[1],tmp[2]};
//    std::cout << tmp[0] << tmp[1] << tmp[2] << std::endl;

    tmp = rrot * glm::vec4(mDirFront, 0);
//    mDirFront = {tmp[0], tmp[1],tmp[2]};

    tmp = rrot * glm::vec4(mDirUp, 0);
//    mDirUp = {tmp[0], tmp[1],tmp[2]};

    mViewNeedsUpdate = true;
    global::frameCount = -1;
}
}
