cmake_minimum_required(VERSION 3.16.2)

# This makes it possible to use relative paths which usually start from the application's location. This may vary depending on your setup.
option(COPY_ASSETS_TO_BINARY "Copy assets to binary" ON)

if (COPY_ASSETS_TO_BINARY)
    add_compile_definitions(KF_COPY_ASSETS)
endif ()


include_directories(${PROJECT_SOURCE_DIR}/example)
file(GLOB FILES_EXAMPLE
        "${PROJECT_SOURCE_DIR}/example/*.cpp"
        )

add_executable(example ${PROJECT_SOURCE_DIR}/example/main.cpp ${FILES_EXAMPLE})

target_link_libraries(example
        kuafu)
