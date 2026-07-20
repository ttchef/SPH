
CC := clang

INCLUDE_PATHS := -Isrc
CFLAGS := -Wall -Wextra -pedantic -std=c11 $(INCLUDE_PATHS)
GLSLC_FLAGS := --target-env=vulkan1.3

DEBUG_FLAGS := -DDEBUG -O0 -g
RELEASE_FLAGS := -DNDEBUG -O2 

ifeq ($(OS),Windows_NT)
LDFLAGS := -lSDL3 -lvulkan-1 -lm
else
LDFLAGS := -lSDL3 -lvulkan -lm
endif

SRC_FILES := src/sph/main.c src/vk/context.c src/vk/swapchain.c src/vk/pipeline.c src/vk/command.c \
			 src/vk/buffer.c src/vk/descriptor.c src/sph/simulation.c src/sph/camera.c src/sph/input.c \
			 src/vk/image.c src/sph/render.c src/sph/ui.c

all: debug

shaders:
	mkdir -p src/shaders/spv
	glslc src/shaders/particle.vert -fshader-stage=vert $(GLSLC_FLAGS) -o src/shaders/spv/particle.vert.spv
	glslc src/shaders/particle.frag -fshader-stage=frag $(GLSLC_FLAGS) -o src/shaders/spv/particle.frag.spv
	glslc src/shaders/cube.vert -fshader-stage=vert $(GLSLC_FLAGS) -o src/shaders/spv/cube.vert.spv
	glslc src/shaders/cube.frag -fshader-stage=frag $(GLSLC_FLAGS) -o src/shaders/spv/cube.frag.spv
	glslc src/shaders/cube_line.vert -fshader-stage=vert $(GLSLC_FLAGS) -o src/shaders/spv/cube_line.vert.spv
	glslc src/shaders/cube_line.frag -fshader-stage=frag $(GLSLC_FLAGS) -o src/shaders/spv/cube_line.frag.spv
	glslc src/shaders/quad.vert -fshader-stage=vert $(GLSLC_FLAGS) -o src/shaders/spv/quad.vert.spv
	glslc src/shaders/quad.frag -fshader-stage=frag $(GLSLC_FLAGS) -o src/shaders/spv/quad.frag.spv
	glslc src/shaders/update.comp -fshader-stage=comp $(GLSLC_FLAGS) -o src/shaders/spv/update.comp.spv
	glslc src/shaders/density.comp -fshader-stage=comp $(GLSLC_FLAGS) -o src/shaders/spv/density.comp.spv
	glslc src/shaders/spatial_lookup.comp -fshader-stage=comp $(GLSLC_FLAGS) -o src/shaders/spv/spatial_lookup.comp.spv
	glslc src/shaders/radixsort_histograms.comp -fshader-stage=comp $(GLSLC_FLAGS) -o src/shaders/spv/radixsort_histograms.comp.spv
	glslc src/shaders/radixsort.comp -fshader-stage=comp $(GLSLC_FLAGS) -o src/shaders/spv/radixsort.comp.spv
	glslc src/shaders/start_indices.comp -fshader-stage=comp $(GLSLC_FLAGS) -o src/shaders/spv/start_indices.comp.spv

debug: shaders
	$(CC) $(SRC_FILES) $(CFLAGS) $(DEBUG_FLAGS) -o main $(LDFLAGS)

release: shaders
	$(CC) $(SRC_FILES) $(CFLAGS) $(RELEASE_FLAGS) -o main $(LDFLAGS)

clean:
	rm -rf src/shaders/spv
	rm main
