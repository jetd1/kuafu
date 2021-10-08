//
// Modified by Jet <i@jetd.me> 2021.
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

#include <fstream>
#include <iomanip>
#include <iostream>
#include <ktx.h>
#include <map>
#include <optional>
#include <tuple>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include <stb/stb_image.h>
#include <vulkan/vulkan.hpp>
#include <filesystem>

// Requires compiler with support for C++17.

// Define the following three lines once in any .cpp source file.
// #define VULKAN_HPP_STORAGE_SHARED
// #define VULKAN_HPP_STORAGE_SHARED_EXPORT
// VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#define VK_CORE_ASSERT( statement, message )         \
  if ( !statement )                                  \
  {                                                  \
    std::cerr << "vkCore: " << message << std::endl; \
    throw std::runtime_error( "vkCore: " #message ); \
  }

#define VK_CORE_THROW( ... ) details::log( true, "vkCore: ", __VA_ARGS__ );

#define VK_CORE_ENABLE_UNIQUE

#define VK_CORE_ASSERT_COMPONENTS

#ifdef VK_CORE_ASSERT_COMPONENTS
  #define VK_CORE_ASSERT_DEVICE VK_CORE_ASSERT( global::device, "Invalid device." )
#else
  #define VK_CORE_ASSERT_DEVICE
#endif

//#define VK_CORE_LOGGING

#ifdef VK_CORE_LOGGING
  #define VK_CORE_LOG( ... ) details::log( false, "vkCore: ", __VA_ARGS__ )
#else
  #define VK_CORE_LOG( ... )
#endif

namespace vkCore
{
  namespace global
  {
    inline vk::PhysicalDeviceLimits physicalDeviceLimits;
    inline vk::PhysicalDevice physicalDevice = nullptr;
    inline vk::Instance instance             = nullptr;
    inline vk::Device device                 = nullptr;
    inline vk::SwapchainKHR swapchain        = nullptr;
    inline vk::SurfaceKHR surface            = nullptr;
    inline vk::Queue graphicsQueue           = nullptr;
    inline vk::Queue transferQueue           = nullptr;
    inline vk::Queue computeQueue            = nullptr; // @todo Is not created.
    inline vk::CommandPool graphicsCmdPool   = nullptr;
    inline vk::CommandPool transferCmdPool   = nullptr;
    inline vk::CommandPool computeCmdPool    = nullptr; // @todo Is not created.
    inline uint32_t graphicsFamilyIndex      = 0U;
    inline uint32_t transferFamilyIndex      = 0U;
    inline uint32_t dataCopies               = 2U;
    inline uint32_t swapchainImageCount      = 0U;
    inline float queuePriority               = 1.0F;
  } // namespace global

  namespace details
  {
    template <typename... Args>
    inline void log( bool error, Args&&... args )
    {
      std::stringstream temp;
      ( temp << ... << args );

      std::cout << temp.str( ) << std::endl;

      if ( error )
      {
        throw std::runtime_error( temp.str( ) );
      }
    }

    /// Used to find any given element inside a STL container.
    /// @param value The value to search for.
    /// @param values The STL container of the same type as value.
    /// @return Returns true, if value was found in values.
    template <typename T, typename Container>
    auto find( T value, const Container& values ) -> bool
    {
      for ( const auto& it : values )
      {
        if ( it == value )
          return true;
      }

      return false;
    }

    /// A template function for unwrapping data structures with a unique handle.
    /// @param data A vector of any data structure with a unique handle.
    /// @return Returns a vector of the given data type but without the unique handles.
    template <typename Out, typename In>
    auto unpack( const std::vector<In>& data ) -> std::vector<Out>
    {
      std::vector<Out> result( data.size( ) );

      for ( size_t i = 0; i < result.size( ); ++i )
        result[i] = data[i].get( );

      return result;
    }

  } // namespace details

  // --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  // Utility Functions
  // --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

  inline auto findMemoryType( vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties ) -> uint32_t
  {
    static vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties( );

    for ( uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i )
    {
      if ( ( ( typeFilter & ( 1 << i ) ) != 0U ) && ( memoryProperties.memoryTypes[i].propertyFlags & properties ) == properties )
      {
        return i;
      }
    }

    VK_CORE_THROW( "vkCore: Failed to find suitable memory type." );

    return 0U;
  }

  template <typename T>
  auto getMemoryRequirements( const T& object )
  {
    vk::MemoryRequirements memoryRequirements;

    if constexpr ( std::is_same<T, vk::Buffer>::value )
    {
      memoryRequirements = global::device.getBufferMemoryRequirements( object );
    }
    else if constexpr ( std::is_same<T, vk::Image>::value )
    {
      memoryRequirements = global::device.getImageMemoryRequirements( object );
    }
    else if constexpr ( std::is_same<T, vk::UniqueBuffer>::value )
    {
      memoryRequirements = global::device.getBufferMemoryRequirements( object.get( ) );
    }
    else if constexpr ( std::is_same<T, vk::UniqueImage>::value )
    {
      memoryRequirements = global::device.getImageMemoryRequirements( object.get( ) );
    }
    else
    {
      VK_CORE_THROW( "vkCore: Failed to retrieve memory requirements for the provided type." );
    }

    return memoryRequirements;
  }

  inline auto parseShader( std::string_view shaderPath, std::string_view glslcPath ) -> std::vector<char>
  {
    std::string delimiter = "/";
    std::string pathToFile;
    std::string fileName;

    size_t pos = shaderPath.find_last_of( delimiter );
    if ( pos != std::string::npos )
    {
      pathToFile = std::string( shaderPath.substr( 0, pos + 1 ) );
      fileName   = std::string( shaderPath.substr( pos + 1 ) );
    }
    else
    {
      VK_CORE_THROW( "Failed to process shader path." );
    }

    std::string fileNameOut = fileName + ".spv";

    if (std::filesystem::exists(glslcPath)) {
        VK_CORE_LOG("glslc found, trying to recompile shaders from source.");
        std::stringstream command;
        command << glslcPath << " " << shaderPath << " -o " << pathToFile << fileNameOut << " --target-env=vulkan1.2";

        std::system( command.str( ).c_str( ) );
    } else
        VK_CORE_LOG("No glslc found, trying to use precompiled shaders.");

    // Read the file and retrieve the source.
    std::string pathToShaderSourceFile = pathToFile + fileNameOut;
    std::ifstream file( pathToShaderSourceFile, std::ios::ate | std::ios::binary );

    if ( !file.is_open( ) )
    {
      VK_CORE_THROW( "Failed to open shader .spv: ", pathToShaderSourceFile );
    }

    size_t fileSize = static_cast<size_t>( file.tellg( ) );
    std::vector<char> buffer( fileSize );

    file.seekg( 0 );
    file.read( buffer.data( ), fileSize );

    file.close( );

    return buffer;
  }

  inline auto isPhysicalDeviceQueueComplete( vk::PhysicalDevice physicalDevice ) -> bool
  {
    auto queueFamilies           = physicalDevice.getQueueFamilyProperties( );
    auto queueFamilyIndicesCount = static_cast<uint32_t>( queueFamilies.size( ) );

    // Get all possible queue family indices with transfer support.
    std::vector<uint32_t> graphicsQueueFamilyIndices;
    std::vector<uint32_t> transferQueueFamilyIndices;
    std::vector<uint32_t> computeQueueFamilyIndices;

    for ( uint32_t index = 0; index < queueFamilyIndicesCount; ++index )
    {
      // Make sure the current queue family index contains at least one queue.
      if ( queueFamilies[index].queueCount == 0 )
      {
        continue;
      }

      if ( ( queueFamilies[index].queueFlags & vk::QueueFlagBits::eGraphics ) == vk::QueueFlagBits::eGraphics )
      {
        if ( !global::surface || physicalDevice.getSurfaceSupportKHR( index, global::surface ) != 0U )
        {
          graphicsQueueFamilyIndices.push_back( index );
        }
      }

      if ( ( queueFamilies[index].queueFlags & vk::QueueFlagBits::eTransfer ) == vk::QueueFlagBits::eTransfer )
      {
        transferQueueFamilyIndices.push_back( index );
      }

      if ( ( queueFamilies[index].queueFlags & vk::QueueFlagBits::eCompute ) == vk::QueueFlagBits::eCompute )
      {
        computeQueueFamilyIndices.push_back( index );
      }
    }

    if ( graphicsQueueFamilyIndices.empty( ) || computeQueueFamilyIndices.empty( ) || transferQueueFamilyIndices.empty( ) )
    {
      return false;
    }

    return true;
  }

  inline auto isPhysicalDeviceWithDedicatedTransferQueueFamily( vk::PhysicalDevice physicalDevice ) -> bool
  {
    auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties( );

    for ( auto& queueFamilyPropertie : queueFamilyProperties )
    {
      if ( ( queueFamilyPropertie.queueFlags & vk::QueueFlagBits::eGraphics ) != vk::QueueFlagBits::eGraphics )
      {
        return true;
      }
    }

    return false;
  }

  // @todo Add feature check.
  inline auto evaluatePhysicalDevice( vk::PhysicalDevice physicalDevice ) -> std::pair<uint32_t, std::string>
  {
    uint32_t score = 0U;

    auto properties = physicalDevice.getProperties( );

    std::string deviceName = properties.deviceName;

    // Always prefer dedicated GPUs.
    if ( properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu )
    {
      score += 100U;
    }
    else
    {
      return { 0U, deviceName };
    }

    // Prefer newer Vulkan support.
#ifdef VK_API_VERSION_1_2
    if ( properties.apiVersion >= VK_API_VERSION_1_2 )
    {
      score += 10U;
    }
#endif

    // Check if the physical device has compute, transfer and graphics families.
    if ( isPhysicalDeviceQueueComplete( physicalDevice ) )
    {
      score += 100U;
    }
    else
    {
      return { 0U, deviceName };
    }

    // Check if there is a queue family for transfer operations that is not the graphics queue itself.
    if ( isPhysicalDeviceWithDedicatedTransferQueueFamily( physicalDevice ) )
    {
      score += 25;
    }

    return { score, deviceName };
  }

  inline auto initPhysicalDevice( ) -> vk::PhysicalDevice
  {
    vk::PhysicalDevice physicalDevice;

    auto physicalDevices = global::instance.enumeratePhysicalDevices( );

    std::vector<std::pair<unsigned int, std::string>> results;

    unsigned int score = 0;
    for ( const auto& it : physicalDevices )
    {
      auto temp = evaluatePhysicalDevice( it );
      results.push_back( temp );

      if ( temp.first > score )
      {
        physicalDevice = it;
        score          = temp.first;
      }
    }

#ifdef VK_CORE_LOGGING
    // Print information about all GPUs available on the machine.
    const std::string_view separator = "===================================================================";

    std::cout << "vkCore: Physical device report: "
              << "\n\n"
              << separator << "\n "
              << " Device name "
              << "\t\t\t"
              << "Score" << std::endl
              << separator << "\n ";

    for ( const auto& result : results )
    {
      std::cout << std::left << std::setw( 32 ) << std::setfill( ' ' ) << result.second << std::left << std::setw( 32 ) << std::setfill( ' ' ) << result.first << std::endl;
    }

    std::cout << std::endl;
#endif

    VK_CORE_ASSERT( physicalDevice, "No suitable physical device was found." );

    // Print information about the GPU that was selected.
    auto properties = physicalDevice.getProperties( );
    VK_CORE_LOG( "Selected GPU: ", properties.deviceName );

    global::physicalDeviceLimits = properties.limits;
    global::physicalDevice       = physicalDevice;

    return physicalDevice;
  }

  inline void checkInstanceLayersSupport( const std::vector<const char*>& layers )
  {
    auto properties = vk::enumerateInstanceLayerProperties( );

    for ( const char* name : layers )
    {
      bool found = false;
      for ( const auto& property : properties )
      {
        if ( strcmp( property.layerName, name ) == 0 )
        {
          found = true;
          break;
        }
      }

      if ( !found )
      {
        VK_CORE_THROW( "Validation layer ", name, " is not available on this device." );
      }

      VK_CORE_LOG( "Added layer: ", name, "." );
    }
  }

  inline uint32_t assessVulkanVersion( uint32_t minVersion )
  {
    uint32_t apiVersion = vk::enumerateInstanceVersion( );

#if defined( VK_API_VERSION_1_0 ) && !defined( VK_API_VERSION_1_1 ) && !defined( VK_API_VERSION_1_2 )
    VK_CORE_LOG( "Found Vulkan SDK API version 1.0." );
#endif

#if defined( VK_API_VERSION_1_1 ) && !defined( VK_API_VERSION_1_2 )
    VK_CORE_LOG( "Found Vulkan SDK API version 1.1." );
#endif

#if defined( VK_API_VERSION_1_2 )
    VK_CORE_LOG( "Found Vulkan SDK API version 1.2." );
#endif

    if ( minVersion > apiVersion )
    {
      VK_CORE_THROW( "Local Vulkan SDK API version is outdated." );
    }

    return apiVersion;
  }

  inline void checkInstanceExtensionsSupport( const std::vector<const char*>& extensions )
  {
    auto properties = vk::enumerateInstanceExtensionProperties( );

    for ( const char* name : extensions )
    {
      bool found = false;
      for ( const auto& property : properties )
      {
        if ( strcmp( property.extensionName, name ) == 0 )
        {
          found = true;
          break;
        }
      }

      if ( !found )
      {
        VK_CORE_THROW( "Instance extensions ", std::string( name ), " is not available on this device." )
      }

      VK_CORE_LOG( "Added instance extension: ", name, "." );
    }
  }

  inline void checkDeviceExtensionSupport( const std::vector<const char*>& extensions )
  {
    // Stores the name of the extension and a bool that tells if they were found or not.
    std::map<const char*, bool> requiredExtensions;

    for ( const auto& extension : extensions )
    {
      requiredExtensions.emplace( extension, false );
    }

    std::vector<vk::ExtensionProperties> physicalDeviceExtensions = global::physicalDevice.enumerateDeviceExtensionProperties( );

    // Iterates over all enumerated physical device extensions to see if they are available.
    for ( const auto& physicalDeviceExtension : physicalDeviceExtensions )
    {
      for ( auto& requiredphysicalDeviceExtension : requiredExtensions )
      {
        if ( strcmp( physicalDeviceExtension.extensionName, requiredphysicalDeviceExtension.first ) == 0 )
        {
          requiredphysicalDeviceExtension.second = true;
        }
      }
    }

    // Give feedback on the previous operations.
    for ( const auto& requiredphysicalDeviceExtension : requiredExtensions )
    {
      if ( !requiredphysicalDeviceExtension.second )
      {
        VK_CORE_THROW( "Missing physical device extension: ", requiredphysicalDeviceExtension.first );
      }
      else
      {
        VK_CORE_LOG( "Added device extension: ", requiredphysicalDeviceExtension.first );
      }
    }
  }

  inline void initQueueFamilyIndices( )
  {
    std::optional<uint32_t> graphicsFamilyIndex;
    std::optional<uint32_t> transferFamilyIndex;

    auto queueFamilyProperties = global::physicalDevice.getQueueFamilyProperties( );
    std::vector<uint32_t> queueFamilies( queueFamilyProperties.size( ) );

    bool dedicatedTransferQueueFamily = isPhysicalDeviceWithDedicatedTransferQueueFamily( global::physicalDevice );

    for ( uint32_t index = 0; index < static_cast<uint32_t>( queueFamilies.size( ) ); ++index )
    {
      if ( queueFamilyProperties[index].queueFlags & vk::QueueFlagBits::eGraphics && !graphicsFamilyIndex.has_value( ) )
      {
        if ( !global::surface || global::physicalDevice.getSurfaceSupportKHR( index, global::surface ) )
        {
          graphicsFamilyIndex = index;
        }
      }

      if ( dedicatedTransferQueueFamily )
      {
        if ( !( queueFamilyProperties[index].queueFlags & vk::QueueFlagBits::eGraphics ) && !transferFamilyIndex.has_value( ) )
        {
          transferFamilyIndex = index;
        }
      }
      else
      {
        if ( queueFamilyProperties[index].queueFlags & vk::QueueFlagBits::eTransfer && !transferFamilyIndex.has_value( ) )
        {
          transferFamilyIndex = index;
        }
      }
    }

    if ( !graphicsFamilyIndex.has_value( ) || !transferFamilyIndex.has_value( ) )
    {
      VK_CORE_THROW( "Failed to retrieve queue family indices." );
    }

    global::graphicsFamilyIndex = graphicsFamilyIndex.value( );
    global::transferFamilyIndex = transferFamilyIndex.value( );
  }

  inline std::vector<vk::DeviceQueueCreateInfo> getDeviceQueueCreateInfos( )
  {
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

    std::vector<uint32_t> queueFamilyIndices = { global::graphicsFamilyIndex, global::transferFamilyIndex };

    uint32_t index = 0;
    for ( const auto& queueFamilyIndex : queueFamilyIndices )
    {
      vk::DeviceQueueCreateInfo queueCreateInfo( { },                      // flags
                                                 queueFamilyIndex,         // queueFamilyIndex
                                                 1,                        // queueCount
                                                 &global::queuePriority ); // pQueuePriorties

      queueCreateInfos.push_back( queueCreateInfo );

      ++index;
    }

    return queueCreateInfos;
  }

  inline auto getImageCreateInfo( vk::Extent3D extent ) -> vk::ImageCreateInfo
  {
    return vk::ImageCreateInfo( { },                                                                     // flags
                                vk::ImageType::e2D,                                                      // imageType
//                                vk::Format::eR8G8B8A8Unorm,                                              // format
                                vk::Format::eR8G8B8A8Srgb,                                              // format
                                extent,                                                                  // extent
                                1U,                                                                      // mipLevels
                                1U,                                                                      // arrayLayers
                                vk::SampleCountFlagBits::e1,                                             // samples
                                vk::ImageTiling::eOptimal,                                               // tiling
                                vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, // usage
                                vk::SharingMode::eExclusive,                                             // sharingMode
                                global::graphicsFamilyIndex,                                             // queueFamilyIndexCount
                                nullptr,                                                                 // pQueueFamilyIndices
                                vk::ImageLayout::eUndefined );                                           // initialLayout
  }

  inline auto getImageViewCreateInfo( vk::Image image, vk::Format format, vk::ImageViewType viewType = vk::ImageViewType::e2D, vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor ) -> vk::ImageViewCreateInfo
  {
    vk::ComponentMapping components = { vk::ComponentSwizzle::eIdentity,
                                        vk::ComponentSwizzle::eIdentity,
                                        vk::ComponentSwizzle::eIdentity,
                                        vk::ComponentSwizzle::eIdentity };

    vk::ImageSubresourceRange subresourceRange = { aspectFlags, // aspectMask
                                                   0U,          // baseMipLevel
                                                   1U,          // levelCount
                                                   0U,          // baseArrayLayer
                                                   1U };        // layerCount

    return vk::ImageViewCreateInfo( { },                // flags
                                    image,              // image
                                    viewType,           // viewType
                                    format,             // format
                                    components,         // components
                                    subresourceRange ); // subresourceRange
  }

  inline auto getSamplerCreateInfo( ) -> vk::SamplerCreateInfo
  {
    return vk::SamplerCreateInfo( { },                              // flags
                                  vk::Filter::eLinear,              // magFilter
                                  vk::Filter::eLinear,              // minFilter
                                  vk::SamplerMipmapMode::eLinear,   // mipmapMode
                                  vk::SamplerAddressMode::eRepeat,  // addressModeU
                                  vk::SamplerAddressMode::eRepeat,  // addressModeV
                                  vk::SamplerAddressMode::eRepeat,  // addressModeW
                                  { },                              // mipLodBias,
                                  VK_TRUE,                          // anisotropyEnable
                                  16.0F,                            // maxAnisotropy
                                  VK_FALSE,                         // compareEnable
                                  vk::CompareOp::eAlways,           // compareOp
                                  { },                              // minLod
                                  { },                              // maxLod
                                  vk::BorderColor::eIntOpaqueBlack, // borderColor
                                  VK_FALSE );                       // unnormalizedCoordinates
  }

  inline auto getPoolSizes( const std::vector<vk::DescriptorSetLayoutBinding>& layoutBindings, uint32_t maxSets ) -> std::vector<vk::DescriptorPoolSize>
  {
    std::vector<vk::DescriptorPoolSize> result;
    result.reserve( layoutBindings.size( ) );

    for ( const auto& layoutBinding : layoutBindings )
    {
      vk::DescriptorPoolSize poolSize( layoutBinding.descriptorType,    // type
                                       layoutBinding.descriptorCount ); // count

      result.push_back( poolSize );
    }

    return result;
  }

  inline auto findSupportedImageFormat( vk::PhysicalDevice physicalDevice, const std::vector<vk::Format>& formatsToTest, vk::FormatFeatureFlagBits features, vk::ImageTiling tiling ) -> vk::Format
  {
    for ( vk::Format format : formatsToTest )
    {
      auto props = physicalDevice.getFormatProperties( format );

      if ( tiling == vk::ImageTiling::eLinear && ( props.linearTilingFeatures & features ) == features )
      {
        return format;
      }
      if ( tiling == vk::ImageTiling::eOptimal && ( props.optimalTilingFeatures & features ) == features )
      {
        return format;
      }
    }

    VK_CORE_THROW( "Failed to retrieve any supported image format." );

    return vk::Format::eUndefined;
  }

///////////////////////////////////////////////////////////////////////////////
// Return the access flag for an image layout
inline vk::AccessFlagBits accessFlagsForImageLayout(vk::ImageLayout layout)
{
    switch(layout) {
        case vk::ImageLayout::ePreinitialized:
            return vk::AccessFlagBits::eHostWrite;
        case vk::ImageLayout::eTransferDstOptimal:
            return vk::AccessFlagBits::eTransferWrite;
        case vk::ImageLayout::eTransferSrcOptimal:
            return vk::AccessFlagBits::eTransferRead;
        case vk::ImageLayout::eColorAttachmentOptimal:
            return vk::AccessFlagBits::eColorAttachmentWrite;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            return vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            return vk::AccessFlagBits::eShaderRead;
        default:
            return vk::AccessFlagBits{};
    }
}

inline vk::PipelineStageFlagBits pipelineStageForLayout(vk::ImageLayout layout)
{
    switch(layout) {
        case vk::ImageLayout::eTransferDstOptimal:
        case vk::ImageLayout::eTransferSrcOptimal:
            return vk::PipelineStageFlagBits::eTransfer;
        case vk::ImageLayout::eColorAttachmentOptimal:
            return vk::PipelineStageFlagBits::eColorAttachmentOutput;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            return vk::PipelineStageFlagBits::eAllCommands;  // We do this to allow queue other than graphic
            // return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            return vk::PipelineStageFlagBits::eAllCommands;  // We do this to allow queue other than graphic
            // return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case vk::ImageLayout::ePreinitialized:
            return vk::PipelineStageFlagBits::eHost;
        case vk::ImageLayout::eUndefined:
            return vk::PipelineStageFlagBits::eTopOfPipe;
        default:
            return vk::PipelineStageFlagBits::eBottomOfPipe;
    }
}



  /// Simplifies the process of setting up an image memory barrier info.
  /// @param image The vulkan image.
  /// @param oldLayout The current image layout of the given vulkan image.
  /// @param newLayout The target image layout.
  /// @param subresourceRange The image view's subresource range.
  /// @return Returns a tuple containing the actual image memory barrier as well as the source stage mask and the destination stage mask.
  inline auto getImageMemoryBarrierInfo( vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, const vk::ImageSubresourceRange* subresourceRange = nullptr ) -> std::tuple<vk::ImageMemoryBarrier, vk::PipelineStageFlags, vk::PipelineStageFlags>
  {
    // TODO: not style conform.
    vk::ImageMemoryBarrier barrier;
    barrier.oldLayout           = oldLayout;
    barrier.newLayout           = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image               = image;

    if ( subresourceRange == nullptr )
    {
      barrier.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
      barrier.subresourceRange.baseMipLevel   = 0;
      barrier.subresourceRange.levelCount     = 1;
      barrier.subresourceRange.baseArrayLayer = 0;
      barrier.subresourceRange.layerCount     = 1;
    }
    else
    {
      barrier.subresourceRange = *subresourceRange;
    }

    vk::PipelineStageFlags srcStageMask = vk::PipelineStageFlagBits::eAllCommands;
    vk::PipelineStageFlags dstStageMask = vk::PipelineStageFlagBits::eAllCommands;

    if ( oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal )
    {
      barrier.srcAccessMask = { };
      barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

      srcStageMask = vk::PipelineStageFlagBits::eTopOfPipe;
      dstStageMask = vk::PipelineStageFlagBits::eTransfer;
    }
    else if ( oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal )
    {
      barrier.srcAccessMask = { };
      barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;

      srcStageMask = vk::PipelineStageFlagBits::eTopOfPipe;
      dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    }
    else if ( oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal )
    {
      barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
      barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

      srcStageMask = vk::PipelineStageFlagBits::eTransfer;
      dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else if ( oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eGeneral )
    {
      // nothing to do.
    }
    else if ( oldLayout == vk::ImageLayout::eGeneral && newLayout == vk::ImageLayout::eTransferSrcOptimal )
    {
      barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
    }
    else if ( oldLayout == vk::ImageLayout::eTransferSrcOptimal && newLayout == vk::ImageLayout::eGeneral )
    {
      barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
    }
    else if ( oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::ePresentSrcKHR )
    {
      barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    }
    else if ( oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::ePresentSrcKHR )
    {
      barrier.srcAccessMask = { };
    }
    else if ( oldLayout == vk::ImageLayout::ePresentSrcKHR && newLayout == vk::ImageLayout::eColorAttachmentOptimal )
    {
      barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    }
    // TODO: jet: check the following 20 lines
    else if ( oldLayout == vk::ImageLayout::ePresentSrcKHR && newLayout == vk::ImageLayout::eTransferSrcOptimal )
    {
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        dstStageMask = vk::PipelineStageFlagBits::eTransfer;
    }
    else if ( oldLayout == vk::ImageLayout::eColorAttachmentOptimal && newLayout == vk::ImageLayout::eTransferSrcOptimal )
    {
      barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
      barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

      srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
      dstStageMask = vk::PipelineStageFlagBits::eTransfer;
    }
    else if ( oldLayout == vk::ImageLayout::eTransferSrcOptimal && newLayout == vk::ImageLayout::ePresentSrcKHR )
    {
        srcStageMask = vk::PipelineStageFlagBits::eTransfer;
        dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
    }
    else if ( oldLayout == vk::ImageLayout::eColorAttachmentOptimal && newLayout == vk::ImageLayout::ePresentSrcKHR )
    {
      barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    }
    else if ( oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eColorAttachmentOptimal )
    {
      barrier.srcAccessMask = { };
      barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    }
    else if ( oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eColorAttachmentOptimal )
    {
      barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
      barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    }
    else
    {
        barrier.srcAccessMask    = accessFlagsForImageLayout(oldLayout);
        barrier.dstAccessMask    = accessFlagsForImageLayout(newLayout);

        srcStageMask     = pipelineStageForLayout(oldLayout);
        dstStageMask     = pipelineStageForLayout(newLayout);
    }

    return { barrier, srcStageMask, dstStageMask };
  }

  /// Retrieves the depth format supported by a given physical device.
  /// @param physicalDevice The physical device to check.
  /// @return Returns the supported depth format.
  inline auto getSupportedDepthFormat( vk::PhysicalDevice physicalDevice ) -> vk::Format
  {
    std::vector<vk::Format> candidates { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint };
    return findSupportedImageFormat( physicalDevice, candidates, vk::FormatFeatureFlagBits::eDepthStencilAttachment, vk::ImageTiling::eOptimal );
  }

  /// Transitions the image layout of any given image using an already existing command buffer.
  /// @param image The vulkan image for which you want to change the image layout.
  /// @param oldLayout The current image layout of the given vulkan image.
  /// @param newLayout The target image layout.
  /// @param commandBuffer The command buffer that will be used. It must be in the recording stage.
  inline void transitionImageLayout( vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::CommandBuffer commandBuffer )
  {
    auto barrierInfo = getImageMemoryBarrierInfo( image, oldLayout, newLayout );

    commandBuffer.pipelineBarrier( std::get<1>( barrierInfo ), // srcStageMask
                                   std::get<2>( barrierInfo ), // dstStageMask
                                   vk::DependencyFlagBits::eByRegion,
                                   0,
                                   nullptr,
                                   0,
                                   nullptr,
                                   1,
                                   &std::get<0>( barrierInfo ) ); // barrier
  }

  inline auto getPipelineShaderStageCreateInfo( vk::ShaderStageFlagBits stage, vk::ShaderModule module, const char* name = "main", vk::SpecializationInfo* specializationInfo = nullptr ) -> vk::PipelineShaderStageCreateInfo
  {
    return vk::PipelineShaderStageCreateInfo( { },                  // flags
                                              stage,                // stage
                                              module,               // module
                                              name,                 // pName
                                              specializationInfo ); // pSpecializationInfo
  }

  /// @cond INTERNAL
  VKAPI_ATTR inline auto VKAPI_CALL debugMessengerCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                            VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                            const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                                            void* userData ) -> VkBool32
  {
    std::array<char, 64> prefix;
    char* message = static_cast<char*>( malloc( strlen( callbackData->pMessage ) + 500 ) );
    assert( message );

    if ( ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT ) != 0 )
    {
      strcpy( prefix.data( ), "VERBOSE: " );
    }
    else if ( ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT ) != 0 )
    {
      strcpy( prefix.data( ), "INFO: " );
    }
    else if ( ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ) != 0 )
    {
      strcpy( prefix.data( ), "WARNING: " );
    }
    else if ( ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ) != 0 )
    {
      strcpy( prefix.data( ), "ERROR: " );
    }

    if ( ( messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT ) != 0U )
    {
      strcat( prefix.data( ), "GENERAL" );
    }
    else
    {
      if ( ( messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT ) != 0U )
      {
        strcat( prefix.data( ), "VALIDATION" );
        if ( ( messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT ) != 0U )
        {
          strcat( prefix.data( ), " | PERFORMANCE" );
        }
      }
    }

    sprintf( message,
             "%s \n\tID Number: %d\n\tID String: %s \n\tMessage:   %s",
             prefix.data( ),
             callbackData->messageIdNumber,
             callbackData->pMessageIdName,
             callbackData->pMessage );

    printf( "%s\n", message );
    fflush( stdout );
    free( message );

    if ( ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ) != 0 )
    {
      //VK_CORE_THROW("Fatal error");
    }

    return VK_FALSE;
  }

  // --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  // Initializer Functions
  // --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

  inline auto initFence( vk::FenceCreateFlags flags = vk::FenceCreateFlagBits::eSignaled ) -> vk::Fence
  {
    vk::FenceCreateInfo createInfo( flags );

    auto fence = global::device.createFence( createInfo );
    VK_CORE_ASSERT( fence, "Failed to create fence." );

    return fence;
  }

  inline auto initSemaphore( vk::SemaphoreCreateFlags flags = { } ) -> vk::Semaphore
  {
    vk::SemaphoreCreateInfo createInfo( flags );

    auto semaphore = global::device.createSemaphore( createInfo );
    VK_CORE_ASSERT( semaphore, "Failed to create semaphore." );

    return semaphore;
  }

  inline auto initCommandPool( uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags flags = { } ) -> vk::CommandPool
  {
    vk::CommandPoolCreateInfo createInfo( flags, queueFamilyIndex );

    auto commandPool = global::device.createCommandPool( createInfo );
    VK_CORE_ASSERT( commandPool, "Failed to create command pool." );

    return commandPool;
  }

  inline auto initDescriptorPool( const std::vector<vk::DescriptorPoolSize>& poolSizes, uint32_t maxSets = 1, vk::DescriptorPoolCreateFlags flags = { } ) -> vk::DescriptorPool
  {
    vk::DescriptorPoolCreateInfo createInfo( flags,                                      // flags
                                             maxSets,                                    // maxSets
                                             static_cast<uint32_t>( poolSizes.size( ) ), // poolSizeCount
                                             poolSizes.data( ) );                        // pPoolSizes

    auto descriptorPool = global::device.createDescriptorPool( createInfo );
    VK_CORE_ASSERT( descriptorPool, "Failed to create unique descriptor pool." );

    return descriptorPool;
  }

  inline auto allocateDescriptorSets( const vk::DescriptorPool& pool, const vk::DescriptorSetLayout& layout ) -> std::vector<vk::DescriptorSet>
  {
    std::vector<vk::DescriptorSetLayout> layouts( static_cast<size_t>( global::dataCopies ), layout );

    vk::DescriptorSetAllocateInfo allocateInfo( pool,
                                                global::dataCopies,
                                                layouts.data( ) );

    auto sets = global::device.allocateDescriptorSets( allocateInfo );

    for ( auto set : sets )
    {
      VK_CORE_ASSERT( set, "Failed to create unique descriptor sets." );
    }

    return sets;
  }

  template <typename T>
  auto allocateMemory( const T& object, vk::MemoryPropertyFlags propertyFlags = { }, void* pNext = nullptr )
  {
    vk::MemoryRequirements memoryRequirements = getMemoryRequirements( object );

    vk::MemoryAllocateInfo allocateInfo( memoryRequirements.size,                                                                      // allocationSize
                                         findMemoryType( global::physicalDevice, memoryRequirements.memoryTypeBits, propertyFlags ) ); // memoryTypeIndex

    allocateInfo.pNext = pNext;

    auto memory = global::device.allocateMemory( allocateInfo );
    VK_CORE_ASSERT( memory, "Failed to allocate memory." );

    return memory;
  }

  inline auto initImageView( const vk::ImageViewCreateInfo& createInfo ) -> vk::ImageView
  {
    auto imageView = global::device.createImageView( createInfo );
    VK_CORE_ASSERT( imageView, "Failed to create image view." );

    return imageView;
  }

  inline auto initSampler( const vk::SamplerCreateInfo& createInfo ) -> vk::Sampler
  {
    auto sampler = global::device.createSampler( createInfo );
    VK_CORE_ASSERT( sampler, "Failed to create sampler." );

    return sampler;
  }

  inline auto initFramebuffer( const std::vector<vk::ImageView>& attachments, vk::RenderPass renderPass, const vk::Extent2D& extent ) -> vk::Framebuffer
  {
    vk::FramebufferCreateInfo createInfo( { },                                          // flags
                                          renderPass,                                   // renderPass
                                          static_cast<uint32_t>( attachments.size( ) ), // attachmentCount
                                          attachments.data( ),                          // pAttachments
                                          extent.width,                                 // width
                                          extent.height,                                // height
                                          1U );                                         // layers

    auto framebuffer = global::device.createFramebuffer( createInfo );
    VK_CORE_ASSERT( framebuffer, "Failed to create framebuffer." );

    return framebuffer;
  }

  inline auto initQueryPool( uint32_t count, vk::QueryType type ) -> vk::QueryPool
  {
    vk::QueryPoolCreateInfo createInfo( { },   // flags
                                        type,  // queryType
                                        count, // queryCount
                                        { } ); // pipelineStatistics

    auto queryPool = global::device.createQueryPool( createInfo );
    VK_CORE_ASSERT( queryPool, "Failed to create query pool." );

    return queryPool;
  }

  inline auto initShaderModule( std::string_view shaderPath, std::string_view glslcPath ) -> vk::ShaderModule
  {
    std::vector<char> source = parseShader( shaderPath, glslcPath );

    vk::ShaderModuleCreateInfo createInfo( { },                                                   // flags
                                           source.size( ),                                        // codeSize
                                           reinterpret_cast<const uint32_t*>( source.data( ) ) ); // pCode

    auto shaderModule = global::device.createShaderModule( createInfo );
    VK_CORE_ASSERT( shaderModule, "Failed to create shader module." );

    return shaderModule;
  }

  inline auto initInstance( const std::vector<const char*>& layers, std::vector<const char*>& extensions, uint32_t minVersion = VK_API_VERSION_1_0 ) -> vk::Instance
  {
    vk::DynamicLoader dl;
    auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>( "vkGetInstanceProcAddr" );
    VULKAN_HPP_DEFAULT_DISPATCHER.init( vkGetInstanceProcAddr );

    // Check if all extensions and layers needed are available.
    checkInstanceLayersSupport( layers );
    checkInstanceExtensionsSupport( extensions );

    // Start creating the instance.
    vk::ApplicationInfo appInfo;
    appInfo.apiVersion = assessVulkanVersion( minVersion );

    vk::InstanceCreateInfo createInfo( { },                                         // flags
                                       &appInfo,                                    // pApplicationInfo
                                       static_cast<uint32_t>( layers.size( ) ),     // enabledLayerCount
                                       layers.data( ),                              // ppEnabledLayerNames
                                       static_cast<uint32_t>( extensions.size( ) ), // enabledExtensionCount
                                       extensions.data( ) );                        // ppEnabledExtensionNames

    auto instance    = createInstance( createInfo );
    global::instance = instance;
    VK_CORE_ASSERT( instance, "Failed to create instance." );

    VULKAN_HPP_DEFAULT_DISPATCHER.init( instance );

    return instance;
  }

  inline auto initDevice( std::vector<const char*>& extensions, const std::optional<vk::PhysicalDeviceFeatures>& features, const std::optional<vk::PhysicalDeviceFeatures2>& features2 = { } ) -> vk::Device
  {
    checkDeviceExtensionSupport( extensions );

    auto queueCreateInfos = getDeviceQueueCreateInfos( );

    vk::DeviceCreateInfo createInfo( { },                                                                                           // flags
                                     static_cast<uint32_t>( queueCreateInfos.size( ) ),                                             // queueCreateInfoCount
                                     queueCreateInfos.data( ),                                                                      // pQueueCreateInfos
                                     0,                                                                                             // enabledLayerCount
                                     nullptr,                                                                                       // ppEnabledLayerNames
                                     static_cast<uint32_t>( extensions.size( ) ),                                                   // enabledExtensionCount
                                     extensions.data( ),                                                                            // ppEnabledExtensionNames
                                     features2.has_value( ) ? nullptr : ( features.has_value( ) ? &features.value( ) : nullptr ) ); // pEnabledFeatures - must be NULL because the VkDeviceCreateInfo::pNext chain includes VkPhysicalDeviceFeatures2.

    createInfo.pNext = features2.has_value( ) ? &features2.value( ) : nullptr;

    auto device    = global::physicalDevice.createDevice( createInfo );
    global::device = device;
    VK_CORE_ASSERT( device, "Failed to create logical device." );

    VULKAN_HPP_DEFAULT_DISPATCHER.init( device );

    return device;
  }

  // --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#ifdef VK_CORE_ENABLE_UNIQUE
  inline auto initFenceUnique( vk::FenceCreateFlags flags = vk::FenceCreateFlagBits::eSignaled ) -> vk::UniqueFence
  {
    vk::FenceCreateInfo createInfo( flags );

    auto fence = global::device.createFenceUnique( createInfo );
    VK_CORE_ASSERT( fence, "Failed to create unique fence." );

    return std::move( fence );
  }

  inline auto initSemaphoreUnique( vk::SemaphoreCreateFlags flags = { } ) -> vk::UniqueSemaphore
  {
    vk::SemaphoreCreateInfo createInfo( flags );

    auto semaphore = global::device.createSemaphoreUnique( createInfo );
    VK_CORE_ASSERT( semaphore, "Failed to create unique semaphore." );

    return std::move( semaphore );
  }

  inline auto initCommandPoolUnique( uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags flags = { } ) -> vk::UniqueCommandPool
  {
    vk::CommandPoolCreateInfo createInfo( flags, queueFamilyIndex );

    auto commandPool = global::device.createCommandPoolUnique( createInfo );
    VK_CORE_ASSERT( commandPool, "Failed to create unique command pool." );

    return std::move( commandPool );
  }

  inline auto initDescriptorPoolUnique( const std::vector<vk::DescriptorPoolSize>& poolSizes, uint32_t maxSets = 1, vk::DescriptorPoolCreateFlags flags = { } ) -> vk::UniqueDescriptorPool
  {
    vk::DescriptorPoolCreateInfo createInfo( flags,                                      // flags
                                             maxSets,                                    // maxSets
                                             static_cast<uint32_t>( poolSizes.size( ) ), // poolSizeCount
                                             poolSizes.data( ) );                        // pPoolSizes

    auto descriptorPool = global::device.createDescriptorPoolUnique( createInfo );
    VK_CORE_ASSERT( descriptorPool, "Failed to create unique descriptor pool." );

    return std::move( descriptorPool );
  }

  inline auto allocateDescriptorSetsUnique( const vk::UniqueDescriptorPool& pool, const vk::UniqueDescriptorSetLayout& layout ) -> std::vector<vk::UniqueDescriptorSet>
  {
    std::vector<vk::DescriptorSetLayout> layouts( static_cast<size_t>( global::dataCopies ), layout.get( ) );

    vk::DescriptorSetAllocateInfo allocateInfo( pool.get( ),
                                                global::dataCopies,
                                                layouts.data( ) );

    auto sets = global::device.allocateDescriptorSetsUnique( allocateInfo );

    for ( const auto& set : sets )
    {
      VK_CORE_ASSERT( set, "Failed to create unique descriptor sets." );
    }

    return std::move( sets );
  }

  template <typename T>
  auto allocateMemoryUnique( const T& object, vk::MemoryPropertyFlags propertyFlags = { }, void* pNext = nullptr )
  {
    vk::MemoryRequirements memoryRequirements = getMemoryRequirements( object );

    vk::MemoryAllocateInfo allocateInfo( memoryRequirements.size,                                                                      // allocationSize
                                         findMemoryType( global::physicalDevice, memoryRequirements.memoryTypeBits, propertyFlags ) ); // memoryTypeIndex

    allocateInfo.pNext = pNext;

    auto memory = global::device.allocateMemoryUnique( allocateInfo );
    VK_CORE_ASSERT( memory, "Failed to allocate memory." );

    return std::move( memory );
  }

  inline auto initImageViewUnique( const vk::ImageViewCreateInfo& createInfo ) -> vk::UniqueImageView
  {
    auto imageView = global::device.createImageViewUnique( createInfo );
    VK_CORE_ASSERT( imageView, "Failed to create image view." );

    return std::move( imageView );
  }

  inline auto initSamplerUnique( const vk::SamplerCreateInfo& createInfo ) -> vk::UniqueSampler
  {
    auto sampler = global::device.createSamplerUnique( createInfo );
    VK_CORE_ASSERT( sampler, "Failed to create sampler." );

    return std::move( sampler );
  }

  inline auto initFramebufferUnique( const std::vector<vk::ImageView>& attachments, vk::RenderPass renderPass, const vk::Extent2D& extent ) -> vk::UniqueFramebuffer
  {
    vk::FramebufferCreateInfo createInfo( { },                                          // flags
                                          renderPass,                                   // renderPass
                                          static_cast<uint32_t>( attachments.size( ) ), // attachmentCount
                                          attachments.data( ),                          // pAttachments
                                          extent.width,                                 // width
                                          extent.height,                                // height
                                          1U );                                         // layers

    auto framebuffer = global::device.createFramebufferUnique( createInfo );
    VK_CORE_ASSERT( framebuffer, "Failed to create framebuffer." );

    return std::move( framebuffer );
  }

  inline auto initQueryPoolUnique( uint32_t count, vk::QueryType type ) -> vk::UniqueQueryPool
  {
    vk::QueryPoolCreateInfo createInfo( { },   // flags
                                        type,  // queryType
                                        count, // queryCount
                                        { } ); // pipelineStatistics

    auto queryPool = global::device.createQueryPoolUnique( createInfo );
    VK_CORE_ASSERT( queryPool, "Failed to create query pool." );

    return std::move( queryPool );
  }

  inline auto initShaderModuleUnique( std::string_view shaderPath, std::string_view glslcPath ) -> vk::UniqueShaderModule
  {
    std::vector<char> source = parseShader( shaderPath, glslcPath );

    vk::ShaderModuleCreateInfo createInfo( { },                                                   // flags
                                           source.size( ),                                        // codeSize
                                           reinterpret_cast<const uint32_t*>( source.data( ) ) ); // pCode

    auto shaderModule = global::device.createShaderModuleUnique( createInfo );
    VK_CORE_ASSERT( shaderModule, "Failed to create shader module." );

    return std::move( shaderModule );
  }

  inline auto initInstanceUnique( const std::vector<const char*>& layers, std::vector<const char*>& extensions, uint32_t minVersion = VK_API_VERSION_1_0 ) -> vk::UniqueInstance
  {
    vk::DynamicLoader dl;
    auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>( "vkGetInstanceProcAddr" );
    VULKAN_HPP_DEFAULT_DISPATCHER.init( vkGetInstanceProcAddr );

    // Check if all extensions and layers needed are available.
    checkInstanceLayersSupport( layers );
    checkInstanceExtensionsSupport( extensions );

    // Start creating the instance.
    vk::ApplicationInfo appInfo;
    appInfo.apiVersion = assessVulkanVersion( minVersion );

    vk::InstanceCreateInfo createInfo( { },                                         // flags
                                       &appInfo,                                    // pApplicationInfo
                                       static_cast<uint32_t>( layers.size( ) ),     // enabledLayerCount
                                       layers.data( ),                              // ppEnabledLayerNames
                                       static_cast<uint32_t>( extensions.size( ) ), // enabledExtensionCount
                                       extensions.data( ) );                        // ppEnabledExtensionNames

    auto instance    = createInstanceUnique( createInfo );
    global::instance = instance.get( );
    VK_CORE_ASSERT( instance, "Failed to create instance." );
    VK_CORE_LOG( "Create instance: Success" );

    VULKAN_HPP_DEFAULT_DISPATCHER.init( instance.get( ) );
    VK_CORE_LOG( "Init Dispatcher: Instance: Success" );

    return std::move( instance );
  }

  inline auto initDeviceUnique( std::vector<const char*>& extensions, const std::optional<vk::PhysicalDeviceFeatures>& features, const std::optional<vk::PhysicalDeviceFeatures2>& features2 = { } ) -> vk::UniqueDevice
  {
    checkDeviceExtensionSupport( extensions );

    auto queueCreateInfos = getDeviceQueueCreateInfos( );

    vk::DeviceCreateInfo createInfo( { },                                                                                           // flags
                                     static_cast<uint32_t>( queueCreateInfos.size( ) ),                                             // queueCreateInfoCount
                                     queueCreateInfos.data( ),                                                                      // pQueueCreateInfos
                                     0,                                                                                             // enabledLayerCount
                                     nullptr,                                                                                       // ppEnabledLayerNames
                                     static_cast<uint32_t>( extensions.size( ) ),                                                   // enabledExtensionCount
                                     extensions.data( ),                                                                            // ppEnabledExtensionNames
                                     features2.has_value( ) ? nullptr : ( features.has_value( ) ? &features.value( ) : nullptr ) ); // pEnabledFeatures - must be NULL because the VkDeviceCreateInfo::pNext chain includes VkPhysicalDeviceFeatures2.

    createInfo.pNext = features2.has_value( ) ? &features2.value( ) : nullptr;

    auto device    = global::physicalDevice.createDeviceUnique( createInfo );
    global::device = device.get( );
    VK_CORE_ASSERT( device, "Failed to create logical device." );

    VULKAN_HPP_DEFAULT_DISPATCHER.init( device.get( ) );

    return std::move( device );
  }

#endif

  // --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  // Classes
  // --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

  /// A wrapper class for Vulkan command buffer objects.
  class CommandBuffer
  {
  public:
    CommandBuffer( ) = default;

    /// Call to init(vk::CommandPool, uint32_t, vk::CommandBufferUsageFlags).
    CommandBuffer( vk::CommandPool commandPool, uint32_t count = 1, vk::CommandBufferUsageFlags usageFlags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit )
    {
      init( commandPool, count, usageFlags );
    }

    /// Creates the command buffers.
    /// @param commandPool The command pool from which the command buffers will be allocated from.
    /// @param count The amount of Vulkan command buffers to initialize (the same as the amount of images in the swapchain).
    /// @param usageFlags Specifies what the buffer will be used for.
    void init( vk::CommandPool commandPool, uint32_t count = 1, vk::CommandBufferUsageFlags usageFlags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit )
    {
      _commandPool = commandPool;

      _commandBuffers.resize( count );

      vk::CommandBufferAllocateInfo allocateInfo( commandPool,                      // commandPool
                                                  vk::CommandBufferLevel::ePrimary, // level
                                                  count );                          // commandBufferCount

      _commandBuffers = global::device.allocateCommandBuffers( allocateInfo );
      for ( const vk::CommandBuffer& commandBuffer : _commandBuffers )
      {
        VK_CORE_ASSERT( commandBuffer, "Failed to create command buffers." );
      }

      // Set up begin info.
      _beginInfo.flags = usageFlags;
    }

    auto get( ) const -> const std::vector<vk::CommandBuffer> { return _commandBuffers; }

    auto get( size_t index ) const -> const vk::CommandBuffer { return _commandBuffers[index]; }

    void free( )
    {
      global::device.freeCommandBuffers( _commandPool, static_cast<uint32_t>( _commandBuffers.size( ) ), _commandBuffers.data( ) );
    }

    void reset( )
    {
      for ( vk::CommandBuffer& buffer : _commandBuffers )
      {
        buffer.reset( vk::CommandBufferResetFlagBits::eReleaseResources );
      }
    }

    /// Used to begin the command buffer recording.
    /// @param index An index to a command buffer to record to.
    void begin( size_t index = 0 )
    {
      _commandBuffers[index].begin( _beginInfo );
    }

    /// Used to stop the command buffer recording.
    /// @param index An index to a command buffer to stop recording.
    void end( size_t index = 0 )
    {
      _commandBuffers[index].end( );
    }

    /// Submits the recorded commands to a queue.
    /// @param queue The queue to submit to.
    /// @param waitSemaphores A std::vector of semaphores to wait for.
    /// @param signalSemaphores A std::vector of semaphores to signal.
    /// @param waitDstStageMask The pipeline stage where the commands will be executed.
    void submitToQueue( vk::Queue queue, vk::Fence fence = nullptr, const std::vector<vk::Semaphore>& waitSemaphores = { }, const std::vector<vk::Semaphore>& signalSemaphores = { }, vk::PipelineStageFlags* waitDstStageMask = { } )
    {
      if ( _beginInfo.flags & vk::CommandBufferUsageFlagBits::eOneTimeSubmit )
      {
        vk::SubmitInfo submitInfo( static_cast<uint32_t>( waitSemaphores.size( ) ),   // waitSemaphoreCount
                                   waitSemaphores.data( ),                            // pWaitSemaphores
                                   waitDstStageMask,                                  // pWaitDstStageMask
                                   static_cast<uint32_t>( _commandBuffers.size( ) ),  // commandBufferCount
                                   _commandBuffers.data( ),                           // pCommandBuffers
                                   static_cast<uint32_t>( signalSemaphores.size( ) ), // signalSemaphoreCount
                                   signalSemaphores.data( ) );                        // pSignalSemaphores

        if ( queue.submit( 1, &submitInfo, fence ) != vk::Result::eSuccess )
        {
          VK_CORE_THROW( "Failed to submit" );
        }

        queue.waitIdle( );
      }
      else
      {
        VK_CORE_THROW( "Only command buffers with a usage flag containing eOneTimeSubmit should be submitted automatically" );
      }
    }

  private:
    std::vector<vk::CommandBuffer> _commandBuffers;

    vk::CommandPool _commandPool; ///< The command pool used to allocate the command buffer from.
    vk::CommandBufferBeginInfo _beginInfo;
  };

  // @todo move up to functions.
  /// Transitions the image layout of any given image. The function will generate its own command buffer.
  /// @param image The vulkan image for which you want to change the image layout.
  /// @param oldLayout The current image layout of the given vulkan image.
  /// @param newLayout The target image layout.
  inline void transitionImageLayout( vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout )
  {
    auto barrierInfo = getImageMemoryBarrierInfo( image, oldLayout, newLayout );

    CommandBuffer commandBuffer;
    commandBuffer.init( global::graphicsCmdPool );
    commandBuffer.begin( );

    commandBuffer.get( 0 ).pipelineBarrier( std::get<1>( barrierInfo ), // srcStageMask
                                            std::get<2>( barrierInfo ), // dstStageMask
                                            vk::DependencyFlagBits::eByRegion,
                                            0,
                                            nullptr,
                                            0,
                                            nullptr,
                                            1,
                                            &std::get<0>( barrierInfo ) ); // barrier

    commandBuffer.end( );
    commandBuffer.submitToQueue( global::graphicsQueue );
  }

  /// A wrapper class for a Vulkan image.
  /// @ingroup API
  class Image
  {
  public:
    auto get( ) const -> vk::Image { return _image.get( ); }

    auto getExtent( ) const -> vk::Extent3D { return _extent; }

    auto getFormat( ) const -> vk::Format { return _format; }

    auto getLayout( ) const -> vk::ImageLayout { return _layout; }

    /// Creates the image and allocates memory for it.
    /// @param createInfo The Vulkan image create info.
    void init( const vk::ImageCreateInfo& createInfo )
    {
      _extent = createInfo.extent;
      _format = createInfo.format;
      _layout = createInfo.initialLayout;

      _image = global::device.createImageUnique( createInfo );
      VK_CORE_ASSERT( _image.get( ), "Failed to create image" );
      _memory = allocateMemoryUnique( _image.get( ), vk::MemoryPropertyFlagBits::eDeviceLocal );
      global::device.bindImageMemory( _image.get( ), _memory.get( ), 0 );
    }

    /// Used to transition this image's layout.
    /// @param layout The target layout.
    /// @param subresourceRange Optionally used to define a non-standard subresource range.
    /// @note This function creates its own single-time usage command buffer.
    void transitionToLayout( vk::ImageLayout layout, const vk::ImageSubresourceRange* subresourceRange = nullptr )
    {
      auto barrierInfo = getImageMemoryBarrierInfo( _image.get( ), _layout, layout, subresourceRange );

      CommandBuffer commandBuffer;
      commandBuffer.init( global::graphicsCmdPool );
      commandBuffer.begin( );

      commandBuffer.get( 0 ).pipelineBarrier( std::get<1>( barrierInfo ), // srcStageMask
                                              std::get<2>( barrierInfo ), // dstStageMask
                                              vk::DependencyFlagBits::eByRegion,
                                              0,
                                              nullptr,
                                              0,
                                              nullptr,
                                              1,
                                              &std::get<0>( barrierInfo ) ); // barrier

      commandBuffer.end( );
      commandBuffer.submitToQueue( global::graphicsQueue );

      _layout = layout;
    }

    /// Used to transition this image's layout using an already existing command buffer.
    /// @param layout The target layout
    /// @param commandBuffer The command buffer that will be used to set up a pipeline barrier.
    /// @param subresourceRange Optionally used to define a non-standard subresource range.
    void transitionToLayout( vk::ImageLayout layout, vk::CommandBuffer commandBuffer, const vk::ImageSubresourceRange* subresourceRange = nullptr )
    {
      auto barrierInfo = getImageMemoryBarrierInfo( _image.get( ), _layout, layout, subresourceRange );

      commandBuffer.pipelineBarrier( std::get<1>( barrierInfo ), // srcStageMask
                                     std::get<2>( barrierInfo ), // dstStageMask
                                     vk::DependencyFlagBits::eByRegion,
                                     0,
                                     nullptr,
                                     0,
                                     nullptr,
                                     1,
                                     &std::get<0>( barrierInfo ) ); // barrier

      _layout = layout;
    }

  protected:
    vk::UniqueImage _image;
    vk::UniqueDeviceMemory _memory;

    vk::Extent3D _extent;
    vk::Format _format;
    vk::ImageLayout _layout;
  };

  /// A wrapper class for a Vulkan buffer object.
  /// @ingroup API
  class Buffer
  {
  public:
    Buffer( ) = default;

    /// Call to init(k::DeviceSize, vk::BufferUsageFlags, const std::vector<uint32_t>&, vk::MemoryPropertyFlags).
    Buffer( vk::DeviceSize size, vk::BufferUsageFlags usage, const std::vector<uint32_t>& queueFamilyIndices = { }, vk::MemoryPropertyFlags memoryPropertyFlags = vk::MemoryPropertyFlagBits::eDeviceLocal, void* pNextMemory = nullptr )
    {
      init( size, usage, queueFamilyIndices, memoryPropertyFlags, pNextMemory );
    }

    /// If memory was mapped previously, the destructor will automatically unmap it again.
    ~Buffer( )
    {
      if ( _memory && _mapped )
      {
        global::device.unmapMemory( _memory.get( ) );
      }
    }

    /// @param buffer The target for the copy operation.
    Buffer( const Buffer& buffer )
    {
      copyToBuffer( buffer );
    }

    Buffer( const Buffer&& ) = delete;

    /// Call to copyToBuffer(Buffer).
    auto operator=( const Buffer& buffer ) -> Buffer&
    {
      buffer.copyToBuffer( *this );
      return *this;
    }

    auto operator=( const Buffer&& ) -> Buffer& = delete;

    auto get( ) const -> const vk::Buffer { return _buffer.get( ); }

    auto getMemory( ) const -> const vk::DeviceMemory { return _memory.get( ); }

    auto getSize( ) const -> const vk::DeviceSize { return _size; }

    /// Creates the buffer and allocates memory for it.
    /// @param queueFamilyIndices Specifies which queue family will access the buffer.
    /// @param memoryPropertyFlags Flags for memory allocation.
    /// @param pNextMemory Attachment to the memory's pNext chain.
    void init( vk::DeviceSize size, vk::BufferUsageFlags usage, const std::vector<uint32_t>& queueFamilyIndices = { }, vk::MemoryPropertyFlags memoryPropertyFlags = vk::MemoryPropertyFlagBits::eDeviceLocal, void* pNextMemory = nullptr )
    {
        if ( _memory && _mapped )
            global::device.unmapMemory( _memory.get( ) );

      _mapped = false;
      _size   = size;

      vk::SharingMode sharingMode = queueFamilyIndices.size( ) > 1 ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;

      vk::BufferCreateInfo createInfo( { },                                                 // flags
                                       size,                                                // size
                                       usage,                                               // usage
                                       sharingMode,                                         // sharingMode
                                       static_cast<uint32_t>( queueFamilyIndices.size( ) ), // queueFamilyIndexCount
                                       queueFamilyIndices.data( ) );                        // pQueueFamilyIndices

      _buffer = global::device.createBufferUnique( createInfo );
      VK_CORE_ASSERT( _buffer.get( ), "Failed to create buffer." );

      _memory = allocateMemoryUnique( _buffer, memoryPropertyFlags, pNextMemory );
      global::device.bindBufferMemory( _buffer.get( ), _memory.get( ), 0 );
    }

    /// Copies the content of this buffer to another Buffer.
    /// @param buffer The target for the copy operation.
    /// @param fence A fence to wait for when submitting the local single-time-use command buffer to the command queue.
    void copyToBuffer( const Buffer& buffer, vk::Fence fence = nullptr ) const
    {
      copyToBuffer( buffer.get( ), fence );
    }

    /// Copies the content of this buffer to another vk::Buffer.
    /// @param buffer The target for the copy operation.
    /// @param fence A fence to wait for when submitting the local single-time-use command buffer to the command queue.
    void copyToBuffer( vk::Buffer buffer, vk::Fence fence = nullptr ) const
    {
      CommandBuffer commandBuffer( global::transferCmdPool );
      commandBuffer.begin( );
      {
        vk::BufferCopy copyRegion( 0, 0, _size );
        commandBuffer.get( 0 ).copyBuffer( _buffer.get( ), buffer, 1, &copyRegion ); // CMD
      }
      commandBuffer.end( );
      commandBuffer.submitToQueue( global::transferQueue, fence );
    }

    /// Copies the content of this buffer to an image.
    /// @param image The target for the copy operation.
    void copyToImage( const Image& image ) const
    {
      copyToImage( image.get( ), image.getExtent( ) );
    }

    /// Copies the content of this buffer to an image.
    /// @param image The target for the copy operation.
    /// @param extent The target's extent.
    void copyToImage( vk::Image image, vk::Extent3D extent ) const
    {
      CommandBuffer commandBuffer( global::graphicsCmdPool );
      commandBuffer.begin( );
      {
        vk::BufferImageCopy region( 0,                                            // bufferOffset
                                    0,                                            // bufferRowLength
                                    0,                                            // bufferImageHeight
                                    { vk::ImageAspectFlagBits::eColor, 0, 0, 1 }, // imageSubresource (aspectMask, mipLevel, baseArrayLayer, layerCount)
                                    vk::Offset3D { 0, 0, 0 },                     // imageOffset
                                    extent );                                     // imageExtent

        commandBuffer.get( 0 ).copyBufferToImage( _buffer.get( ), image, vk::ImageLayout::eTransferDstOptimal, 1, &region ); // CMD
      }
      commandBuffer.end( );
      commandBuffer.submitToQueue( global::graphicsQueue );
    }

    /// Used to fill the buffer with the content of a given std::vector.
    /// @param data The data to fill the buffer with.
    /// @param offset The data's offset within the buffer.
    template <class T>
    void fill( const std::vector<T>& data, vk::DeviceSize offset = 0, std::optional<vk::DeviceSize> size = { } )
    {
      vk::DeviceSize actualSize = data.size( ) * sizeof( data[0] );

      // Only call mapMemory once or every time the buffer has been initialized again.
      if ( !_mapped )
      {
        _mapped = true;

        if ( global::device.mapMemory( _memory.get( ), offset, actualSize, { }, &_ptrToData ) != vk::Result::eSuccess )
        {
          VK_CORE_THROW( "Failed to map memory." );
        }
      }

      VK_CORE_ASSERT( ( _ptrToData != nullptr ), "Failed to copy data to storage staging buffer." );
      memcpy( _ptrToData, data.data( ), static_cast<uint32_t>( actualSize ) );
    }

    /// Used to fill the buffer by using a pointer and (optionally) passing the underlying memory size.
    /// @param data The data to fill the buffer with.
    /// @param offset The data's offset within the buffer.
    /// @param size An optional size parameter to pass the size of data. If omitted, the size passed when calling init() will be used instead.
    template <class T>
    void fill( const T* data, vk::DeviceSize offset = 0, std::optional<vk::DeviceSize> size = { } )
    {
      vk::DeviceSize finalSize = _size;
      if ( size.has_value( ) )
      {
        finalSize = size.value( );
      }

      // Only call mapMemory once or every time the buffer has been initialized again.
      if ( !_mapped )
      {
        _mapped = true;

        if ( global::device.mapMemory( _memory.get( ), offset, finalSize, { }, &_ptrToData ) != vk::Result::eSuccess )
        {
          VK_CORE_THROW( "Failed to map memory." );
        }
      }

      VK_CORE_ASSERT( ( _ptrToData != nullptr ), "Failed to copy data to storage staging buffer." );
      memcpy( _ptrToData, data, static_cast<uint32_t>( _size ) );
      // @todo unmap memory in destructor
    }

  protected:
    vk::UniqueBuffer _buffer;
    vk::UniqueDeviceMemory _memory;

    vk::DeviceSize _size = 0;

    void* _ptrToData = nullptr;
    bool _mapped     = false;
  };

  /// A specialization class for creating textures using the sbt_image header.
  /// @ingroup API
  class Texture : public Image
  {
  public:
    auto getImageView( ) const -> vk::ImageView { return _imageView.get( ); }

    auto getPath( ) const -> const std::string& { return _path; }

    /// Creates the texture.
    /// @param path The relative path to the texture file.
    void init( std::string_view path )
    {
      _path = path;

      int width;
      int height;
      int channels;
      stbi_uc* pixels = stbi_load( path.data( ), &width, &height, &channels, STBI_rgb_alpha );

      if ( pixels == nullptr )
      {
        VK_CORE_THROW( "Failed to load texture" );
      }

      vk::DeviceSize size = width * height * 4;             // FIXME

      // Set up the staging buffer.
      Buffer stagingBuffer( size,
                            vk::BufferUsageFlagBits::eTransferSrc,
                            { global::graphicsFamilyIndex },
                            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent );

      stagingBuffer.fill<stbi_uc>( pixels );

      stbi_image_free( pixels );

      auto imageCreateInfo = getImageCreateInfo(
              vk::Extent3D { static_cast<uint32_t>( width ), static_cast<uint32_t>( height ), 1 } );
      Image::init( imageCreateInfo );

      transitionToLayout( vk::ImageLayout::eTransferDstOptimal );
      stagingBuffer.copyToImage( _image.get( ), _extent );
      transitionToLayout( vk::ImageLayout::eShaderReadOnlyOptimal );

      _imageView = initImageViewUnique( getImageViewCreateInfo( _image.get( ), _format ) );

      _path = path;

      /*
      ktxTexture* texture = nullptr;

      std::string temp( path );

      if ( ktxTexture_CreateFromNamedFile( temp.c_str( ), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture ) != KTX_SUCCESS )
      {
        VK_CORE_THROW( "Failed to load KTX file from: ", temp );
      }

      ktx_uint8_t* textureData = texture->pData;
      ktx_size_t textureSize   = texture->dataSize;
      uint32_t width           = texture->baseWidth;
      uint32_t height          = texture->baseHeight;
      uint32_t numLevels       = 1;

      // Set up the staging buffer.
      Buffer stagingBuffer( textureSize,
                            vk::BufferUsageFlagBits::eTransferSrc,
                            { global::graphicsFamilyIndex },
                            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent );

      stagingBuffer.fill<ktx_uint8_t>( textureData );

      auto imageCreateInfo = getImageCreateInfo( vk::Extent3D { static_cast<uint32_t>( width ), static_cast<uint32_t>( height ), 1 } );
      Image::init( imageCreateInfo );

      transitionToLayout( vk::ImageLayout::eTransferDstOptimal );
      stagingBuffer.copyToImage( _image.get( ), _extent );
      transitionToLayout( vk::ImageLayout::eShaderReadOnlyOptimal );

      _imageView = initImageViewUnique( getImageViewCreateInfo( _image.get( ), _format ) );

      ktxTexture_Destroy( texture );
      */
    }

  private:
    std::string _path; ///< The relative path to the texture file.

    vk::UniqueImageView _imageView;
  };

  /// A specialization class for creating cubemaps.
  /// @ingroup API
  class Cubemap : public Image
  {
  public:
    auto getImageView( ) const -> const vk::ImageView { return _imageView.get( ); }

    auto getSampler( ) const -> const vk::Sampler { return _sampler.get( ); }

    /// Initializes the cubemap.
    /// @param path A path to a ktx cubemap file. If left empty, an empty cubemap will be created instead.
    void init( std::string_view path = std::string_view( ) )
    {
      ktxTexture* texture = nullptr;

      ktx_uint8_t data         = '0';
      ktx_uint8_t* textureData = &data;
      ktx_size_t textureSize   = 16;
      uint32_t width           = 1;
      uint32_t height          = 1;
      uint32_t numLevels       = 1;

      if ( !path.empty( ) )
      {
        std::string fullPath = std::string( path );

        if ( ktxTexture_CreateFromNamedFile( fullPath.c_str( ), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture ) != KTX_SUCCESS )
        {
          VK_CORE_THROW( "Failed t o load KTX file from: ", fullPath );
        }

        textureData = texture->pData;
        textureSize = texture->dataSize;
        width       = texture->baseWidth;
        height      = texture->baseHeight;
      }

      // Set up the staging buffer.
      Buffer stagingBuffer( textureSize,
                            vk::BufferUsageFlagBits::eTransferSrc,
                            { global::graphicsFamilyIndex },
                            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent );

      vk::MemoryRequirements reqs = global::device.getBufferMemoryRequirements( stagingBuffer.get( ) );

      // Copy to staging buffer
      stagingBuffer.fill<ktx_uint8_t>( textureData );

      // Init image
      vk::Extent3D extent( width, height, 1 );

      auto imageCreateInfo        = getImageCreateInfo( extent );
      imageCreateInfo.arrayLayers = 6;
      imageCreateInfo.flags       = vk::ImageCreateFlagBits::eCubeCompatible;
      imageCreateInfo.mipLevels   = numLevels;
      imageCreateInfo.format      = vk::Format::eR8G8B8A8Srgb; // @todo add format support handling

      Image::init( imageCreateInfo );

      // Image create info
      auto imageViewCreateInfo                        = getImageViewCreateInfo( _image.get( ), _format, vk::ImageViewType::eCube );
      imageViewCreateInfo.subresourceRange.layerCount = 6;
      imageViewCreateInfo.subresourceRange.levelCount = numLevels;
      imageViewCreateInfo.components                  = { vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA };

      // Create buffer copy regions
      std::vector<vk::BufferImageCopy> bufferCopyRegions;
      bufferCopyRegions.reserve( 6 * numLevels );

      for ( uint32_t face = 0; face < 6; ++face )
      {
        for ( uint32_t mipLevel = 0; mipLevel < numLevels; ++mipLevel )
        {
          ktx_size_t offset = 0;
          if ( texture != nullptr )
          {
            VK_CORE_ASSERT( ktxTexture_GetImageOffset( texture, mipLevel, 0, face, &offset ) == KTX_SUCCESS, "KTX: Failed to get image offset." );
          }
          vk::BufferImageCopy bufferCopyRegion             = { };
          bufferCopyRegion.imageSubresource.aspectMask     = vk::ImageAspectFlagBits::eColor;
          bufferCopyRegion.imageSubresource.mipLevel       = mipLevel;
          bufferCopyRegion.imageSubresource.baseArrayLayer = face;
          bufferCopyRegion.imageSubresource.layerCount     = 1;
          bufferCopyRegion.imageExtent.width               = width >> mipLevel;
          bufferCopyRegion.imageExtent.height              = height >> mipLevel;
          bufferCopyRegion.imageExtent.depth               = 1;
          bufferCopyRegion.bufferOffset                    = offset;
          bufferCopyRegions.push_back( bufferCopyRegion );
        }
      }

      // Fill the actual image.
      transitionToLayout( vk::ImageLayout::eTransferDstOptimal, &imageViewCreateInfo.subresourceRange );

      CommandBuffer commandBuffer( global::graphicsCmdPool );
      commandBuffer.begin( );
      {
        commandBuffer.get( 0 ).copyBufferToImage( stagingBuffer.get( ),
                                                  _image.get( ),
                                                  vk::ImageLayout::eTransferDstOptimal,
                                                  static_cast<uint32_t>( bufferCopyRegions.size( ) ),
                                                  bufferCopyRegions.data( ) ); // CMD
      }
      commandBuffer.end( );
      commandBuffer.submitToQueue( global::graphicsQueue );

      transitionToLayout( vk::ImageLayout::eShaderReadOnlyOptimal, &imageViewCreateInfo.subresourceRange );

      // Init sampler
      auto samplerCreateInfo          = getSamplerCreateInfo( );
      samplerCreateInfo.addressModeU  = vk::SamplerAddressMode::eClampToEdge;
      samplerCreateInfo.addressModeV  = vk::SamplerAddressMode::eClampToEdge;
      samplerCreateInfo.addressModeW  = vk::SamplerAddressMode::eClampToEdge;
      samplerCreateInfo.minLod        = 0.0F;
      samplerCreateInfo.maxLod        = static_cast<float>( numLevels );
      samplerCreateInfo.borderColor   = vk::BorderColor::eFloatOpaqueWhite;
      samplerCreateInfo.maxAnisotropy = 1.0F;
      samplerCreateInfo.compareOp     = vk::CompareOp::eNever;

      _sampler = initSamplerUnique( samplerCreateInfo );

      // Init image view
      _imageView = initImageViewUnique( imageViewCreateInfo );

      // Clean up
      if ( texture != nullptr )
      {
        ktxTexture_Destroy( texture );
      }
    }

    /// Initializes the cubemap from a created ktxTexture
    /// It's the callers responsibility to destroy the texture: ktxTexture_Destroy( texture );
    /// @param texture A pointer to the ktx cubemap.
    void init( ktxTexture* texture )
    {
      ktx_uint8_t* textureData = texture->pData;
      ktx_size_t textureSize   = texture->dataSize;
      uint32_t width           = texture->baseWidth;
      uint32_t height          = texture->baseHeight;
      uint32_t numLevels       = 1;


      // Set up the staging buffer.
      Buffer stagingBuffer( textureSize,
                            vk::BufferUsageFlagBits::eTransferSrc,
                            { global::graphicsFamilyIndex },
                            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent );

      vk::MemoryRequirements reqs = global::device.getBufferMemoryRequirements( stagingBuffer.get( ) );

      // Copy to staging buffer
      stagingBuffer.fill<ktx_uint8_t>( textureData );

      // Init image
      vk::Extent3D extent( width, height, 1 );

      auto imageCreateInfo        = getImageCreateInfo( extent );
      imageCreateInfo.arrayLayers = 6;
      imageCreateInfo.flags       = vk::ImageCreateFlagBits::eCubeCompatible;
      imageCreateInfo.mipLevels   = numLevels;
      imageCreateInfo.format      = vk::Format::eR8G8B8A8Srgb; // @todo add format support handling

      Image::init( imageCreateInfo );

      // Image create info
      auto imageViewCreateInfo                        = getImageViewCreateInfo( _image.get( ), _format, vk::ImageViewType::eCube );
      imageViewCreateInfo.subresourceRange.layerCount = 6;
      imageViewCreateInfo.subresourceRange.levelCount = numLevels;
      imageViewCreateInfo.components                  = { vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA };

      // Create buffer copy regions
      std::vector<vk::BufferImageCopy> bufferCopyRegions;
      bufferCopyRegions.reserve( 6 * numLevels );

      for ( uint32_t face = 0; face < 6; ++face )
      {
        for ( uint32_t mipLevel = 0; mipLevel < numLevels; ++mipLevel )
        {
          ktx_size_t offset = 0;
          if ( texture != nullptr )
          {
            VK_CORE_ASSERT( ktxTexture_GetImageOffset( texture, mipLevel, 0, face, &offset ) == KTX_SUCCESS, "KTX: Failed to get image offset." );
          }
          vk::BufferImageCopy bufferCopyRegion             = { };
          bufferCopyRegion.imageSubresource.aspectMask     = vk::ImageAspectFlagBits::eColor;
          bufferCopyRegion.imageSubresource.mipLevel       = mipLevel;
          bufferCopyRegion.imageSubresource.baseArrayLayer = face;
          bufferCopyRegion.imageSubresource.layerCount     = 1;
          bufferCopyRegion.imageExtent.width               = width >> mipLevel;
          bufferCopyRegion.imageExtent.height              = height >> mipLevel;
          bufferCopyRegion.imageExtent.depth               = 1;
          bufferCopyRegion.bufferOffset                    = offset;
          bufferCopyRegions.push_back( bufferCopyRegion );
        }
      }

      // Fill the actual image.
      transitionToLayout( vk::ImageLayout::eTransferDstOptimal, &imageViewCreateInfo.subresourceRange );

      CommandBuffer commandBuffer( global::graphicsCmdPool );
      commandBuffer.begin( );
      {
        commandBuffer.get( 0 ).copyBufferToImage( stagingBuffer.get( ),
                                                  _image.get( ),
                                                  vk::ImageLayout::eTransferDstOptimal,
                                                  static_cast<uint32_t>( bufferCopyRegions.size( ) ),
                                                  bufferCopyRegions.data( ) ); // CMD
      }
      commandBuffer.end( );
      commandBuffer.submitToQueue( global::graphicsQueue );

      transitionToLayout( vk::ImageLayout::eShaderReadOnlyOptimal, &imageViewCreateInfo.subresourceRange );

      // Init sampler
      auto samplerCreateInfo          = getSamplerCreateInfo( );
      samplerCreateInfo.addressModeU  = vk::SamplerAddressMode::eClampToEdge;
      samplerCreateInfo.addressModeV  = vk::SamplerAddressMode::eClampToEdge;
      samplerCreateInfo.addressModeW  = vk::SamplerAddressMode::eClampToEdge;
      samplerCreateInfo.minLod        = 0.0F;
      samplerCreateInfo.maxLod        = static_cast<float>( numLevels );
      samplerCreateInfo.borderColor   = vk::BorderColor::eFloatOpaqueWhite;
      samplerCreateInfo.maxAnisotropy = 1.0F;
      samplerCreateInfo.compareOp     = vk::CompareOp::eNever;

      _sampler = initSamplerUnique( samplerCreateInfo );

      // Init image view
      _imageView = initImageViewUnique( imageViewCreateInfo );
    }

  private:
    vk::UniqueSampler _sampler;
    vk::UniqueImageView _imageView;
  };

  /// A shader storage buffer specilization class.
  /// @ingroup API
  template <class T>
  class StorageBuffer
  {
  public:
    auto get( ) const -> const std::vector<Buffer>& { return _storageBuffers; }

    auto get( size_t index ) const -> const vk::Buffer { return _storageBuffers[index].get( ); }

    auto getCount( ) const -> uint32_t { return _count; }

    auto getDescriptorInfos( ) const -> const std::vector<vk::DescriptorBufferInfo>& { return _bufferInfos; }

    /// Creates a storage buffer and n copies.
    /// @param data The data to fill the storage buffer(s) with.
    /// @param copies The amount of copies to make.
    /// @param deviceAddressVisible If true, the buffer will be device visible.
    void init(
            const std::vector<T>& data, size_t copies = 1,
            bool deviceAddressVisible = false, const std::vector<vk::BufferUsageFlags>& additionalBufferUsageFlags = {})
    {
      _count = static_cast<uint32_t>( data.size( ) );

      _maxSize = sizeof( data[0] ) * data.size( );

      _stagingBuffers.resize( copies );
      _storageBuffers.resize( copies );
      _bufferInfos.resize( copies );
//      _fences.resize( copies );

      vk::MemoryAllocateFlagsInfo* allocateFlags = nullptr;
      vk::MemoryAllocateFlagsInfo temp( vk::MemoryAllocateFlagBitsKHR::eDeviceAddress );

      if ( deviceAddressVisible )
      {
        allocateFlags = &temp;
      }

      for ( size_t i = 0; i < copies; ++i )
      {
        _stagingBuffers[i].init( _maxSize,                                                                             // size
                                 vk::BufferUsageFlagBits::eTransferSrc,                                                // usage
                                 { global::transferFamilyIndex },                                                      // queueFamilyIndices
                                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, // memoryPropertyFlags
                                 allocateFlags );

        vk::BufferUsageFlags bufferUsageFlags = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer;
        if ( deviceAddressVisible )
          bufferUsageFlags |= vk::BufferUsageFlagBits::eShaderDeviceAddress;

        for (auto flag: additionalBufferUsageFlags)
            bufferUsageFlags |= flag;

        _storageBuffers[i].init( _maxSize,                                 // size
                                 bufferUsageFlags,                         // usage
                                 { global::transferFamilyIndex },          // queueFamilyIndices
                                 vk::MemoryPropertyFlagBits::eDeviceLocal, // memoryPropertyFlags
                                 allocateFlags );

        _bufferInfos[i].buffer = _storageBuffers[i].get( );
        _bufferInfos[i].offset = 0;
        _bufferInfos[i].range  = VK_WHOLE_SIZE;

//        _fences[i] = initFenceUnique( vk::FenceCreateFlagBits::eSignaled );
      }

      upload( data );
    }

    /// Uploads data to the buffer.
    ///
    /// First, the data is being copied to the staging buffer which is visible to the host.
    /// Finally, the staging buffer is copied to the actual buffer on the device.
    /// @param index Optionally used in case the data should only be uploaded to a specific buffer.
    void upload( const std::vector<T>& data, std::optional<uint32_t> index = { } )
    {
      VK_CORE_ASSERT( _maxSize >= sizeof( data[0] ) * data.size( ), "Exceeded maximum storage buffer size." );

      if ( !index.has_value( ) )
      {
        for ( size_t i = 0; i < _storageBuffers.size( ); ++i )
        {
          _stagingBuffers[i].fill<T>( data );
          _stagingBuffers[i].copyToBuffer( _storageBuffers[i].get( ) );
        }
      }
      else
      {
        _stagingBuffers[index.value( )].fill<T>( data );
        _stagingBuffers[index.value( )].copyToBuffer( _storageBuffers[index.value( )].get( ) );
      }
    }

  private:
    std::vector<Buffer> _stagingBuffers; ///< Holds the staging buffer and all its copies.
    std::vector<Buffer> _storageBuffers; ///< Holds the storage buffer and all its copies.

    std::vector<vk::DescriptorBufferInfo> _bufferInfos;
//    std::vector<vk::UniqueFence> _fences;

    vk::DeviceSize _maxSize = 0;
    uint32_t _count         = 0;
  };

  /// A uniform buffer specialization class.
  /// @ingroup API
  template <class T>
  class UniformBuffer
  {
  public:
    UniformBuffer( ) = default;

    auto get( ) const -> const std::vector<Buffer>& { return _buffers; }

    /// Creates the uniform buffer and allocates memory for it.
    ///
    /// The function will create as many uniform buffers as there are images in the swapchain.
    /// Additionally, it will create the descriptor buffer infos which can be later used to write to a descriptor set.
    void init( )
    {
      _buffers.resize( global::dataCopies );

      for ( Buffer& buffer : _buffers )
      {
        buffer.init( sizeof( T ),
                     vk::BufferUsageFlagBits::eUniformBuffer,
                     { },
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent );
      }

      _bufferInfos.resize( global::dataCopies );

      for ( size_t i = 0; i < _buffers.size( ); ++i )
      {
        _bufferInfos[i] = vk::DescriptorBufferInfo( _buffers[i].get( ), 0, sizeof( T ) );
      }
    }

    /// Used to fill an image's buffer.
    /// @param imageIndex The image's index in the swapchain.
    /// @param ubo The actual uniform buffer object holding the data.
    void upload( uint32_t imageIndex, T& ubo )
    {
      _buffers[imageIndex].fill<T>( &ubo );
    }

    std::vector<vk::DescriptorBufferInfo> _bufferInfos;

  private:
    std::vector<Buffer> _buffers; ///< Holds the uniform buffer and all its copies.
  };

  /// A utility class for managing descriptor related resources.
  /// @ingroup API
  class Bindings
  {
  public:
    /// Used to add a binding.
    /// @param binding The binding's index.
    /// @param type The binding's descriptor type.
    /// @param stage The binding's shader type.
    /// @param count The amount of descriptors for this binding.
    /// @param flags The binding's descriptor binding flags.
    void add( uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stage, uint32_t count = 1, vk::DescriptorBindingFlags flags = { }, vk::Sampler* pImmutableSamplers = nullptr )
    {
      vk::DescriptorSetLayoutBinding temp( binding,              // binding
                                           type,                 // descriptorType
                                           count,                // descriptorCount
                                           stage,                // stageFlags
                                           pImmutableSamplers ); // pImmutableSamplers

      _bindings.push_back( temp );
      _flags.push_back( flags );

      // Resources are created per swapchain image. This operation will only be executed once.
      if ( _writes.size( ) != global::dataCopies )
      {
        _writes.resize( global::dataCopies );
      }

      // For every new binding that is added, the size of each write in writes will get increased by one.
      for ( auto& write : _writes )
      {
        write.resize( write.size( ) + 1 );
      }
    }

    /// Used to initialize a unique descriptor set layout.
    /// @return Returns a descriptor set layout with a unqiue handle.
    auto initLayoutUnique( vk::DescriptorSetLayoutCreateFlags flags = { } ) -> vk::UniqueDescriptorSetLayout
    {
      auto bindingCount = static_cast<uint32_t>( _bindings.size( ) );

      vk::DescriptorSetLayoutCreateInfo createInfo( flags,               // flags
                                                    bindingCount,        // bindingCount
                                                    _bindings.data( ) ); // pBindings

      vk::DescriptorSetLayoutBindingFlagsCreateInfoEXT layoutFlags( bindingCount,     // bindingCount
                                                                    _flags.data( ) ); // pBindingFlags
      createInfo.pNext = &layoutFlags;

      auto layout = global::device.createDescriptorSetLayoutUnique( createInfo );
      VK_CORE_ASSERT( layout.get( ), "Failed to create descriptor set layout." );

      return std::move( layout );
    }

    /// Used to initialize a descriptor pool.
    /// @param maxSets The maximum amount of descriptor sets that can be allocated from the pool.
    /// @param flags The pool's create flags.
    /// @return Returns a descriptor pool with a unique handle.
    auto initPoolUnique( uint32_t maxSets, vk::DescriptorPoolCreateFlags flags = { } ) -> vk::UniqueDescriptorPool
    {
      std::vector<vk::DescriptorPoolSize> tPoolSizes;

      if ( _poolSizes.has_value( ) )
      {
        tPoolSizes = _poolSizes.value( );
      }
      else
      {
        tPoolSizes = getPoolSizes( _bindings, maxSets );
      }

      for ( auto flag : _flags )
      {
        if ( flag == vk::DescriptorBindingFlagBits::eUpdateAfterBind )
        {
          flags |= vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
          break;
        }
      }

      vk::DescriptorPoolCreateInfo createInfo( flags,                                       // flags
                                               maxSets,                                     // maxSets
                                               static_cast<uint32_t>( tPoolSizes.size( ) ), // poolSizeCount
                                               tPoolSizes.data( ) );                        // pPoolSizes

      auto pool = global::device.createDescriptorPoolUnique( createInfo );
      VK_CORE_ASSERT( pool.get( ), "Failed to create descriptor pool." );

      return std::move( pool );
    }

    /// Updates the descriptor set.
    /// @note There are no descriptor set handles required for this function.
    void update( )
    {
      for ( const auto& write : _writes )
      {
        global::device.updateDescriptorSets( static_cast<uint32_t>( write.size( ) ), write.data( ), 0, nullptr );
      }
    }

    /// Used to create a descriptor write for an acceleration structure.
    /// @param sets The descriptor set handles.
    /// @param binding The binding's index.
    /// @param pWriteDescriptorSetAccelerationStructureKHR A pointer to a acceleration structure descriptor write.
    void write( const std::vector<vk::DescriptorSet>& sets, uint32_t binding, const vk::WriteDescriptorSetAccelerationStructureKHR* pWriteDescriptorSetAccelerationStructureKHR )
    {
      for ( size_t i = 0; i < sets.size( ); ++i )
      {
        size_t j            = write( sets[i], i, binding );
        _writes[i][j].pNext = pWriteDescriptorSetAccelerationStructureKHR;
      }
    }

    /// Used to create a descriptor write for an image.
    /// @param sets The descriptor set handles.
    /// @param binding The binding's index.
    /// @param pImageInfo A pointer to an image descriptor write.
    void write( const std::vector<vk::DescriptorSet>& sets, uint32_t binding, const vk::DescriptorImageInfo* pImageInfo )
    {
      for ( size_t i = 0; i < sets.size( ); ++i )
      {
        size_t j                 = write( sets[i], i, binding );
        _writes[i][j].pImageInfo = pImageInfo;
      }
    }

    /// Used to create a descriptor write for a buffer.
    /// @param sets The descriptor set handles.
    /// @param binding The binding's index.
    /// @param pBufferInfo A pointer to a buffer descriptor write.
    void write( const std::vector<vk::DescriptorSet>& sets, uint32_t binding, const vk::DescriptorBufferInfo* pBufferInfo )
    {
      for ( size_t i = 0; i < sets.size( ); ++i )
      {
        size_t j                  = write( sets[i], i, binding );
        _writes[i][j].pBufferInfo = pBufferInfo;
      }
    }

    /// Used to create a descriptor write for uniform buffers.
    /// @param sets The descriptor set handles.
    /// @param binding The binding's index.
    /// @param uniformBufferInfos The uniform buffer's descriptor buffer infos.
    void write( const std::vector<vk::DescriptorSet>& sets, uint32_t binding, const std::vector<vk::DescriptorBufferInfo>& uniformBufferInfos )
    {
      VK_CORE_ASSERT( uniformBufferInfos.size( ) == sets.size( ), "Failed to write descriptor for uniform buffer because there are not as many descriptor sets as uniform buffers." );

      for ( size_t i = 0; i < sets.size( ); ++i )
      {
        size_t j                  = write( sets[i], i, binding );
        _writes[i][j].pBufferInfo = &uniformBufferInfos[i];
      }
    }

    /// Used to create an array of descriptor writes for buffers.
    /// @param sets The descriptor set handles.
    /// @param binding The binding's index.
    /// @param pBufferInfo The pointer to the first element of an array of descriptor buffer infos.
    void writeArray( const std::vector<vk::DescriptorSet>& sets, uint32_t binding, const vk::DescriptorBufferInfo* pBufferInfo )
    {
      for ( size_t i = 0; i < sets.size( ); ++i )
      {
        size_t j                  = writeArray( sets[i], i, binding );
        _writes[i][j].pBufferInfo = pBufferInfo;
      }
    }

    /// Used to create an array of descriptor writes for images.
    /// @param sets The descriptor set handles.
    /// @param binding The binding's index.
    /// @param pImageInfo The pointer to the first element of an array of descriptor image infos.
    void writeArray( const std::vector<vk::DescriptorSet>& sets, uint32_t binding, const vk::DescriptorImageInfo* pImageInfo )
    {
      for ( size_t i = 0; i < sets.size( ); ++i )
      {
        size_t j                 = writeArray( sets[i], i, binding );
        _writes[i][j].pImageInfo = pImageInfo;
      }
    }

    /// Used to set pool sizes manually.
    /// @param poolSizes The pool sizes to set.
    /// @note Must be set before initializing the descriptor pool.
    void setPoolSizes( const std::vector<vk::DescriptorPoolSize>& poolSizes )
    {
      _poolSizes = poolSizes;
    }

    /// Resets all members.
    void reset( )
    {
      _bindings.clear( );
      _flags.clear( );
      _flags.clear( );
      _poolSizes.reset( );
      _writes.clear( );
    }

  private:
    /// Used to create a descriptor write.
    /// @param set The descriptor set to bind to.
    /// @param writeIndex The write index of the descriptor set.
    /// @param binding The binding's index.
    /// @return Returns the index of the matching binding.
    auto write( vk::DescriptorSet set, size_t writeIndex, uint32_t binding ) -> size_t
    {
      vk::WriteDescriptorSet result;

      for ( size_t i = 0; i < _bindings.size( ); ++i )
      {
        if ( _bindings[i].binding == binding )
        {
          result.descriptorCount = 1;
          result.descriptorType  = _bindings[i].descriptorType;
          result.dstBinding      = binding;
          result.dstSet          = set;

          _writes[writeIndex][i] = result;
          return i;
        }
      }

      VK_CORE_THROW( "Failed to write binding to set. Binding could not be found." );
      return 0;
    }

    /// Used to create an array of descriptor writes.
    /// @param set The descriptor set to bind to.
    /// @param writeIndex The write index of the descriptor set.
    /// @param binding The binding's index.
    /// @return Returns the index of the matching binding.
    auto writeArray( vk::DescriptorSet set, size_t writeIndex, uint32_t binding ) -> size_t
    {
      vk::WriteDescriptorSet result;

      for ( size_t i = 0; i < _bindings.size( ); ++i )
      {
        if ( _bindings[i].binding == binding )
        {
          result.descriptorCount = _bindings[i].descriptorCount;
          result.descriptorType  = _bindings[i].descriptorType;
          result.dstBinding      = binding;
          result.dstSet          = set;
          result.dstArrayElement = 0;

          _writes[writeIndex][i] = result;
          return i;
        }
      }

      VK_CORE_THROW( "Failed to write binding to set. Binding could not be found." );
      return 0;
    }

    std::vector<vk::DescriptorSetLayoutBinding> _bindings;         ///< Contains the actual binding.
    std::vector<vk::DescriptorBindingFlags> _flags;                ///< Contains binding flags for each of the actual bindings.
    std::optional<std::vector<vk::DescriptorPoolSize>> _poolSizes; ///< The pool sizes for allocating the descriptor pool (Only used if setPoolSizes(const std::vector<vk::DescriptorPoolSize>&) is called).
    std::vector<std::vector<vk::WriteDescriptorSet>> _writes;      ///< Contains all descriptor writes for each binding.
  };

  /// Encapsulates descriptor-related resources.
  /// @ingroup API
  struct Descriptors
  {
    vk::UniqueDescriptorSetLayout layout;
    vk::UniqueDescriptorPool pool;
    Bindings bindings;
  };

  /// A wrapper class for a Vulkan render pass.
  /// @ingroup API
  class RenderPass
  {
  public:
    /// @return Returns the Vulkan render pass without the unique handle.
    auto get( ) const -> vk::RenderPass { return _renderPass.get( ); }

    /// Initializes the Vulkan render pass.
    /// @param attachments The Vulkan attachment description.
    /// @param subpasses The Vulkan subpass description.
    /// @param dependencies The Vulkan subpass dependencies.
    void init( const std::vector<vk::AttachmentDescription>& attachments, const std::vector<vk::SubpassDescription>& subpasses, const std::vector<vk::SubpassDependency>& dependencies )
    {
      vk::RenderPassCreateInfo createInfo( { },                                           // flags
                                           static_cast<uint32_t>( attachments.size( ) ),  // attachmentCount
                                           attachments.data( ),                           // pAttachments
                                           static_cast<uint32_t>( subpasses.size( ) ),    // subpassCount
                                           subpasses.data( ),                             // pSubpasses
                                           static_cast<uint32_t>( dependencies.size( ) ), // dependencyCount
                                           dependencies.data( ) );                        // pDependencies

      _renderPass = global::device.createRenderPassUnique( createInfo );
      VK_CORE_ASSERT( _renderPass, "Failed to create render pass." );
    }

    /// Call to begin the render pass.
    /// @param framebuffer The swapchain framebuffer.
    /// @param commandBuffer The command buffer used to begin the render pass.
    /// @param renderArea Defines the size of the render area.
    /// @param clearValues The clear values.
    /// @note CommandBuffer::begin() or vk::CommandBuffer::begin() must have been already called prior to calling this function.
    void begin( vk::Framebuffer framebuffer, vk::CommandBuffer commandBuffer, vk::Rect2D renderArea, const std::vector<vk::ClearValue>& clearValues ) const
    {
      vk::RenderPassBeginInfo beginInfo( _renderPass.get( ),                           // renderPass
                                         framebuffer,                                  // framebuffer
                                         renderArea,                                   // renderArea
                                         static_cast<uint32_t>( clearValues.size( ) ), // clearValueCount
                                         clearValues.data( ) );                        // pClearValues

      commandBuffer.beginRenderPass( beginInfo, vk::SubpassContents::eInline );
    }

    /// Call to end the render pass.
    /// @param commandBuffer
    void end( vk::CommandBuffer commandBuffer ) const
    {
      commandBuffer.endRenderPass( );
    }

  private:
    vk::UniqueRenderPass _renderPass; ///< The Vulkan render pass with a unique handle.
  };

  /// A wrapper class for a Vulkan surface.
  /// @ingroup API
  class Surface
  {
  public:
    Surface( ) = default;

    ~Surface( )
    {
      destroy( );
    }

    Surface( const Surface& )  = delete;
    Surface( const Surface&& ) = delete;

    auto operator=( const Surface& ) -> Surface& = delete;
    auto operator=( const Surface&& ) -> Surface& = delete;

    /// @return Returns the surface format.
    auto getFormat( ) const -> vk::Format { return _format; }

    /// @return Returns the surface's color space.
    auto getColorSpace( ) const -> vk::ColorSpaceKHR { return _colorSpace; }

    /// @return Returns the surface's present mode.
    auto getPresentMode( ) const -> vk::PresentModeKHR { return _presentMode; }

    /// @return Returns the surface's capabilities.
    auto getCapabilities( ) const -> vk::SurfaceCapabilitiesKHR { return _capabilities; }

    /// @return Returns the surface's extent.
    auto getExtent( ) const -> vk::Extent2D { return _extent; }

    /// @return Update the surface's extent. Note: this function will not update the surface itself.
    void setExtent( vk::Extent2D ex ) { _extent = ex; }

    /// @note If any of the specified format, color space and present mode are not available the function will fall back to settings that are guaranteed to be supported.
    void init( vk::SurfaceKHR surface, vk::Extent2D extent )
    {
      _surface        = surface;
      global::surface = _surface;
      VK_CORE_ASSERT( _surface, "Invalid surface handle. Create the surface before calling this function." );

      _extent = extent;
    }

    /// Checks if the preferred settings for format, color space and present mode are available. If not, the function will set them to some fallback values.
    /// @warning Must be called right after the enumeration of the physical device.
    void assessSettings( )
    {
      // Get all surface capabilities.
      _capabilities = global::physicalDevice.getSurfaceCapabilitiesKHR( _surface );

      // Check a present mode.
      std::vector<vk::PresentModeKHR> presentModes = global::physicalDevice.getSurfacePresentModesKHR( _surface );

      if ( !details::find<vk::PresentModeKHR>( _presentMode, presentModes ) )
      {
        if ( details::find<vk::PresentModeKHR>( vk::PresentModeKHR::eMailbox, presentModes ) )
        {
          _presentMode = vk::PresentModeKHR::eMailbox;
        }
        else if ( details::find<vk::PresentModeKHR>( vk::PresentModeKHR::eImmediate, presentModes ) )
        {
          _presentMode = vk::PresentModeKHR::eImmediate;
        }
        else if ( details::find<vk::PresentModeKHR>( vk::PresentModeKHR::eFifoRelaxed, presentModes ) )
        {
          _presentMode = vk::PresentModeKHR::eFifoRelaxed;
        }
        else
        {
          // Fall back, as FIFO is always supported on every device.
          VK_CORE_LOG( "Preferred present mode not available. Falling back to ", vk::to_string( _presentMode ), " present mode." );

          _presentMode = vk::PresentModeKHR::eFifo;
        }
      }

      // Check format and color space.
      auto surfaceFormats = global::physicalDevice.getSurfaceFormatsKHR( _surface );

      bool colorSpaceAndFormatSupported = false;
      for ( const auto& iter : surfaceFormats )
      {
        if ( iter.format == _format && iter.colorSpace == _colorSpace )
        {
          colorSpaceAndFormatSupported = true;
          break;
        }
      }

      // If the prefered format and color space are not available, fall back.
      if ( !colorSpaceAndFormatSupported )
      {
        _format     = surfaceFormats[0].format;
        _colorSpace = surfaceFormats[0].colorSpace;
        VK_CORE_LOG( "Preferred format and colorspace not supported. Falling back to the first option of each." );
      }
    }

  private:
    /// Destroys the surface.
    void destroy( )
    {
      if ( _surface )
      {
        global::instance.destroySurfaceKHR( _surface );
        _surface = nullptr;
      }
    }

    vk::SurfaceKHR _surface                  = nullptr;                           ///< The Vulkan surface.
    vk::Format _format                       = vk::Format::eB8G8R8A8Srgb;        ///< The desired surface format.
    vk::ColorSpaceKHR _colorSpace            = vk::ColorSpaceKHR::eSrgbNonlinear; ///< The desired color space.
    vk::PresentModeKHR _presentMode          = vk::PresentModeKHR::eMailbox;      ///< The desired present mode.
    vk::SurfaceCapabilitiesKHR _capabilities = 0;                                 ///< The surface's capabilities.
    vk::Extent2D _extent                     = { };                               ///< The surface's extent.
  };

  /// A wrapper class for a Vulkan swapchain.
  /// @ingroup API
  class Swapchain
  {
  public:
    /// Creates the swapchain, the swapchain images and their image views as well as their framebuffers.
    /// @param surface A pointer to a Surface object.
    /// @param renderPass The render pass to create the framebuffers.
    void init( Surface* surface, vk::RenderPass renderPass )
    {
      surface->assessSettings( );
      VK_CORE_LOG( "Present mode: ", vk::to_string( surface->getPresentMode( ) ) );

      auto surfaceCapabilities = surface->getCapabilities( );

      vk::SwapchainCreateInfoKHR createInfo;
      createInfo.surface = global::surface;

      // Add another image so that the application does not have to wait for the driver before another image can be acquired.
      uint32_t minImageCount = surfaceCapabilities.minImageCount + 1;

      if ( surfaceCapabilities.maxImageCount == 0 )
      {
        VK_CORE_THROW( "The surface does not support any images for a swap chain." );
      }

      // If the preferred image count is exceeding the supported amount then use the maximum amount of images supported by the surface.
      if ( minImageCount > surfaceCapabilities.maxImageCount )
      {
        minImageCount = surfaceCapabilities.maxImageCount;
      }

      createInfo.minImageCount   = minImageCount;
      createInfo.imageFormat     = surface->getFormat( );
      createInfo.imageColorSpace = surface->getColorSpace( );
      createInfo.preTransform    = surfaceCapabilities.currentTransform;

      // Prefer opaque bit over any other composite alpha value.
      createInfo.compositeAlpha = surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eOpaque         ? vk::CompositeAlphaFlagBitsKHR::eOpaque :
                                  surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied  ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied :
                                  surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied :
                                                                                                                                 vk::CompositeAlphaFlagBitsKHR::eInherit;

      // Handle the swap chain image extent.
      if ( surfaceCapabilities.currentExtent.width != UINT32_MAX )
      {
        // The surface size will be determined by the extent of a swapchain targeting the surface.
        _extent = surfaceCapabilities.currentExtent;
      }
      else
      {
        // Clamp width and height.
        _extent = surface->getExtent( );

        _extent.width  = std::clamp( _extent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width );
        _extent.height = std::clamp( _extent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height );
      }

      createInfo.imageExtent = _extent;

      if ( surfaceCapabilities.maxImageArrayLayers < 1 )
      {
        VK_CORE_THROW( "The surface does not support a single array layer." );
      }

      createInfo.imageArrayLayers = 1;
      createInfo.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment
                                  | vk::ImageUsageFlagBits::eTransferDst
                                  | vk::ImageUsageFlagBits::eTransferSrc;

      std::vector<uint32_t> queueFamilyIndices = { global::graphicsFamilyIndex };

      if ( queueFamilyIndices.size( ) > 1 )
      {
        createInfo.imageSharingMode      = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = static_cast<uint32_t>( queueFamilyIndices.size( ) );
        createInfo.pQueueFamilyIndices   = queueFamilyIndices.data( );
      }
      else
      {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
      }

      createInfo.presentMode = surface->getPresentMode( );

      _swapchain = global::device.createSwapchainKHRUnique( createInfo );
      VK_CORE_ASSERT( _swapchain.get( ), "Failed to create swapchain" );
      global::swapchain = _swapchain.get( );

      initImages( minImageCount, surface->getFormat( ) );
      initDepthImage( ); // not necessary for the path tracer (refactoring needed)
      initFramebuffers( renderPass );
    }

    /// Destroys the swapchain.
    void destroy( )
    {
      if ( _swapchain )
      {
        global::device.destroySwapchainKHR( _swapchain.get( ) );
        _swapchain.get( ) = nullptr;
      }
    }

    /// @return Returns the swapchain framebuffer at a given index.
    auto getFramebuffer( uint32_t index ) const -> const vk::Framebuffer& { return _framebuffers[index].get( ); }

    /// @return Returns the current swapchain image index.
    auto getCurrentImageIndex( ) const -> uint32_t { return _currentImageIndex; }

    /// @return Returns the swapchain images' extent.
    auto getExtent( ) const -> vk::Extent2D { return _extent; }

    /// @return Returns the swapchain images' image aspect.
    auto getImageAspect( ) const -> vk::ImageAspectFlags { return _imageAspect; }

    /// Returns the swapchain image at a given index.
    /// @param index The index of the swapchain image.
    /// @return The swapchain image.
    auto getImage( size_t index ) const -> vk::Image { return _images[index]; }

    /// @return Returns a vector containing all swapchain images.
    auto getImages( ) const -> const std::vector<vk::Image>& { return _images; }

    /// @return Returns a vector containing all swapchain image views.
    /// @todo Returning by reference will result in size 0.
    auto getImageViews( ) const -> std::vector<vk::ImageView> { return details::unpack<vk::ImageView>( _imageViews ); }

    /// Used to set the desired image aspect flags.
    void setImageAspect( vk::ImageAspectFlags flags )
    {
      _imageAspect = flags;
    }

    /// Used to transition from one layout to another.
    /// @param oldLayout The swapchain images' current image layout.
    /// @param newLayout The target image layout.
    void setImageLayout(size_t idx, vk::ImageLayout oldLayout, vk::ImageLayout newLayout )
    {
      transitionImageLayout( _images[idx], oldLayout, newLayout );
    }

    void setImageLayout(vk::ImageLayout oldLayout, vk::ImageLayout newLayout )
    {
      for (const auto& image : _images)
          transitionImageLayout( image, oldLayout, newLayout );
    }

    /// Retrieves the next swapchain image.
    /// @param semaphore A semaphore to signal.
    /// @param fence A fence to signal.
    void acquireNextImage( vk::Semaphore semaphore, vk::Fence fence )
    {
      vk::Result result = global::device.acquireNextImageKHR( _swapchain.get( ), UINT64_MAX, semaphore, fence, &_currentImageIndex );
      VK_CORE_ASSERT( ( result == vk::Result::eSuccess ), "Failed to acquire next swapchain image." );
    }

  private:
    /// Initializes the swapchain images.
    /// @param minImageCount The minimum amount of images in the swapchain.
    /// @param surfaceFormat The surface's format.
    void initImages( uint32_t minImageCount, vk::Format surfaceFormat )
    {
      // Retrieve the actual swapchain images. This sets them up automatically.
      _images                     = global::device.getSwapchainImagesKHR( _swapchain.get( ) );
      global::swapchainImageCount = static_cast<uint32_t>( _images.size( ) );

      if ( _images.size( ) < minImageCount )
      {
        VK_CORE_THROW( "Failed to get swapchain images." );
      }

      // Create image views for swapchain images.
      _imageViews.resize( _images.size( ) );
      for ( size_t i = 0; i < _imageViews.size( ); ++i )
      {
        _imageViews[i] = vkCore::initImageViewUnique( getImageViewCreateInfo( _images[i], surfaceFormat, vk::ImageViewType::e2D, _imageAspect ) );
      }
    }

    /// Initializes the depth image for the depth attachment.
    void initDepthImage( )
    {
      // Depth image for depth buffering
      vk::Format depthFormat = getSupportedDepthFormat( global::physicalDevice );

      auto imageCreateInfo   = getImageCreateInfo( vk::Extent3D( _extent.width, _extent.height, 1 ) );
      imageCreateInfo.format = depthFormat;
      imageCreateInfo.usage  = vk::ImageUsageFlagBits::eDepthStencilAttachment;

      _depthImage.init( imageCreateInfo );

      // Image view for depth image
      _depthImageView = vkCore::initImageViewUnique( getImageViewCreateInfo( _depthImage.get( ), depthFormat, vk::ImageViewType::e2D, vk::ImageAspectFlagBits::eDepth ) );
    }

    /// Initializes the swapchain framebuffers.
    /// @param renderPass The render pass to create the framebuffers.
    void initFramebuffers( vk::RenderPass renderPass )
    {
      _framebuffers.resize( _imageViews.size( ) );
      for ( size_t i = 0; i < _framebuffers.size( ); ++i )
      {
        _framebuffers[i] = vkCore::initFramebufferUnique( { _imageViews[i].get( ) }, renderPass, _extent );
      }
    }

    vk::UniqueSwapchainKHR _swapchain; ///< The Vulkan swapchain object with a unique handle.

    vk::Extent2D _extent;                                                ///< The swapchain images' extent.
    vk::ImageAspectFlags _imageAspect = vk::ImageAspectFlagBits::eColor; ///< The swapchain image's image aspect.

    std::vector<vk::Image> _images;               ///< The swapchain images.
    std::vector<vk::UniqueImageView> _imageViews; ///< The swapchain images' image views with a unique handle.

    std::vector<vk::UniqueFramebuffer> _framebuffers; ///< The swapchain images' framebuffers with a unique handle.

    vkCore::Image _depthImage;           ///< The depth image.
    vk::UniqueImageView _depthImageView; ///< The depth image's image views with a unique handle.

    uint32_t _currentImageIndex = 0; ///< The current swapchain image index.
  };

  /// A wrapper class for a Vulkan debug utility messenger.
  ///
  /// The class features scope-bound destruction.
  /// @ingroup API
  class DebugMessenger
  {
  public:
    DebugMessenger( ) = default;

    ~DebugMessenger( )
    {
      if ( _debugMessenger )
      {
        destroy( );
      }
    }

    DebugMessenger( const DebugMessenger& )  = delete;
    DebugMessenger( const DebugMessenger&& ) = delete;

    auto operator=( const DebugMessenger& ) -> DebugMessenger& = delete;
    auto operator=( const DebugMessenger&& ) -> DebugMessenger& = delete;

    /// Creates the debug messenger with the given properties.
    /// @param messageSeverity - Specifies the type of severity of messages that will be logged.
    /// @param messageType - Specifies the types of messages that will be logged.
    void init( vk::DebugUtilsMessageSeverityFlagsEXT messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError, vk::DebugUtilsMessageTypeFlagsEXT messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation )
    {
      vk::DebugUtilsMessengerCreateInfoEXT createInfo( { },
                                                       messageSeverity,
                                                       messageType,
                                                       debugMessengerCallback,
                                                       nullptr );

      _debugMessenger = global::instance.createDebugUtilsMessengerEXT( createInfo );
      VK_CORE_ASSERT( _debugMessenger, "Failed to create debug messenger." );
    }

  private:
    /// Destroys the debug messenger.
    void destroy( )
    {
      if ( _debugMessenger )
      {
        global::instance.destroyDebugUtilsMessengerEXT( _debugMessenger );
        _debugMessenger = nullptr;
      }
    }

    vk::DebugUtilsMessengerEXT _debugMessenger;
  };

  class Sync
  {
  public:
    auto getMaxFramesInFlight( ) -> size_t { return _maxFramesInFlight; }

    auto getImageInFlight( size_t imageIndex ) -> vk::Fence { return _imagesInFlight[imageIndex]; }

    auto getInFlightFence( size_t imageIndex ) -> vk::Fence { return _inFlightFences[imageIndex].get( ); }

    auto getImageAvailableSemaphore( size_t imageIndex ) -> vk::Semaphore { return _imageAvailableSemaphores[imageIndex].get( ); }

    auto getFinishedRenderSemaphore( size_t imageIndex ) -> vk::Semaphore { return _finishedRenderSemaphores[imageIndex].get( ); }

    void init( size_t n = 2 )
    {
      if (_initialized)
        return;

      _maxFramesInFlight = n;
      _imageAvailableSemaphores.resize( _maxFramesInFlight );
      _finishedRenderSemaphores.resize( _maxFramesInFlight );
      _inFlightFences.resize( _maxFramesInFlight );
      _imagesInFlight.resize( global::swapchainImageCount, nullptr );

      for ( size_t i = 0; i < _maxFramesInFlight; ++i )
      {
        _imageAvailableSemaphores[i] = vkCore::initSemaphoreUnique( );
        _finishedRenderSemaphores[i] = vkCore::initSemaphoreUnique( );
        _inFlightFences[i]           = vkCore::initFenceUnique( vk::FenceCreateFlagBits::eSignaled );
      }

      _initialized = true;
    }

    void waitForFrame( size_t frame )
    {
      vk::Result result = global::device.waitForFences( 1, &_inFlightFences[frame].get( ), VK_TRUE, UINT64_MAX );
      VK_CORE_ASSERT( ( result == vk::Result::eSuccess ), "Failed to wait for fences." );
    }

  private:
    std::vector<vk::Fence> _imagesInFlight;
    std::vector<vk::UniqueFence> _inFlightFences;
    std::vector<vk::UniqueSemaphore> _imageAvailableSemaphores;
    std::vector<vk::UniqueSemaphore> _finishedRenderSemaphores;

    size_t _maxFramesInFlight = 2;
    bool _initialized = false;
  };




    // Additional Utilities by Jet

    ///
    /// @param
    inline void download(vk::Image _image, vk::Format _format, vk::ImageLayout _layout, vk::Extent3D _extent,
                  void *data, size_t size, vk::Offset3D offset, vk::Extent3D extent,
                  const std::vector<vk::Semaphore>& waitSemaphores = { }) {
        vk::ImageAspectFlags aspect;
        uint32_t formatSize;

        switch (_format) {
            case vk::Format::eR8G8B8A8Unorm:
            case vk::Format::eB8G8R8A8Unorm:
            case vk::Format::eB8G8R8A8Srgb:
                formatSize = 4;
                aspect = vk::ImageAspectFlagBits::eColor;
                break;
            case vk::Format::eR32G32B32A32Uint:
                formatSize = 16;
                aspect = vk::ImageAspectFlagBits::eColor;
                break;
            case vk::Format::eR32G32B32A32Sfloat:
                formatSize = 16;
                aspect = vk::ImageAspectFlagBits::eColor;
                break;
//            case vk::Format::eD32Sfloat:
//                formatSize = 4;
//                aspect = vk::ImageAspectFlagBits::eDepth;
//                break;
//            case vk::Format::eD24UnormS8Uint:
//                formatSize = 4;
//                aspect = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
//                break;
            default:
                throw std::runtime_error("failed to download image: unsupported format.");
        }

        size_t imageSize = extent.width * extent.height * extent.depth * formatSize;
        if (size != 0 && size != imageSize) {
            throw std::runtime_error("image download failed: expecting size " +
                                     std::to_string(imageSize) + ", got " +
                                     std::to_string(size));
        }
        size = imageSize;

        vk::ImageLayout sourceLayout;
        vk::AccessFlags sourceAccessFlag;
        vk::PipelineStageFlags sourceStage;

        switch (_layout) {
            case vk::ImageLayout::ePresentSrcKHR:
                sourceLayout = vk::ImageLayout::ePresentSrcKHR;
                sourceAccessFlag = {};
                sourceStage = vk::PipelineStageFlagBits::eAllCommands;
                break;
            case vk::ImageLayout::eColorAttachmentOptimal:
                sourceLayout = vk::ImageLayout::eColorAttachmentOptimal;
                sourceAccessFlag = vk::AccessFlagBits::eColorAttachmentWrite;
                sourceStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
                break;
//            case vk::ImageLayout::eDepthStencilAttachmentOptimal:
//                sourceLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
//                sourceAccessFlag = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
//                sourceStage = vk::PipelineStageFlagBits::eEarlyFragmentTests |
//                              vk::PipelineStageFlagBits::eLateFragmentTests;
//                break;
//            case vk::ImageLayout::eShaderReadOnlyOptimal:
//                sourceLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
//                sourceAccessFlag = {};
//                sourceStage = vk::PipelineStageFlagBits::eFragmentShader;
//                break;
//            case vk::ImageLayout::eTransferSrcOptimal:
//                break;
            default:
                throw std::runtime_error("failed to download image: invalid layout.");
        }

        Buffer stagingBuffer(size,
                             vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
                             { global::transferFamilyIndex },  // TODO: check this
                             vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent );
        CommandBuffer commandBuffer(global::transferCmdPool);
        commandBuffer.begin();

        if (_layout != vk::ImageLayout::eTransferSrcOptimal) {
            auto barrierInfo = getImageMemoryBarrierInfo(_image, _layout, vk::ImageLayout::eTransferSrcOptimal);
            commandBuffer.get(0).pipelineBarrier( std::get<1>( barrierInfo ), // srcStageMask
                                                  std::get<2>( barrierInfo ), // dstStageMask
                                                  vk::DependencyFlagBits::eByRegion,
                                                  0,
                                                  nullptr,
                                                  0,
                                                  nullptr,
                                                  1,
                                                  &std::get<0>( barrierInfo ) ); // barrier
        }
        vk::BufferImageCopy copyRegion(0, _extent.width, _extent.height,
                                       {aspect, 0, 0, 1}, offset, extent);
        commandBuffer.get(0).copyImageToBuffer(_image, vk::ImageLayout::eTransferSrcOptimal, stagingBuffer.get(), copyRegion);
        commandBuffer.end();
        commandBuffer.submitToQueue(global::transferQueue, {}, waitSemaphores);  // wait idle inside

        void *mapped;
        auto result = vkCore::global::device.mapMemory(
                stagingBuffer.getMemory(), 0, stagingBuffer.getSize(), {}, &mapped);
        std::memcpy(data, mapped, size);
        vkCore::global::device.unmapMemory(stagingBuffer.getMemory());
    }

    inline void download(vk::Image _image, vk::Format _format, vk::ImageLayout _layout, vk::Extent3D _extent,
                  void *data, size_t size, const std::vector<vk::Semaphore>& waitSemaphores = { }) {
        download(_image, _format, _layout, _extent, data, size, {0, 0, 0}, _extent, waitSemaphores);
    }

    template <typename DataType>
    inline std::vector<DataType> download(vk::Image _image, vk::Format _format, vk::ImageLayout _layout, vk::Extent3D _extent,
                                   vk::Offset3D offset, vk::Extent3D extent,
                                   const std::vector<vk::Semaphore>& waitSemaphores = { }) {
//        if (!isFormatCompatible<DataType>(_format)) {
//            throw std::runtime_error("Download failed: incompatible format");
//        }
        static_assert(sizeof(DataType) == 1 ||
                      sizeof(DataType) == 4); // only support char, int or float
        uint32_t formatSize;
        switch (_format) {
            case vk::Format::eR8G8B8A8Unorm:
            case vk::Format::eB8G8R8A8Unorm:
                formatSize = 4;
                break;
            case vk::Format::eR32G32B32A32Uint:
                formatSize = 16;
                break;
            case vk::Format::eR32G32B32A32Sfloat:
                formatSize = 16;
                break;
            case vk::Format::eD32Sfloat:
                formatSize = 4;
                break;
            case vk::Format::eD24UnormS8Uint:
                formatSize = 4;
                break;
            case vk::Format::eB8G8R8A8Srgb:
                formatSize = 4;
                break;
            default:
                throw std::runtime_error("failed to download image: unsupported format.");
        }
        size_t size = formatSize * extent.width * extent.height * extent.depth;
        std::vector<DataType> data(size / sizeof(DataType));
        download(_image, _format, _layout, _extent, data.data(), size, offset, extent, waitSemaphores);
        return data;
    }

    template <typename DataType>
    inline std::vector<DataType> download(vk::Image _image, vk::Format _format, vk::ImageLayout _layout, vk::Extent3D _extent,
                                          const std::vector<vk::Semaphore>& waitSemaphores = { }) {
        return download<DataType>(_image, _format, _layout, _extent, {0, 0, 0}, _extent, waitSemaphores);
    }

}
