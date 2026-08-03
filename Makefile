.PHONY: folders debug release clean run format

CC := clang
SHADERC := slangc
ASSETS := assets

ifneq ($(filter release,$(MAKECMDGOALS)),)
BUILD_MODE := release
MODE_FLAGS := -DNDEBUG -O2
else
BUILD_MODE := debug
MODE_FLAGS := -DDEBUG -O0 -g -fsanitize=address
endif

BUILD_DIR := build/$(BUILD_MODE)
OBJ_DIR := $(BUILD_DIR)/obj
SPV_DIR := $(BUILD_DIR)/spv
ASSETS_DIR := $(BUILD_DIR)/assets
EXE := $(BUILD_DIR)/main

INCLUDE_PATHS := -Isrc
DEPFLAGS := -MMD -MP
CFLAGS := -Wall -Wextra -pedantic -std=c11 $(INCLUDE_PATHS) $(DEPFLAGS) $(MODE_FLAGS)

ifeq ($(OS),Windows_NT)
LDFLAGS := -Llib -lSDL3 -lvulkan-1 -lm
else
LDFLAGS := -Llib -lSDL3 -lvulkan -lm
endif

rwildcard = $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

SRC_FILES := $(call rwildcard,src/,*.c)
OBJ_FILES := $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

# NOTE: Use of wildcard not rwildcard
SHADER_FILES := $(wildcard src/shaders/*.slang)
SPV_FILES := $(patsubst src/shaders/%.slang,$(SPV_DIR)/%.spv,$(SHADER_FILES))

all: debug

folders:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(SPV_DIR)

$(ASSETS_DIR):
	@mkdir -p $(ASSETS_DIR)
	@cp -r $(ASSETS)/. $(ASSETS_DIR)/
	@echo "Copying assets..."

debug: folders $(ASSETS_DIR) $(SPV_FILES) $(OBJ_FILES)
	@$(CC) $(OBJ_FILES) $(CFLAGS) -o $(EXE) $(LDFLAGS)
	@echo "Finished debug build."

release: folders $(ASSETS_DIR) $(SPV_FILES) $(OBJ_FILES)
	@$(CC) $(OBJ_FILES) $(CFLAGS) -o $(EXE) $(LDFLAGS)
	@echo "Finished release build."

run: debug
	@# Mainly for developing
	@./$(EXE)

format: $(SRC_FILES)
	@clang-format -i $^

clean:
	@rm -rf build

$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled: $<"

$(SPV_DIR)/%.spv: src/shaders/%.slang
	@mkdir -p $(dir $@)
	@$(SHADERC) -Isrc/shaders/modules $< -o $@
	@echo "Shader: $<"

DEP_FILES := $(OBJ_FILES:.o=.d)
-include $(DEP_FILES)
