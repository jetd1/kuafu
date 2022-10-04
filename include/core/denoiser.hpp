//
// Created by jet on 9/16/21.
//

#pragma once


#include "stdafx.hpp"
#include "optix_types.h"
#include "cuda_dl.hpp"


#define OPTIX_CHECK(call)                                          \
  do                                                               \
  {                                                                \
    auto val = (call);                                             \
    KF_ASSERT(val == OPTIX_SUCCESS,                                \
         "{}:{} Optix call failed: {}", __FILE__, __LINE__, val);  \
  } while(false)

#define CUDA_CHECK0(call)                                        \
  do                                                             \
  {                                                              \
    KF_ASSERT((call) == CUDA_SUCCESS,                            \
         "{}:{} CUDA call failed", __FILE__, __LINE__);          \
  } while(false)

#define CUDA_CHECK(call)                                         \
  do                                                             \
  {                                                              \
    KF_ASSERT((call) == cudaSuccess,                             \
         "{}:{} CUDA call failed", __FILE__, __LINE__);          \
  } while(false)

namespace kuafu {

class DenoiserOptix {
    CUstream               mCudaStream = nullptr;

    OptixPixelFormat       mPixelFormat{};
    uint32_t               mSizeofPixel = 0;
    int                    mDenoiseAlpha = 0;

    OptixDenoiser          mDenoiser = nullptr;
    OptixDenoiserOptions   mDOptions{};
    OptixDenoiserSizes     mDSizes{};
    CUdeviceptr            mDState{0};
    CUdeviceptr            mDScratch{0};
    CUdeviceptr            mDIntensity{0};
    CUdeviceptr            mDMinRgb{0};

    // Holding the Buffer for Cuda interop
    struct BufferCuda
    {
        vkCore::Buffer buffer;

        // Extra for Cuda
        int handle = -1;
        void* cudaPtr = nullptr;
        cudaExternalMemory_t mem {};

        void destroy() {
            if (handle != -1) {
                cudaDestroyExternalMemory(mem);
                mem = {};
                cudaFree(cudaPtr);
                cudaPtr = nullptr;
                close(handle);
                handle = -1;
            }
        }
    };

    vk::Extent2D                 mImageSize;
    std::array<BufferCuda, 3>    mPixelBufferIn;
    BufferCuda                   mPixelBufferOut;

    void createBufferCuda(BufferCuda& buf);

    // For synchronizing with Vulkan
    struct Semaphore
    {
        vk::Semaphore           vk;  // Vulkan
        cudaExternalSemaphore_t cu;  // Cuda version
        int handle{-1};
    } mSemaphore;

public:
    DenoiserOptix() = default;
    ~DenoiserOptix() = default;

    bool initOptiX(
            OptixDenoiserInputKind inputKind,
            OptixPixelFormat pixelFormat,
            bool hdr);
    void denoiseImageBuffer(uint64_t& fenceValue);


    void allocateBuffers(const vk::Extent2D& imgSize);
    void imageToBuffer(const vk::CommandBuffer& cmdBuf, const std::vector<vk::Image>& imgIn);
    void bufferToImage(const vk::CommandBuffer& cmdBuf, vk::Image imgOut);

    void createSemaphore();
    auto getTLSemaphore() { return mSemaphore.vk; }

    void freeResources();
    void destroy();

};

}
