
SRC_DIR := src

CC := clang

DEBUG_FLAGS := 
CFLAGS := -Wall -Wextra -pedantic -O2 -Isrc
LDFLAGS := -lraylib -lm

SRC_FILES := $(SRC_DIR)/main.c
EXE := main

all: debug

debug:
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SRC_FILES) -o $(EXE) $(LDFLAGS)

release:
	$(CC) $(CFLAGS) $(SRC_FILES) -o $(EXE) $(LDFLAGS)
