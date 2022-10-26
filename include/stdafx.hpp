#pragma once

#define SDL_MAIN_HANDLED

#include <SDL.h>
#include <SDL_vulkan.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "vkCore/vkCore.hpp"
#include "core/time.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>

#ifdef KUAFU_OPTIX_DENOISER
#include <cuda.h>
#include <cuda_runtime.h>
#include <driver_types.h>
#endif

#include <spdlog/spdlog.h>

#include <algorithm>
#include <any>
#include <array>
#include <charconv>
#include <filesystem>
#include <forward_list>
#include <fstream>
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

#include "core/context/global.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"

template<typename ...Types>
inline void KF_DEBUG(Types... args) { kuafu::global::logger->debug(args...); }

template<typename ...Types>
inline void KF_INFO(Types... args) { kuafu::global::logger->info(args...); }

template<typename ...Types>
inline void KF_WARN(Types... args) { kuafu::global::logger->warn(args...); }

template<typename ...Types>
inline void KF_ERROR(Types... args) { kuafu::global::logger->error(args...); }

template<typename ...Types>
inline void KF_CRITICAL(Types... args) {
    kuafu::global::logger->critical(args...); throw std::runtime_error(
            "Critical error encountered. See log above for details."); }

template<typename BType, typename ...Types>
inline void KF_ASSERT(BType st, Types... args) { if (!st) KF_CRITICAL(args...); }
