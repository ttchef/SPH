
#pragma once

#include <types.h>

typedef struct
{
    u32 capacity;
    u32 free;
    u8 *base;
} memory_arena;

typedef struct
{
    u32 size;
    u8 *data;
} memory_stream;

memory_arena memory_arena_create(u32 capacity);

void *memory_arena_alloc(memory_arena *arena, u32 size);

void *memory_arena_calloc(memory_arena *arena, u32 n, u32 size);

void memory_arena_reset(memory_arena *arena);

void memory_arena_destroy(memory_arena *arena);

memory_stream memory_stream_reader(u32 size, void *data);

void *memory_stream_consume(memory_stream *stream, u32 size);

u8 memory_stream_read_u8(memory_stream *stream);

u16 memory_stream_read_u16_be(memory_stream *stream);

u32 memory_stream_read_u32_be(memory_stream *stream);

i8 memory_stream_read_i8(memory_stream *stream);

i16 memory_stream_read_i16_be(memory_stream *stream);

i32 memory_stream_read_i32_be(memory_stream *stream);

i64 memory_stream_read_i64_be(memory_stream *stream);

f64 memory_stream_read_f2dot14(memory_stream *stream);
