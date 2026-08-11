
#include <sph/hdr.h>

#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_log.h>

const u8 signature[] = "#?RADIANCE";

bool hdr_create(memory_arena *arena, u32 size, void *data, image_hdr *out_image)
{
	if (SDL_memcmp(data, signature, sizeof(signature) - 1) != 0)
	{
		SDL_Log("[HDR] Invalid signature.");
		return false;
	}

	
	
	return true;
}

void hdr_destroy(image_hdr *image)
{
	
}
