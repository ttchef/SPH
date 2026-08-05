
cmake_minimum_required(VERSION 4.0)
project(SPH C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG 53b485c7aa873af8d1dd53466d1f050c1f374b96
)

if (LINUX)
    set(SLANG_URL https://github.com/shader-slang/slang/releases/download/v2026.14.1/slang-2026.14.1-linux-x86_64.tar.gz)
elseif (WINDOWS)
    set(SLANG_URL https://github.com/shader-slang/slang/releases/download/v2026.14.1/slang-2026.14.1-windows-aarch64.zip)
elseif (APPLE)
    set(SLANG_URL https://github.com/shader-slang/slang/releases/download/v2026.14.1/slang-2026.14.1-macos-aarch64.zip)
else()
    message(STATUS "Unkown system")
endif()

FetchContent_Declare(
    SLANG
    URL ${SLANG_URL}    
)

FetchContent_Makeavailable(SDL3 SLANG)

set(SHADER_FILES
    src/shaders/cube_lines.slang
    src/shaders/textured_quad.slang
    src/shaders/particle_density.slang
    src/shaders/particle_update.slang
    src/shaders/particle_render.slang
    src/shaders/spatial_lookup_write.slang
    src/shaders/spatial_lookup_histograms.slang
    src/shaders/spatial_lookup_sort.slang
)

set(SPV_OUT ${CMAKE_BINARY_DIR}/spv)
file(MAKE_DIRECTORY ${SPV_OUT})

set(SPV_FILES)

foreach(FILE ${SHADER_FILES})
    get_filename_component(FILE_NAME ${FILE} NAME_WE)
    set(SPV_FILE ${SPV_OUT}/${FILE_NAME}.spv)

    add_custom_command(
        OUTPUT ${SPV_FILE}
        COMMAND ${SLANG_SOURCE_DIR}/bin/slangc ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}
                -I ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/modules
                -o ${SPV_FILE}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}
        COMMENT "Compiled shader: ${FILE}"
        VERBATIM
    )

    list(APPEND SPV_FILES ${SPV_FILE})
endforeach(FILE)

add_custom_target(Shaders ALL DEPENDS ${SPV_FILES})

set(SRC_FILES src/sph/app.c src/sph/camera.c src/sph/input.c
              src/sph/memory.c src/sph/pipelines.c src/sph/png.c
              src/sph/simulation.c src/sph/ttf.c src/sph/ui.c
              src/sph/ui_layout.c src/sph/window.c

              src/vk/buffer.c src/vk/command.c src/vk/context.c
              src/vk/descriptor.c src/vk/destroy_queue.c src/vk/image.c
              src/vk/pipeline.c src/vk/swapchain.c)


add_executable(${PROJECT_NAME} WIN32 ${SRC_FILES})
add_dependencies(${PROJECT_NAME} Shaders)
target_include_directories(${PROJECT_NAME} PRIVATE src)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -pedantic
        -g
        -O0
        -DDEBUG
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -O2
        -DNDEBUG
    )
endif()

if (NOT MSVC)
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan SDL3::SDL3)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            ${CMAKE_CURRENT_BINARY_DIR}/assets
    VERBATIM
)
