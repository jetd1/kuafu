//
// Created by jet on 9/21/21.
//

#pragma once

#include "stdafx.hpp"

namespace kuafu {

class Frames {
    size_t mN;

    vk::Format mFormat;
    vk::Extent2D mExtent;
    std::vector<vk::UniqueImage> mImages;
    std::vector<vk::UniqueDeviceMemory> mImagesMemory;

    std::vector<vk::UniqueImageView> mImageViews;
    std::vector<vk::UniqueFramebuffer> mFramebuffers;

    size_t mCurrentImageIdx;

    std::mutex mLock;

    bool mInitialized = false;

public:

    size_t mCurrentFrame = 0;
    size_t mPrevFrame = 0;

    inline void init(
            size_t n,
            vk::Extent2D extent,
            vk::Format format,
            vk::ColorSpaceKHR colorSpace,
            vk::RenderPass renderPass) {
        if (mInitialized)
            return;

        mN = n;
        mFormat = format;
        mExtent = extent;
        mImages.resize(n);
        mImagesMemory.resize(n);
        mImageViews.resize(n);
        mFramebuffers.resize(n);

        vk::ImageCreateInfo createInfo;
        createInfo.format = format;
        createInfo.extent = vk::Extent3D{extent, 1};
        createInfo.imageType = vk::ImageType::e2D;
        createInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
        createInfo.arrayLayers = 1;
        createInfo.mipLevels = 1;
        createInfo.sharingMode = vk::SharingMode::eExclusive;

        for (size_t i = 0; i < n; ++i) {
            mImages[i] = vkCore::global::device.createImageUnique(createInfo);

            auto memRequirements = vkCore::global::device.getImageMemoryRequirements(mImages[i].get());

            vk::MemoryAllocateInfo allocInfo;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = vkCore::findMemoryType(
                    vkCore::global::physicalDevice,
                    memRequirements.memoryTypeBits,
                    vk::MemoryPropertyFlagBits::eDeviceLocal);

            mImagesMemory[i] = vkCore::global::device.allocateMemoryUnique(allocInfo);
            vkCore::global::device.bindImageMemory(mImages[i].get(), mImagesMemory[i].get(), 0);
        }

        for (size_t i = 0; i < mImageViews.size(); ++i)
            mImageViews[i] = vkCore::initImageViewUnique(vkCore::getImageViewCreateInfo(
                    mImages[i].get(), format, vk::ImageViewType::e2D, vk::ImageAspectFlagBits::eColor));

        for (size_t i = 0; i < mFramebuffers.size(); ++i)
            mFramebuffers[i] = vkCore::initFramebufferUnique(
                    { mImageViews[i].get( ) }, renderPass, extent);

        mInitialized = true;
    }

    inline void destroy() {
        if (!mInitialized)
            return;

        mFramebuffers.clear();
        mImageViews.clear();
        mImages.clear();
        mImagesMemory.clear();

        mInitialized = false;
    }

    inline auto getFormat() { return mFormat; }
    inline auto getExtent() { return mExtent; }
    inline auto getImage(size_t n) {
        KF_ASSERT(mInitialized, "mInitialized: getImage"); return mImages[n].get(); }
    inline auto& getFramebuffer(size_t n) {
        KF_ASSERT(mInitialized, "mInitialized: getFramebuffer"); return mFramebuffers[n]; }

    [[nodiscard]] inline size_t getCurrentImageIndex() const {
        KF_ASSERT(mInitialized, "mInitialized: getCurrentImageIndex"); return mCurrentImageIdx; }
    inline void acquireNextImage() {
        KF_ASSERT(mInitialized, "acquireNextImage: acquireNextImage");
        mLock.lock();
        mCurrentImageIdx = (mCurrentImageIdx + 1) % mN;
        mLock.unlock();
    }

    [[nodiscard]] inline bool initialized() const { return mInitialized; }
};

class StorageImage {
    vkCore::Image mStorageImage;
    vk::UniqueImageView mStorageImageView;
    vk::UniqueSampler mStorageImageSampler;
    vk::DescriptorImageInfo mStorageImageInfo;

public:
    inline void init(const vk::ImageCreateInfo& createInfo) {
        mStorageImage.init(createInfo);
        mStorageImage.transitionToLayout(vk::ImageLayout::eGeneral);

        mStorageImageView = vkCore::initImageViewUnique(
                vkCore::getImageViewCreateInfo(mStorageImage.get(), mStorageImage.getFormat()));

        auto samplerCreateInfo = vkCore::getSamplerCreateInfo();
        mStorageImageSampler = vkCore::initSamplerUnique(samplerCreateInfo);

        mStorageImageInfo.sampler = mStorageImageSampler.get();
        mStorageImageInfo.imageView = mStorageImageView.get();
        mStorageImageInfo.imageLayout = mStorageImage.getLayout();
    }

    [[nodiscard]] inline auto get() const { return mStorageImage.get(); }
    [[nodiscard]] inline auto getExtent() const { return mStorageImage.getExtent(); }
    [[nodiscard]] inline auto getFormat() const { return mStorageImage.getFormat(); }
    [[nodiscard]] inline auto getView() const { return mStorageImageView.get(); };
    [[nodiscard]] inline auto getSampler() const { return mStorageImageSampler.get(); };
    [[nodiscard]] inline const auto& getInfo() const { return mStorageImageInfo; };

    inline void setLayout(vk::ImageLayout layout) { mStorageImage.transitionToLayout(layout); }
};

typedef std::unordered_map<std::string, StorageImage> RenderTargets;

}

