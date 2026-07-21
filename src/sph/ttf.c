
#include <types.h>
#include <sph/ttf.h>
#include <sph/memory.h>

#include <SDL3/SDL_log.h>
#include <vulkan/vulkan_core.h>

typedef struct table
{
	u32 tag;
	u32 check_sum;
	u32 offset;
	u32 length;
} table;

typedef struct directory
{
	u32 scalar_type;
	u16 num_tables;
	u16 search_range;
	u16 entry_selector;
	u16 range_shift;

	table *tables;
} directory;

typedef struct head
{
        i32 version;
        i32 font_revision;
        u32 check_sum_adjustment;
        u32 magic_number;
        u16 flags;
        u16 units_per_em;
        i64 created;
        i64 modified;
        i16 x_min;
        i16 y_min;
        i16 x_max;
        i16 y_max;
        u16 mac_style;
        u16 lowest_rec_ppem;
        i16 font_direction_hint;
        i16 index_to_loc_format;
        i16 glyph_data_format;
} __attribute__((packed)) head;

static_assert(sizeof(head) == 54, "head structure invalid size");

// NOTE: only the actually needed data is parsed not the full table
typedef struct maxp
{
	u16 num_glyphs;
} maxp;

enum
{
	TAG_HEAD = FOURCC_BE('h', 'e', 'a', 'd'),	
	TAG_MAXP = FOURCC_BE('m', 'a', 'x', 'p'),	
};

static table *table_find(directory *directory, u32 table)
{
	for (u32 i = 0; i < directory->num_tables; i++)
	{
		if (directory->tables[i].tag == table)
		{
			return &directory->tables[i]; 
		}
	}

	SDL_Log("[TTF] Warning couldnt find a required table.");

	return NULL;
}

static bool directory_parse(u32 size, void *data, directory *out_directory)
{
	memory_stream stream = memory_stream_reader(size, data);
	
	out_directory->scalar_type = memory_stream_read_u32_be(&stream);
	out_directory->num_tables = memory_stream_read_u16_be(&stream);
	out_directory->search_range = memory_stream_read_u16_be(&stream);
	out_directory->entry_selector = memory_stream_read_u16_be(&stream);
	out_directory->range_shift = memory_stream_read_u16_be(&stream);

	if (out_directory->num_tables == 0)
	{
		SDL_Log("[TTF] Font table count is zero.");
		return false;
	}

	out_directory->tables = SDL_calloc(out_directory->num_tables, sizeof(table));
	assert(out_directory->tables);

	for (u16 i = 0; i < out_directory->num_tables; i++)
	{
		table *table = &out_directory->tables[i];

		table->tag = memory_stream_read_u32_be(&stream);
		table->check_sum = memory_stream_read_u32_be(&stream);
		table->offset = memory_stream_read_u32_be(&stream);
		table->length = memory_stream_read_u32_be(&stream);
	}
	
	return true;
}

static memory_stream stream_make(u32 size, void *data, table *table)
{
	u32 stream_size = size - table->offset;
	u8 *stream_data = (u8 *)data + table->offset;
	return memory_stream_reader(stream_size, stream_data);
}

static bool head_parse(u32 size, void *data, table *table, head *out_head)
{
	// NOTE: table_find can fail
	assert(table);

	if (table->offset >= size)
	{
		SDL_Log("[TTF] Error parsing head table.");
		return false;
	}

	memory_stream stream = stream_make(size, data, table);

	out_head->version = memory_stream_read_i32_be(&stream);
	out_head->font_revision = memory_stream_read_i32_be(&stream);
	out_head->check_sum_adjustment = memory_stream_read_u32_be(&stream);
	out_head->magic_number = memory_stream_read_u32_be(&stream);
	out_head->flags = memory_stream_read_u16_be(&stream);
	out_head->units_per_em = memory_stream_read_u16_be(&stream);
	out_head->created = memory_stream_read_i64_be(&stream);
	out_head->modified = memory_stream_read_i64_be(&stream);
	out_head->x_min = memory_stream_read_i16_be(&stream);
	out_head->y_min = memory_stream_read_i16_be(&stream);
	out_head->x_max = memory_stream_read_i16_be(&stream);
	out_head->y_max = memory_stream_read_i16_be(&stream);
	out_head->mac_style = memory_stream_read_u16_be(&stream);
	out_head->lowest_rec_ppem = memory_stream_read_u16_be(&stream);
	out_head->font_direction_hint = memory_stream_read_i16_be(&stream);
	out_head->index_to_loc_format = memory_stream_read_i16_be(&stream);
	out_head->glyph_data_format = memory_stream_read_i16_be(&stream);

	return true;
}

static bool maxp_parse(u32 size, void *data, table *table, maxp *out_maxp)
{
	// NOTE: table_find can fail
	assert(table);
	
	memory_stream stream = stream_make(size, data, table);

	return true;
}

bool ttf_create(u32 size, void *data, ttf_font *out_font)
{	
	directory directory;
	if (!directory_parse(size, data, &directory))
	{
		SDL_Log("[TTF] Failed to parse directory.");
		return false;
	}

	head head;
	if (!head_parse(size, data, table_find(&directory, TAG_HEAD), &head))
	{
		SDL_Log("[TTF] Failed to parse head.");
		return false;
	}

	maxp maxp;
	if (!maxp_parse(size, data, table_find(&directory, TAG_MAXP), &maxp))
	{
		SDL_Log("[TTF] Failed to parse maxp.");
		return false;
	}
	
	return true;	
}

void ttf_destroy(ttf_font *font)
{
	
}
