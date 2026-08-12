
#include <sph/hdr.h>

#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_log.h>

const u8 signature[] = "#?RADIANCE";

bool hdr_create(memory_arena *arena, u32 size, void *data, image_hdr *out_image)
{
	image_hdr result = {0};
	
	memory_stream stream = memory_stream_reader(size, data);

	char *ascii_signature = memory_stream_consume(&stream, sizeof(signature));
	
	if (SDL_memcmp(ascii_signature, signature, sizeof(signature) - 1) != 0)
	{
		SDL_Log("[HDR] Invalid signature.");
		return false;
	}

	string_slice slice = memory_stream_skip_line(&stream);
	while (slice.len > 0)
	{
		slice = memory_stream_skip_line(&stream);

		// NOTE: Skip comment
		if (slice.data[0] == '#')
		{
			continue;
		}

		string_slice key;
		string_slice value;
		string_slice_split(slice, '=', &key, &value);

		if (string_slice_equal2(key, "FORMAT"))
		{
			if (!string_slice_equal2(value, "32-bit_rle_rgbe"))
			{
				SDL_Log("[HDR] Invalid header format: %.*s", value.len, value.data);
				return false;
			}
		}
	}


	*out_image = result;
	
	return true;
}

void hdr_destroy(image_hdr *image)
{
	
}
