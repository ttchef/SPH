

INCLUDE_PATHS := -Isrc
CFLAGS := -Wall -Wextra -pedantic -std=c11 $(INCLUDE_PATHS)
LDFLAGS := -lSDL3

all:
	clang src/new_main.c $(CFLAGS) -o main $(LDFLAGS)
