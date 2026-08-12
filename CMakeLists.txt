
cmake_minimum_required(VERSION 4.0)
project(SPH C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG 53b485c7aa873af8d1dd53466d1f050c1f374b96
)

if (LINUX)
    set(SLANG_URL https://github.com/shader-slang/slang/releases/download/v2026.14.1/slang-2026.14.1-linux-x86_64.tar.gz)
elseif (WIN32)
    set(SLANG_URL https://github.com/shader-slang/slang/releases/download/v2026.14.1/slang-2026.14.1-windows-x86_64.zip)
elseif (APPLE)
    set(SLANG_URL https://github.com/shader-slang/slang/releases/download/v2026.14.1/slang-2026.14.1-macos-aarch64.zip)
else()
    message(FATAL_ERROR "Unknown system")
endif()

FetchContent_Declare(
    SLANG
    URL ${SLANG_URL}    
)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_UCLIBC_LIBM OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    VOLK
    GIT_REPOSITORY https://github.com/zeux/volk.git
    GIT_TAG dbc6d2fbb0cc07d3e42669f594a991ac35f666d9
)
set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WAYLAND_KHR)

FetchContent_Makeavailable(SDL3 SLANG VOLK)

set(SHADER_FILES
    src/shaders/cube_lines.slang
    src/shaders/textured_quad.slang
    src/shaders/particle_density.slang
    src/shaders/particle_update.slang
    src/shaders/particle_render.slang
    src/shaders/particle_reorder.slang
    src/shaders/spatial_lookup_write.slang
    src/shaders/spatial_lookup_histograms.slang
    src/shaders/spatial_lookup_sort.slang
    src/shaders/start_indices.slang
    src/shaders/color_picker.slang
)

set(SPV_OUT ${CMAKE_BINARY_DIR}/spv)
file(MAKE_DIRECTORY ${SPV_OUT})

set(SPV_FILES)

foreach(FILE ${SHADER_FILES})
    get_filename_component(FILE_NAME ${FILE} NAME_WE)
    set(SPV_FILE ${SPV_OUT}/${FILE_NAME}.spv)

    add_custom_command(
        OUTPUT ${SPV_FILE}
        COMMAND ${slang_SOURCE_DIR}/bin/slangc ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}
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
              src/vk/pipeline.c src/vk/swapchain.c
          )


add_executable(${PROJECT_NAME} WIN32 ${SRC_FILES})
add_dependencies(${PROJECT_NAME} Shaders)
target_include_directories(${PROJECT_NAME} PRIVATE src)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<C_COMPILER_ID:MSCV>:/Isrc;/I${volk_SOURCE_DIR}>
    $<$<NOT:$<C_COMPILER_ID:MSCV>>:-Isrc;-I${volk_SOURCE_DIR}>
    
    $<$<AND:$<CONFIG:Debug>,$<C_COMPILER_ID:MSVC>>:/W4>
    $<$<AND:$<CONFIG:Debug>,$<NOT:$<C_COMPILER_ID:MSVC>>>:-Wall;-Wextra;-pedantic;-g;-O0>
    $<$<AND:$<CONFIG:Release>,$<C_COMPILER_ID:MSVC>>:/O2>
    $<$<AND:$<CONFIG:Release>,$<NOT:$<C_COMPILER_ID:MSVC>>>:-O2>
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

if (NOT MSVC)
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE volk::volk SDL3::SDL3)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            ${SPV_OUT}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/spv
    VERBATIM
)
