
#pragma once

#include <types.h>

#include <SDL3/SDL_stdinc.h>

typedef struct
{
	char *data;
	u32 len;
} string_slice;

static inline string_slice string_slice_create(char *data, u32 len)
{
	return (string_slice){
		.data = data,
		.len = len,
	};
}

static inline bool string_slice_equal(string_slice a, string_slice b)
{
	if (a.len != b.len)
	{
		return false;
	}

	return SDL_memcmp(a.data, b.data, a.len) == 0;
}

static inline bool string_slice_equal2(string_slice a, const char *s)
{
	usize s_len = SDL_strlen(s);
	
	if (a.len != s_len)
	{
		return false;
	}

	return SDL_memcmp(a.data, s, a.len) == 0;
}

// NOTE: Splits the string into two parts by the splitting character
static inline void string_slice_split(string_slice slice, char c, string_slice *out_left, string_slice *out_right)
{
	string_slice left = {0};
	string_slice right = {0};

	left.data = slice.data;
	bool record_right = false;
	
	for (u32 i = 0; i < slice.len; i++)
	{
		if (slice.data[i] == c)
		{
			assert(i + 1 < slice.len);
			right.data = slice.data + i + 1;
			record_right = true;
			continue;
		}

		if (record_right)
		{
			++right.len;	
		}
		else
		{
			++left.len;
		}
	}

	*out_left = left;
	*out_right = right;
}
