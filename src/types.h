
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
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

typedef size_t usize;

static_assert(sizeof(f32) == 4, "a f32 is not 4 bytes");
static_assert(sizeof(f64) == 8, "a f64 is not 8 bytes");

//
// NOTE: Helper macros
//

#define UNUSED(x) (void)(x);
#define ARRAY_COUNT(x) (sizeof((x)) / sizeof((x)[0]))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(n, lower, upper) (MAX(MIN(n, upper), lower))
#define KILOBYTES(x) ((x) * 1024)
#define MEGABYTES(x) ((KILOBYTES(x)) * 1024)
