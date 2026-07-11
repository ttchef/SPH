
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

static_assert(sizeof(f32) == 4, "a f32 is not 4 bytes");
static_assert(sizeof(f64) == 8, "a f64 is not 8 bytes");

#define UNUSED(x) (void)(x);
