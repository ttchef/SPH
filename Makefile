
CC := clang

INCLUDE_PATHS := -Isrc
CFLAGS := -Wall -Wextra -pedantic -std=c11 $(INCLUDE_PATHS)

DEBUG_FLAGS := -DDEBUG -O0 -g
RELEASE_FLAGS := -DNDEBUG -O2 

LDFLAGS := -lSDL3 -lvulkan

SRC_FILES := src/sph/main.c src/vk/context.c src/vk/swapchain.c src/vk/pipeline.c src/vk/command.c

all: shaders debug

shaders:
	mkdir -p src/shaders/spv
	glslc src/shaders/triangle.vert -fshader-stage=vert -o src/shaders/spv/triangle.vert.spv
	glslc src/shaders/triangle.frag -fshader-stage=frag -o src/shaders/spv/triangle.frag.spv

debug:
	$(CC) $(SRC_FILES) $(CFLAGS) $(DEBUG_FLAGS) -o main $(LDFLAGS)

release:
	$(CC) $(SRC_FILES) $(CFLAGS) $(RELEASE_FLAGS) -o main $(LDFLAGS)
