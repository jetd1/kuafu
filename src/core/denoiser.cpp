//
// Created by jet on 9/16/21.
//

#ifdef KUAFU_OPTIX_DENOISER

#include "core/denoiser.hpp"

#include "optix.h"
#include "optix_stubs.h"
#include "optix_function_table_definition.h"


namespace kuafu {

OptixDeviceContext gOptixDevice;

static void _log_cb(unsigned int level, const char* tag, const char* message, void*) {
#ifdef _DEBUG
    std::cerr << "[" << std::setw(2) << level << "]" \
              << "[" << std::setw(12) << tag << "]: " << message << "\n";
#endif
}


bool DenoiserOptix::initOptiX(
        OptixDenoiserInputKind inputKind,
        OptixPixelFormat pixelFormat,
        bool hdr) {
    kfCuDlOpen();
    CUDA_CHECK0(kfCuInit(0));
    CUDA_CHECK(cudaFree(0)); // Force initialization of cuda primary context

    CUdevice device = 0;    // TODO
    CUDA_CHECK0(kfCuStreamCreate(&mCudaStream, CU_STREAM_DEFAULT));

    OPTIX_CHECK(optixInit());
    OPTIX_CHECK(optixDeviceContextCreate(nullptr, nullptr, &gOptixDevice));
    OPTIX_CHECK(optixDeviceContextSetLogCallback(gOptixDevice, _log_cb, nullptr, 4));

    mPixelFormat = pixelFormat;
    switch(pixelFormat) {
        case OPTIX_PIXEL_FORMAT_FLOAT3:
            mSizeofPixel  = static_cast<uint32_t>(3 * sizeof(float));
            mDenoiseAlpha = 0;
            break;
        case OPTIX_PIXEL_FORMAT_FLOAT4:
            mSizeofPixel  = static_cast<uint32_t>(4 * sizeof(float));
            mDenoiseAlpha = 1;
            break;
        case OPTIX_PIXEL_FORMAT_UCHAR3:
            mSizeofPixel  = static_cast<uint32_t>(3 * sizeof(uint8_t));
            mDenoiseAlpha = 0;
            break;
        case OPTIX_PIXEL_FORMAT_UCHAR4:
            mSizeofPixel  = static_cast<uint32_t>(4 * sizeof(uint8_t));
            mDenoiseAlpha = 1;
            break;
        case OPTIX_PIXEL_FORMAT_HALF3:
            mSizeofPixel  = static_cast<uint32_t>(3 * sizeof(uint16_t));
            mDenoiseAlpha = 0;
            break;
        case OPTIX_PIXEL_FORMAT_HALF4:
            mSizeofPixel  = static_cast<uint32_t>(4 * sizeof(uint16_t));
            mDenoiseAlpha = 1;
            break;
        default:
            KF_CRITICAL("Unsupported OPTIX_PIXEL_FORMAT!");
            break;
    }

    mDOptions.inputKind = inputKind;
    OPTIX_CHECK(optixDenoiserCreate(gOptixDevice, &mDOptions, &mDenoiser));
    OPTIX_CHECK(optixDenoiserSetModel(
            mDenoiser, hdr ? OPTIX_DENOISER_MODEL_KIND_HDR
                           : OPTIX_DENOISER_MODEL_KIND_LDR, nullptr, 0));

    return true;
}


void DenoiserOptix::createSemaphore() {
    auto handleType = vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd;

    vk::SemaphoreTypeCreateInfo timelineCreateInfo;
    timelineCreateInfo.semaphoreType = vk::SemaphoreType::eTimeline;
    timelineCreateInfo.initialValue  = 0;

    vk::ExportSemaphoreCreateInfo esci;
    esci.handleTypes = handleType;
    esci.pNext       = &timelineCreateInfo;

    vk::SemaphoreCreateInfo sci;
    sci.pNext        = &esci;

    mSemaphore.vk    = vkCore::global::device.createSemaphore(sci);
    mSemaphore.handle = vkCore::global::device.getSemaphoreFdKHR(
            {mSemaphore.vk, handleType});

    cudaExternalSemaphoreHandleDesc externalSemaphoreHandleDesc {};
    std::memset(&externalSemaphoreHandleDesc, 0, sizeof(externalSemaphoreHandleDesc));
    externalSemaphoreHandleDesc.flags = 0;

    externalSemaphoreHandleDesc.type  = cudaExternalSemaphoreHandleTypeTimelineSemaphoreFd;
    externalSemaphoreHandleDesc.handle.fd = mSemaphore.handle;

    CUDA_CHECK(cudaImportExternalSemaphore(&mSemaphore.cu, &externalSemaphoreHandleDesc));
}


//--------------------------------------------------------------------------------------------------
// Allocating all the buffers in which the images will be transfered.
// The buffers are shared with Cuda, therefore OptiX can denoised them
//
void DenoiserOptix::allocateBuffers(const vk::Extent2D& imgSize)
{
    mImageSize = imgSize;

    freeResources();
    createSemaphore();

    vk::DeviceSize bufferSize =
            static_cast<unsigned long long>(mImageSize.width) * mImageSize.height * 4 * sizeof(float);

    // Using direct method
    vk::BufferUsageFlags usage{
          vk::BufferUsageFlagBits::eUniformBuffer
        | vk::BufferUsageFlagBits::eTransferDst
        | vk::BufferUsageFlagBits::eTransferSrc
    };

    mPixelBufferIn[0].buffer.init(
            bufferSize, usage,
            {vkCore::global::graphicsFamilyIndex}, vk::MemoryPropertyFlagBits::eDeviceLocal);

    createBufferCuda(mPixelBufferIn[0]);  // Exporting the buffer to Cuda handle and pointers

    if(mDOptions.inputKind > OPTIX_DENOISER_INPUT_RGB) {
        mPixelBufferIn[1].buffer.init(
                bufferSize, usage,
                {vkCore::global::graphicsFamilyIndex}, vk::MemoryPropertyFlagBits::eDeviceLocal);
        createBufferCuda(mPixelBufferIn[1]);
    }
    if(mDOptions.inputKind == OPTIX_DENOISER_INPUT_RGB_ALBEDO_NORMAL) {
        mPixelBufferIn[2].buffer.init(
                bufferSize, usage,
                {vkCore::global::graphicsFamilyIndex}, vk::MemoryPropertyFlagBits::eDeviceLocal);
        createBufferCuda(mPixelBufferIn[2]);
    }

    // Output image/buffer
    mPixelBufferOut.buffer.init(
            bufferSize, usage,
            {vkCore::global::graphicsFamilyIndex}, vk::MemoryPropertyFlagBits::eDeviceLocal);
    createBufferCuda(mPixelBufferOut);

    // Computing the amount of memory needed to do the denoiser
    OPTIX_CHECK(optixDenoiserComputeMemoryResources(mDenoiser, mImageSize.width, mImageSize.height, &mDSizes));

    CUDA_CHECK(cudaMalloc((void**)&mDState, mDSizes.stateSizeInBytes));
    CUDA_CHECK(cudaMalloc((void**)&mDScratch, mDSizes.withoutOverlapScratchSizeInBytes));
    CUDA_CHECK(cudaMalloc((void**)&mDMinRgb, 4 * sizeof(float)));
    if(mPixelFormat == OPTIX_PIXEL_FORMAT_FLOAT3 || mPixelFormat == OPTIX_PIXEL_FORMAT_FLOAT4)
        CUDA_CHECK(cudaMalloc((void**)&mDIntensity, sizeof(float)));

    OPTIX_CHECK(optixDenoiserSetup(mDenoiser, mCudaStream, mImageSize.width, mImageSize.height, mDState,
                                   mDSizes.stateSizeInBytes, mDScratch, mDSizes.withoutOverlapScratchSizeInBytes));
}


void DenoiserOptix::createBufferCuda(BufferCuda& buf) {
    buf.handle = vkCore::global::device.getMemoryFdKHR({
        buf.buffer.getMemory(),
        vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd});
    auto req = vkCore::global::device.getBufferMemoryRequirements(buf.buffer.get());

    cudaExternalMemoryHandleDesc cudaExtMemHandleDesc{};
    cudaExtMemHandleDesc.size      = req.size;
    cudaExtMemHandleDesc.type      = cudaExternalMemoryHandleTypeOpaqueFd;
    cudaExtMemHandleDesc.handle.fd = buf.handle;

    CUDA_CHECK(cudaImportExternalMemory(&buf.mem, &cudaExtMemHandleDesc));

    cudaExternalMemoryBufferDesc cudaExtBufferDesc{};
    cudaExtBufferDesc.offset = 0;
    cudaExtBufferDesc.size   = req.size;
    cudaExtBufferDesc.flags  = 0;
    CUDA_CHECK(cudaExternalMemoryGetMappedBuffer(&buf.cudaPtr, buf.mem, &cudaExtBufferDesc));
}


//--------------------------------------------------------------------------------------------------
// Converting the image to a buffer used by the denoiser
//
void DenoiserOptix::imageToBuffer(
        const vk::CommandBuffer& cmdBuf, const std::vector<vk::Image>& imgIn) {
    for(int i = 0; i < static_cast<int>(imgIn.size()); i++) {
        const vk::Buffer& pixelBufferIn = mPixelBufferIn[i].buffer.get();
        // Make the image layout eTransferSrcOptimal to copy to buffer
        vkCore::transitionImageLayout(
                imgIn[i],
                vk::ImageLayout::eGeneral,
                vk::ImageLayout::eTransferSrcOptimal,
                cmdBuf);

        // Copy the image to the buffer
        vk::BufferImageCopy copyRegion;
        copyRegion.setImageSubresource(
                {vk::ImageAspectFlagBits::eColor, 0, 0, 1});
        copyRegion.setImageExtent(vk::Extent3D(mImageSize, 1));
        cmdBuf.copyImageToBuffer(
                imgIn[i], vk::ImageLayout::eTransferSrcOptimal, pixelBufferIn, {copyRegion});

        // Put back the image as it was
        vkCore::transitionImageLayout(
                imgIn[i],
                vk::ImageLayout::eTransferSrcOptimal,
                vk::ImageLayout::eGeneral,
                cmdBuf);
    }
}


//--------------------------------------------------------------------------------------------------
// Converting the output buffer to the image
//
void DenoiserOptix::bufferToImage(const vk::CommandBuffer& cmdBuf, vk::Image imgOut)
{
    const vk::Buffer& pixelBufferOut = mPixelBufferOut.buffer.get();

    // Transit the depth buffer image in eTransferSrcOptimal
    vkCore::transitionImageLayout(
            imgOut,
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eTransferDstOptimal,
            cmdBuf);

    // Copy the pixel under the cursor
    vk::BufferImageCopy copyRegion;
    copyRegion.setImageSubresource(
            {vk::ImageAspectFlagBits::eColor, 0, 0, 1});
    copyRegion.setImageOffset({0, 0, 0});
    copyRegion.setImageExtent(vk::Extent3D(mImageSize, 1));
    cmdBuf.copyBufferToImage(
            pixelBufferOut, imgOut, vk::ImageLayout::eTransferDstOptimal, {copyRegion});

    // Put back the depth buffer as  it was
    vkCore::transitionImageLayout(
            imgOut,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eGeneral,
            cmdBuf);
}


//--------------------------------------------------------------------------------------------------
// Denoising the image in input and saving the denoised image in the output
//
void DenoiserOptix::denoiseImageBuffer(uint64_t& fenceValue)
{
    try
    {
        OptixPixelFormat pixelFormat      = mPixelFormat;
        auto             sizeofPixel      = mSizeofPixel;
        uint32_t         rowStrideInBytes = sizeofPixel * mImageSize.width;

        std::vector<OptixImage2D> inputLayer;  // Order: RGB, Albedo, Normal

        // RGB
        inputLayer.push_back(OptixImage2D{(CUdeviceptr)mPixelBufferIn[0].cudaPtr, mImageSize.width, mImageSize.height,
                                          rowStrideInBytes, 0, pixelFormat});
        // ALBEDO
        if(mDOptions.inputKind == OPTIX_DENOISER_INPUT_RGB_ALBEDO || mDOptions.inputKind == OPTIX_DENOISER_INPUT_RGB_ALBEDO_NORMAL)
            inputLayer.push_back(OptixImage2D{(CUdeviceptr)mPixelBufferIn[1].cudaPtr, mImageSize.width, mImageSize.height,
                                              rowStrideInBytes, 0, pixelFormat});

        // NORMAL
        if(mDOptions.inputKind == OPTIX_DENOISER_INPUT_RGB_ALBEDO_NORMAL)
            inputLayer.push_back(OptixImage2D{(CUdeviceptr)mPixelBufferIn[2].cudaPtr, mImageSize.width, mImageSize.height,
                                              rowStrideInBytes, 0, pixelFormat});

        OptixImage2D outputLayer = {
                (CUdeviceptr)mPixelBufferOut.cudaPtr, mImageSize.width, mImageSize.height, rowStrideInBytes, 0, pixelFormat};

        // Wait from Vulkan (Copy to Buffer)
        cudaExternalSemaphoreWaitParams waitParams{};
        waitParams.flags              = 0;
        waitParams.params.fence.value = fenceValue;
        cudaWaitExternalSemaphoresAsync(&mSemaphore.cu, &waitParams, 1, nullptr);

        CUstream stream = mCudaStream;
        if(mDIntensity != 0)
        {
            OPTIX_CHECK(optixDenoiserComputeIntensity(mDenoiser, stream, inputLayer.data(), mDIntensity, mDScratch,
                                                      mDSizes.withoutOverlapScratchSizeInBytes));
        }

        OptixDenoiserParams params{};
        params.denoiseAlpha = mDenoiseAlpha;
        params.hdrIntensity = mDIntensity;
        params.blendFactor  = 0.0f;  // Fully denoised

        OPTIX_CHECK(optixDenoiserInvoke(mDenoiser, stream, &params, mDState, mDSizes.stateSizeInBytes, inputLayer.data(),
                                        (uint32_t)inputLayer.size(), 0, 0, &outputLayer, mDScratch,
                                        mDSizes.withoutOverlapScratchSizeInBytes));

        CUDA_CHECK(cudaStreamSynchronize(stream));  // Making sure the denoiser is done

        cudaExternalSemaphoreSignalParams sigParams{};
        sigParams.flags              = 0;
        sigParams.params.fence.value = ++fenceValue;
        cudaSignalExternalSemaphoresAsync(&mSemaphore.cu, &sigParams, 1, stream);
    }
    catch(const std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
}

void DenoiserOptix::freeResources() {
    vkCore::global::device.destroy(mSemaphore.vk);

    for(auto& p : mPixelBufferIn)
        p.destroy();               // Closing Handle
    mPixelBufferOut.destroy();     // Closing Handle

    if(mDState != 0)
        CUDA_CHECK(cudaFree((void*)mDState));

    if(mDScratch != 0)
        CUDA_CHECK(cudaFree((void*)mDScratch));

    if(mDIntensity != 0)
        CUDA_CHECK(cudaFree((void*)mDIntensity));

    if(mDMinRgb != 0)
        CUDA_CHECK(cudaFree((void*)mDMinRgb));
}

void DenoiserOptix::destroy() {
    try {
        freeResources();
    } catch(...) {}

    kfCuDlClose();
}



}

#endif
