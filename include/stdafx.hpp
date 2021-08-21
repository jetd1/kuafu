//
// Created by jet on 4/9/21.
//

#pragma once


#define SDL_MAIN_HANDLED

#include <SDL.h>
#include <SDL_vulkan.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VK_ENABLE_BETA_EXTENSIONS

#include <vkCore/vkCore.hpp>

#include "core/time.hpp"

//#include <ImGui/imgui.h>
//#include <ImGui/imgui_impl_sdl.h>
//#include <ImGui/imgui_impl_vulkan.h>
//#define GLM_SWIZZLE
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#define GLFORCE_DEPTH_ZERO_TO_ONE

#include <algorithm>
#include <any>
#include <array>
#include <charconv>
#include <filesystem>
#include <forward_list>
#include <fstream>
#include <gsl/gsl>
#include <iomanip>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <set>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>