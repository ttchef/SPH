
CC := clang

INCLUDE_PATHS := -Isrc
CFLAGS := -Wall -Wextra -pedantic -std=c11 $(INCLUDE_PATHS)

DEBUG_FLAGS := -DDEBUG -O0 -g
RELEASE_FLAGS := -DNDEBUG -O2 

ifeq ($(OS),Windows_NT)
LDFLAGS := -lSDL3 -lvulkan-1
else
LDFLAGS := -lSDL3 -lvulkan
endif

SRC_FILES := src/sph/main.c src/vk/context.c src/vk/swapchain.c src/vk/pipeline.c src/vk/command.c \
			 src/vk/buffer.c src/vk/descriptor.c src/sph/simulation.c

all: shaders debug

shaders:
	mkdir -p src/shaders/spv
	glslc src/shaders/shader.vert -fshader-stage=vert -o src/shaders/spv/shader.vert.spv
	glslc src/shaders/shader.frag -fshader-stage=frag -o src/shaders/spv/shader.frag.spv
	glslc src/shaders/update.comp -fshader-stage=comp -o src/shaders/spv/update.comp.spv
	glslc src/shaders/density.comp -fshader-stage=comp -o src/shaders/spv/density.comp.spv

debug:
	$(CC) $(SRC_FILES) $(CFLAGS) $(DEBUG_FLAGS) -o main $(LDFLAGS)

release:
	$(CC) $(SRC_FILES) $(CFLAGS) $(RELEASE_FLAGS) -o main $(LDFLAGS)
