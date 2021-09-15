#pragma once

#define SDL_MAIN_HANDLED

#include <SDL.h>
#include <SDL_vulkan.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <vkCore/vkCore.hpp>

#include "core/time.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

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

template<typename ..._T>
inline void KF_DEBUG(_T... args) { kuafu::global::logger->debug(args...); }

template<typename ..._T>
inline void KF_INFO(_T... args) { kuafu::global::logger->info(args...); }

template<typename ..._T>
inline void KF_WARN(_T... args) { kuafu::global::logger->warn(args...); }

template<typename ..._T>
inline void KF_ERROR(_T... args) { kuafu::global::logger->error(args...); }

template<typename ..._T>
inline void KF_CRITICAL(_T... args) { kuafu::global::logger->critical(args...); }

template<typename ..._T>
inline void KF_ASSERT(bool st, _T... args) {
    if (!st) { kuafu::global::logger->critical(args...); throw std::runtime_error("assertion failed"); } }
