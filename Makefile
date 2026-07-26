
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
			 src/vk/buffer.c src/sph/simulation.c src/sph/camera.c src/sph/input.c \
			 src/vk/image.c src/sph/ttf.c src/sph/memory.c src/sph/window.c src/vk/descriptor.c

all: debug

shaders:
	mkdir -p src/shaders/spv
	slangc src/shaders/shader.slang -o src/shaders/spv/hello.spv

debug: shaders
	$(CC) $(SRC_FILES) $(CFLAGS) $(DEBUG_FLAGS) -o main $(LDFLAGS)

release: shaders
	$(CC) $(SRC_FILES) $(CFLAGS) $(RELEASE_FLAGS) -o main $(LDFLAGS)

clean:
	rm -rf src/shaders/spv
	rm main
