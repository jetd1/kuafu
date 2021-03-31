cmake_minimum_required(VERSION 3.17)
project(vkPlayground)

set(CMAKE_CXX_STANDARD 20)

add_definitions(-DDEBUG)

include_directories(/usr/local/cuda-11.1/targets/x86_64-linux/include/)
link_directories(/usr/local/cuda-11.1/targets/x86_64-linux/lib/)


add_executable(vkPlayground vulkan-tutorial/main.cpp vulkan-tutorial/context.cpp vulkan-tutorial/context.h)
target_link_libraries(vkPlayground vulkan glfw3 cudart dl pthread X11 Xxf86vm Xrandr Xi)

