cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(kuafu VERSION 0.1)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_DEBUG "-o0 -g3 -Wall -Wnon-virtual-dtor -fsized-deallocation -Wno-deprecated-declarations")
set(CMAKE_CXX_FLAGS_RELEASE "-o3 -g0 -Wall -Wnon-virtual-dtor  -fsized-deallocation -Wno-deprecated-declarations")

find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

if (glslc_executable STREQUAL "glslc_executable-NOTFOUND")
    message(FATAL_ERROR "glslc needed for compiling the shaders \nHint: did you install vulkan-sdk?")
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("-- DEBUG Build")
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_DEBUG})
    add_definitions(-DVK_VALIDATION)
    add_definitions(-DVK_CORE_LOGGING)
    add_definitions(-D_DEBUG)
    add_compile_definitions(KF_GLSLC_PATH="${glslc_executable}")
    message("-- KF: shaders will be rebuilt during runtime in the Debug build")
else ()
    message("-- RELEASE Build")
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_RELEASE})
    add_definitions(-DNDEBUG)
    add_compile_definitions(KF_GLSLC_PATH="")
endif ()

find_package(spdlog REQUIRED)
find_package(Vulkan REQUIRED)
find_package(assimp 5.0.1 REQUIRED PACKAGE_FIND_VERSION)
find_package(Ktx REQUIRED)
find_package(SDL2 REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})
include_directories(${SDL2_INCLUDE_DIRS})

#set(KTX_FEATURE_TESTS OFF)
#set(KTX_FEATURE_TOOLS OFF)
#set(KTX_FEATURE_LOADTEST_APPS OFF)
#add_subdirectory(3rd_party/KTX-Software)
#add_subdirectory(3rd_party/SDL)
#
#include_directories(3rd_party/SDL/include)
#include_directories(3rd_party/KTX-Software/include)

add_custom_target(
        compile_kf_shaders COMMAND
        ${glslc_executable} ${PROJECT_SOURCE_DIR}/resources/shaders/PathTrace.rahit -o ${PROJECT_SOURCE_DIR}/resources/shaders/PathTrace.rahit.spv --target-env=vulkan1.2 &&
        ${glslc_executable} ${PROJECT_SOURCE_DIR}/resources/shaders/PathTrace.rchit -o ${PROJECT_SOURCE_DIR}/resources/shaders/PathTrace.rchit.spv --target-env=vulkan1.2 &&
        ${glslc_executable} ${PROJECT_SOURCE_DIR}/resources/shaders/PathTrace.rgen  -o ${PROJECT_SOURCE_DIR}/resources/shaders/PathTrace.rgen.spv --target-env=vulkan1.2 &&
        ${glslc_executable} ${PROJECT_SOURCE_DIR}/resources/shaders/PathTrace.rmiss -o ${PROJECT_SOURCE_DIR}/resources/shaders/PathTrace.rmiss.spv --target-env=vulkan1.2 &&
        ${glslc_executable} ${PROJECT_SOURCE_DIR}/resources/shaders/PathTraceShadow.rmiss -o ${PROJECT_SOURCE_DIR}/resources/shaders/PathTraceShadow.rmiss.spv --target-env=vulkan1.2 &&
        ${glslc_executable} ${PROJECT_SOURCE_DIR}/resources/shaders/PostProcessing.frag -o ${PROJECT_SOURCE_DIR}/resources/shaders/PostProcessing.frag.spv --target-env=vulkan1.2 &&
        ${glslc_executable} ${PROJECT_SOURCE_DIR}/resources/shaders/PostProcessing.vert -o ${PROJECT_SOURCE_DIR}/resources/shaders/PostProcessing.vert.spv --target-env=vulkan1.2
)

file(GLOB_RECURSE RENDERER_SRC "src/*")

include_directories("include")
include_directories("include/external")
add_library(kuafu SHARED ${RENDERER_SRC})
add_dependencies(kuafu compile_kf_shaders)
target_link_libraries(kuafu vulkan assimp ktx SDL2)
target_precompile_headers(kuafu PUBLIC "${PROJECT_SOURCE_DIR}/include/stdafx.hpp")

include(${PROJECT_SOURCE_DIR}/example/CMakeLists.txt)
