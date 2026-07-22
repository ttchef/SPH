
#include <sph/memory.h>

memory_stream memory_stream_reader(u32 size, void *data)
{
	memory_stream result = {0};

	result.size = size;
	result.data = data;

	return result;
}

void *memory_stream_consume(memory_stream *stream, u32 size)
{
	if (stream->size < size)
	{
		assert(0);
	}

	void *result = stream->data;
	stream->size -= size;
	stream->data += size;

	return result;
}

u8 memory_stream_read_u8(memory_stream *stream)
{
	return *(u8 *)memory_stream_consume(stream, sizeof(u8));
}

u16 memory_stream_read_u16_be(memory_stream *stream)
{
	u8 *x = memory_stream_consume(stream, sizeof(u16));
	return ((u16)x[0] << 8) | ((u16)x[1]);
}

u32 memory_stream_read_u32_be(memory_stream *stream)
{
	u8 *x = memory_stream_consume(stream, sizeof(u32));
	return ((u32)x[0] << 24) | ((u32)x[1] << 16) | ((u32)x[2] << 8) | ((u32)x[3]);
}

i8 memory_stream_read_i8(memory_stream *stream)
{
	return *(i8 *)memory_stream_consume(stream, sizeof(i8));
}

i16 memory_stream_read_i16_be(memory_stream *stream)
{
	u8 *x = memory_stream_consume(stream, sizeof(i16));
	return ((i16)x[0] << 8) | ((i32)x[1]);
}

i32 memory_stream_read_i32_be(memory_stream *stream)
{
	u8 *x = memory_stream_consume(stream, sizeof(i32));
	return ((i32)x[0] << 24) | ((i32)x[1] << 16) | ((i32)x[2] << 8) | ((i32)x[3]);
}

i64 memory_stream_read_i64_be(memory_stream *stream)
{
	u8 *x = memory_stream_consume(stream, sizeof(i64));
	return ((i64)x[0] << 56) | ((i64)x[1] << 48) | ((i64)x[2] << 40) | ((i64)x[3] << 32) | ((i64)x[4] << 24) | ((i64)x[5] << 16) | ((i64)x[6] << 8) | ((i64)x[7]);
}
