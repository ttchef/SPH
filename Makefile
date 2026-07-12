
CC := clang

INCLUDE_PATHS := -Isrc
CFLAGS := -Wall -Wextra -pedantic -std=c11 $(INCLUDE_PATHS)

DEBUG_FLAGS := -DDEBUG -O0 -g
RELEASE_FLAGS := -DNDEBUG -O2 

LDFLAGS := -lSDL3 -lvulkan

SRC_FILES := src/sph/main.c src/vk/context.c src/vk/swapchain.c 

all: debug

debug:
	$(CC) $(SRC_FILES) $(CFLAGS) $(DEBUG_FLAGS) -o main $(LDFLAGS)

release:
	$(CC) $(SRC_FILES) $(CFLAGS) $(RELEASE_FLAGS) -o main $(LDFLAGS)
