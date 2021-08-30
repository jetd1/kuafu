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
//
// This is currently a dummy class due to ImGui conflict with svulkan2
//

#pragma once

#include "stdafx.hpp"

namespace kuafu {
/// A class to create an ImGui-based GUI.
///
/// This class acts like an interface for the user to create their own GUI.
/// It is possible to create multiple GUI objects and re-assign them to the renderer at runtime.
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~.cpp
/// class CustomGui : public Gui
/// {
/// private:
///   // Configure style and input of the GUI.
///   void configure() override { }
///
///   // Put the rendering commands in here.
///   void render() override { }
/// };
///
/// myRenderer.init( );
///
/// // Put this line after the declaration of the kuafu::Kuafu object.
/// auto myGui = std::make_shared<CustomGui>( );
/// myRenderer.setGui( myGui );
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// @warning The GUI object must be declared after the renderer to ensure the correct order for destroying and freeing resources.
/// @ingroup BASE API
class Gui {
public:
    virtual ~Gui() = default;

    /// Used to configure all ImGui settings.
    ///
    /// The user should override this function if they want to change the style or various other configuration settings.
    virtual void configure();

    /// This function is for calling the individual ImGui components, e.g. widgets.
    ///
    /// The user should override this function to create their own GUI.
    virtual void render();

    /// Creates the GUI and all required Vulkan components.
    /// @param surface A pointer to a kuafu::Surface object.
    /// @param swapchainImageExtent The extent of the swapchain images.
    void init(SDL_Window *window, const vkCore::Surface *surface, vk::Extent2D swapchainImageExtent,
              vk::RenderPass renderPass);

    /// Used to recreate the GUI in case the window size was changed.
    /// @param swapchainImageExtent The extent of the swapchain images.
    /// @param swapchainImageViews The swapchain images' image views.
    void recreate(vk::Extent2D swapchainImageExtent);

    /// Records the ImGui rendering calls to the command buffer at the given image index.
    /// @param imageIndex The index addressing a command buffer.
    void renderDrawData(vk::CommandBuffer commandBuffer);

    /// Destroys all ImGui resources.
    void destroy();

private:
    /// Creates a descriptor pool.
    void initDescriptorPool();

    /// Creates the ImGui font textures.
    void initFonts();

    vk::UniqueDescriptorPool mDescriptorPool; ///< A Vulkan descriptor pool with a unique handle.
    vk::UniqueCommandPool mCommandPool;       ///< A Vulkan command pool with a unique handle.
    vk::Extent2D mSwapchainImageExtent;       ///< The swapchain images' image extent.
    SDL_Window *pWindow = nullptr;
};
}
