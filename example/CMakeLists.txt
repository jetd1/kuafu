cmake_minimum_required(VERSION 3.16.2)

include_directories(${PROJECT_SOURCE_DIR}/example)
file(GLOB FILES_EXAMPLE "${PROJECT_SOURCE_DIR}/example/*.cpp" )

add_executable(kuafu_example ${PROJECT_SOURCE_DIR}/example/main.cpp ${FILES_EXAMPLE})
target_link_libraries(kuafu_example kuafu)
