//
// Created by jet on 9/21/21.
//

#pragma once

#include <stdafx.hpp>
#include <vulkan/vulkan.hpp>

namespace kuafu {

class Frames {
    size_t mN;

    std::vector<vk::Image> mImages;
    std::vector<vk::DeviceMemory> mImagesMemory;

    std::vector<vk::UniqueImageView> mImageViews;
    std::vector<vk::UniqueFramebuffer> mFramebuffers;

    size_t mCurrentImageIdx;

    std::mutex mLock;

public:

    inline void init(
            size_t n,
            vk::Extent2D extent,
            vk::Format format,
            vk::ColorSpaceKHR colorSpace,
            vk::RenderPass renderPass) {
        mN = n;
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
            mImages[i] = vkCore::global::device.createImage(createInfo);

            auto memRequirements = vkCore::global::device.getImageMemoryRequirements(mImages[i]);

            vk::MemoryAllocateInfo allocInfo;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = vkCore::findMemoryType(
                    vkCore::global::physicalDevice,
                    memRequirements.memoryTypeBits,
                    vk::MemoryPropertyFlagBits::eDeviceLocal);

            mImagesMemory[i] = vkCore::global::device.allocateMemory(allocInfo);
            vkCore::global::device.bindImageMemory(mImages[i], mImagesMemory[i], 0);
        }

        for (size_t i = 0; i < mImageViews.size(); ++i)
            mImageViews[i] = vkCore::initImageViewUnique(vkCore::getImageViewCreateInfo(
                    mImages[i], format, vk::ImageViewType::e2D, vk::ImageAspectFlagBits::eColor));

        for (size_t i = 0; i < mFramebuffers.size(); ++i)
            mFramebuffers[i] = vkCore::initFramebufferUnique(
                    { mImageViews[i].get( ) }, renderPass, extent);

    }

    inline void destroy() {
        mFramebuffers.clear();
        mImageViews.clear();
        for (size_t i = 0; i < mN; i++) {
            vkCore::global::device.destroy(mImages[i]);
            vkCore::global::device.freeMemory(mImagesMemory[i]);
        }
        mImages.clear();
        mImagesMemory.clear();
    }

    inline auto getImage(size_t n) { return mImages[n]; }
    inline auto& getFramebuffer(size_t n) { return mFramebuffers[n]; }

    [[nodiscard]] inline size_t getCurrentImageIndex() const { return mCurrentImageIdx; }
    inline void acquireNextImage() {
        mLock.lock();
        mCurrentImageIdx = (mCurrentImageIdx + 1) % mN;
        mLock.unlock();
    }
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

    inline auto get() const { return mStorageImage.get(); }
    inline auto getExtent() const { return mStorageImage.getExtent(); }
    inline auto getFormat() const { return mStorageImage.getFormat(); }
    inline auto getView() const { return mStorageImageView.get(); };
    inline auto getSampler() const { return mStorageImageSampler.get(); };
    inline const auto& getInfo() const { return mStorageImageInfo; };

    inline void setLayout(vk::ImageLayout layout) { mStorageImage.transitionToLayout(layout); }
};

typedef std::unordered_map<std::string, StorageImage> RenderTargets;

}

