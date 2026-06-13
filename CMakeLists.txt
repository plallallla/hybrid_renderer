cmake_minimum_required(VERSION 3.10)

project(HybridCPU_Renderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GEOMETRY_SOURCES
    src/geometry/ray_scene.cpp
    src/geometry/trace_primitive.cpp
)

add_executable(hybrid_renderer
    src/app/main.cpp
    src/asset/scene_loader.cpp
    src/core/png_writer.cpp
    src/renderer/rasterizer/rasterizer.cpp
    src/renderer/trace_primitive_builder.cpp
    src/scene/obj_loader.cpp
    src/third_party/stb_image_impl.cpp
    ${GEOMETRY_SOURCES}
)

target_include_directories(hybrid_renderer PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

add_executable(geometry_tests
    tests/geometry_tests.cpp
    ${GEOMETRY_SOURCES}
)

target_include_directories(geometry_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

add_executable(gbuffer_tests
    tests/gbuffer_tests.cpp
    src/renderer/rasterizer/rasterizer.cpp
)

target_include_directories(gbuffer_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)
