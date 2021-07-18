cmake_minimum_required(VERSION 3.10)
project(kuafu VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)

if(NOT CMAKE_BUILD_TYPE)
    message("MX: Build type not set. Continuing with: Debug")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "No build type was set" FORCE)
endif(NOT CMAKE_BUILD_TYPE)

find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
set(WINDOW_LIBS ${WINDOW_LIBS} "${SDL2_LIBRARIES}")

link_directories(${PROJECT_SOURCE_DIR}/external_libs/KTX)
set(KTX ${KTX} "libktx.so" "libktx_read.so")

file(GLOB_RECURSE RENDERER_SRC "src/*")

include_directories("include")
include_directories("include/external")
add_library(kuafu SHARED ${RENDERER_SRC})
target_link_libraries(kuafu
        ${Vulkan_LIBRARY}
        ${WINDOW_LIBS}
        ${KTX}
)
target_precompile_headers(kuafu
        PUBLIC "${PROJECT_SOURCE_DIR}/include/stdafx.hpp"
)

add_compile_definitions(KF_ASSETS_PATH="${PROJECT_SOURCE_DIR}/assets/")
#add_compile_definitions(KF_PATH_TO_LIBRARY="${PROJECT_SOURCE_DIR}/build/bin/${BUILD_PATH}/")
add_compile_definitions(KF_GLSLC_PATH="/usr/bin/glslc")
include(${PROJECT_SOURCE_DIR}/example/CMakeLists.txt)
