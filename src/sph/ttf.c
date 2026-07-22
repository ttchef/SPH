
#include <types.h>
#include <sph/ttf.h>
#include <sph/memory.h>

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_endian.h>

//
// NOTE: TTF Spec: https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6cmap.html
//       This parser only implements cmap format 4 and 12 which are the most widely used
// 

typedef struct
{
	u32 tag;
	u32 check_sum;
	u32 offset;
	u32 length;
} table;

typedef struct
{
	u32 scalar_type;
	u16 num_tables;
	u16 search_range;
	u16 entry_selector;
	u16 range_shift;

	table *tables;
} directory;

typedef struct
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
} head;

// NOTE: only the actually needed data is parsed not the full table
typedef struct
{
	i32 version;
	u16 num_glyphs;
} maxp;

typedef struct
{
	u16 end_code;
	u16 start_code;
	u16 id_delta;
	u16 id_range_offset;

	u16 *glyph_ids;
} cmap_format_4_entry;

typedef struct
{
    u16 length;
    u16 language;
    u16 segment_count;
    u16 search_range;
    u16 entry_selector;
    u16 range_shift;

		cmap_format_4_entry *entries;
} cmap_format_4;

typedef struct
{
    u32 start_char_code;
    u32 end_char_code;
    u32 start_glyph_code;
} cmap_format_12_group;

typedef struct
{
    u16 reserved;
    u32 length;
    u32 language;
    u32 group_count;

    cmap_format_12_group *groups;
} cmap_format_12;

typedef struct
{
	u16 version;
	u16 number_subtables;
	
    cmap_format_4 format_4;
    cmap_format_12 format_12;
} cmap;

typedef struct
{
	u32 num_glyphs;
	u32 *offsets;
} loca;

typedef struct
{
	i32 version;
	i16 ascent;
	i16 descent;
	i16 line_gap;
	u16 advance_width_max;
	i16 min_left_side_bearing;
	i16 ming_right_side_bearing;
	i16 x_max_extent;
	i16 caret_slope_rise;
	i16 caret_slope_run;
	i16 caret_offset;
	i16 reserved[4];
	i16 metric_data_format;
	u16 num_of_long_hor_metrics;
} hhea;

typedef struct
{
	u16 advance_width;
	i16 left_side_bearing;
} hmtx_glyph_metric;

typedef struct
{
	u16 num_glyphs;
	hmtx_glyph_metric *metrics;
} hmtx;

typedef struct
{
	u16 *end_pts_of_contours;
	u16 instruction_length;
	u8 *instructions;
	u8 *flags;
	i16 *x;
	i16 *y;
} glyf_simple;

typedef struct
{
	u16 flags;
	u16 glyph_index;
	i16 arg_1;
	i16 arg_2;
} glyf_compound;

typedef struct
{
	i16 number_of_contours;
	i16 x_min;
	i16 y_min;
	i16 x_max;
	i16 y_max;

	union {
		glyf_simple simple;
		glyf_compound compound;	
	};
} glyf;

enum
{
	TAG_HEAD = FOURCC_BE('h', 'e', 'a', 'd'),	
	TAG_MAXP = FOURCC_BE('m', 'a', 'x', 'p'),	
	TAG_CMAP = FOURCC_BE('c', 'm', 'a', 'p'),	
	TAG_LOCA = FOURCC_BE('l', 'o', 'c', 'a'),	
	TAG_HHEA = FOURCC_BE('h', 'h', 'e', 'a'),	
	TAG_HMTX = FOURCC_BE('h', 'm', 't', 'x'),	
	TAG_GLYF = FOURCC_BE('g', 'l', 'y', 'f'),	
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


	if (out_head->version != 0x00010000)
	{
		SDL_Log("[TTF] head version mismatch.");
		return false;
	}

	return true;
}

static bool maxp_parse(u32 size, void *data, table *table, maxp *out_maxp)
{
	// NOTE: table_find can fail
	assert(table);
	
	memory_stream stream = stream_make(size, data, table);

	out_maxp->version = memory_stream_read_i32_be(&stream);
	out_maxp->num_glyphs = memory_stream_read_u16_be(&stream);

	return true;
}

static bool cmap_format_12_parse(memory_stream *stream, cmap_format_12 *out_format)
{
	out_format->reserved = memory_stream_read_u16_be(stream);
	out_format->length = memory_stream_read_u32_be(stream);
	out_format->language = memory_stream_read_u32_be(stream);
	out_format->group_count = memory_stream_read_u32_be(stream);

	if (out_format->reserved != 0)
	{
		SDL_Log("[TTF] Format 12 reserved field != 0.");
		return false;
	}

	if (out_format->group_count == 0)
	{
		SDL_Log("[TTF] Format 12 group_count == 0.");
		return false;
	}

	out_format->groups = SDL_calloc(out_format->group_count, sizeof(cmap_format_12_group));
	assert(out_format->groups);

	for (u32 i = 0; i < out_format->group_count; i++)
	{
		cmap_format_12_group *group = &out_format->groups[i];

		group->start_char_code = memory_stream_read_u32_be(stream);
		group->end_char_code = memory_stream_read_u32_be(stream);
		group->start_glyph_code = memory_stream_read_u32_be(stream);
	}
	
	return true;	
}

static bool cmap_format_4_parse(memory_stream *stream, cmap_format_4 *out_format)
{
	out_format->length = memory_stream_read_u16_be(stream);
	out_format->language = memory_stream_read_u16_be(stream);
	out_format->segment_count = memory_stream_read_u16_be(stream) / 2u;
	out_format->search_range = memory_stream_read_u16_be(stream);
	out_format->entry_selector = memory_stream_read_u16_be(stream);
	out_format->range_shift = memory_stream_read_u16_be(stream);

	out_format->entries = SDL_calloc(out_format->segment_count, sizeof(cmap_format_4_entry));
	assert(out_format->entries);

	for (u32 i = 0; i < out_format->segment_count; i++)
	{
		out_format->entries[i].end_code = memory_stream_read_u16_be(stream);
	}

	// NOTE: padding
	memory_stream_read_u16_be(stream);

	for (u32 i = 0; i < out_format->segment_count; i++)
	{
		out_format->entries[i].start_code = memory_stream_read_u16_be(stream);
	}

	for (u32 i = 0; i < out_format->segment_count; i++)
	{
		out_format->entries[i].id_delta = memory_stream_read_u16_be(stream);
	}

	u16 *id_range_offset_base = (u16 *)stream->data;
	
	for (u32 i = 0; i < out_format->segment_count; i++)
	{
		u16 offset = memory_stream_read_u16_be(stream);
		out_format->entries[i].id_range_offset = offset;

		if (offset == 0)
		{
			out_format->entries[i].glyph_ids = NULL;
		}
		else
		{
			out_format->entries[i].glyph_ids = (u16 *)((u8 *)&id_range_offset_base[i] + offset);
		}
	}

	return true;	
}

static bool cmap_parse(u32 size, void *data, table *table, cmap *out_cmap)
{
	// NOTE: table_find can fail
	assert(table);

	memory_stream stream = stream_make(size, data, table);

	out_cmap->version = memory_stream_read_u16_be(&stream);
	out_cmap->number_subtables = memory_stream_read_u16_be(&stream);

	if (out_cmap->version != 0)
	{
		SDL_Log("[TTF] cmap format != 0.");
		return false;
	}

	for (u16 i = 0; i < out_cmap->number_subtables; i++)
	{
		u16 platform_id = memory_stream_read_u16_be(&stream);
		u16 specific_id = memory_stream_read_u16_be(&stream);
		u32 offset = memory_stream_read_u32_be(&stream);

		UNUSED(platform_id);
		UNUSED(specific_id);

		memory_stream format_stream = memory_stream_reader(size - stream.size - offset, (u8 *)data + table->offset + offset);
		u16 format = memory_stream_read_u16_be(&format_stream);

		if (format == 12)
		{
			if (!cmap_format_12_parse(&format_stream, &out_cmap->format_12))
			{
				SDL_Log("[TTF] Failed to parse format 12.");
				return false;
			}
		}
		else if (format == 4)
		{
			if (!cmap_format_4_parse(&format_stream, &out_cmap->format_4))
			{
				SDL_Log("[TTF] Failed to parse format 4.");
				return false;
			}
		}
	}

	if (!out_cmap->format_4.entries || !out_cmap->format_12.groups)
	{
		SDL_Log("[TTF] Faile to parse format 4 or 12.");
		return false;
	}

	return true;
}

static u16 cmap_format_12_lookup(cmap_format_12 *format, u32 codepoint)
{
	if (codepoint > 0x10FFFF)
	{
		return 0;
	}

	for (u32 i = 0; i < format->group_count; i++)
	{
		cmap_format_12_group *group = &format->groups[i];

		if (codepoint < group->start_char_code)
		{
			return 0;
		}

		if (codepoint > group->end_char_code)
		{
			continue;
		}

		u32 glyph_index = group->start_glyph_code + (codepoint - group->start_char_code);
		if (glyph_index > 0xFFFF)
		{
			return 0;
		}

		return (u16)glyph_index;
	}

	return 0;
}

static u16 cmap_format_4_lookup(cmap_format_4 *format, u32 codepoint)
{
	for (u32 i = 0; i < format->segment_count; i++)
	{
		cmap_format_4_entry *entry = &format->entries[i];

		if (entry->end_code < codepoint)
		{
			continue;
		}

		if (entry->start_code > codepoint)
		{
			return 0;
		}

		if (entry->id_range_offset == 0)
		{
			return (u16)(codepoint + (u32)entry->id_delta) & 0xFFFF;
		}

		u32 index = codepoint - entry->start_code;
		u16 glyph_id = SDL_Swap16(entry->glyph_ids[index]);

		if (glyph_id == 0)
		{
			return 0;
		}

		return (u16)(glyph_id + entry->id_delta) & 0xFFFF;
	}
	
	return 0;
}

static u16 cmap_lookup(cmap *cmap, u32 codepoint)
{
	if (codepoint > 0xFFFF)
	{
		return cmap_format_12_lookup(&cmap->format_12, codepoint);
	}

	if (cmap->format_4.entries)
	{
		return cmap_format_4_lookup(&cmap->format_4, codepoint);
	}

	return cmap_format_12_lookup(&cmap->format_12, codepoint);
}

static bool loca_parse(u32 size, void *data, table *table, head *head, maxp *maxp, loca *out_loca)
{
	// NOTE: table_find can fail
	assert(table);

	memory_stream stream = stream_make(size, data, table);

	if (head->index_to_loc_format > 1)
	{
		SDL_Log("[TTF] Invalid index_to_loc_format.");
		return false;
	}

	out_loca->num_glyphs = maxp->num_glyphs;
	out_loca->offsets = SDL_calloc(maxp->num_glyphs, sizeof(u32));
	assert(out_loca->offsets);

	if (head->index_to_loc_format == 0)
	{
		for (u32 i = 0; i < maxp->num_glyphs; i++)
		{
			out_loca->offsets[i] = (u32)memory_stream_read_u16_be(&stream);
		}
	}
	else
	{
		for (u32 i = 0; i < maxp->num_glyphs; i++)
		{
			out_loca->offsets[i] = memory_stream_read_u32_be(&stream);
		}
	}

	return true;
}

static bool hhea_parse(u32 size, void *data, table *table, hhea *out_hhea)
{
	// NOTE: table_find can fail
	assert(table);

	memory_stream stream = stream_make(size, data, table);

	out_hhea->version = memory_stream_read_i32_be(&stream);
	out_hhea->ascent = memory_stream_read_i16_be(&stream);
	out_hhea->descent = memory_stream_read_i16_be(&stream);
	out_hhea->line_gap = memory_stream_read_i16_be(&stream);
	out_hhea->advance_width_max = memory_stream_read_u16_be(&stream);
	out_hhea->min_left_side_bearing = memory_stream_read_i16_be(&stream);
	out_hhea->ming_right_side_bearing = memory_stream_read_i16_be(&stream);
	out_hhea->x_max_extent = memory_stream_read_i16_be(&stream);
	out_hhea->caret_slope_rise = memory_stream_read_i16_be(&stream);
	out_hhea->caret_slope_run = memory_stream_read_i16_be(&stream);
	out_hhea->caret_offset = memory_stream_read_i16_be(&stream);
	out_hhea->reserved[0] = memory_stream_read_i16_be(&stream);
	out_hhea->reserved[1] = memory_stream_read_i16_be(&stream);
	out_hhea->reserved[2] = memory_stream_read_i16_be(&stream);
	out_hhea->reserved[3] = memory_stream_read_i16_be(&stream);
	out_hhea->metric_data_format = memory_stream_read_i16_be(&stream);
	out_hhea->num_of_long_hor_metrics = memory_stream_read_u16_be(&stream);

	if (out_hhea->version != 0x00010000)
	{
		SDL_Log("[TTF] hhea version mismatch.");
		return false;
	}

	if (out_hhea->reserved[0] != 0 || out_hhea->reserved[1] != 0 || out_hhea->reserved[2] != 0 || out_hhea->reserved[3] != 0)
	{
		SDL_Log("[TTF] hhea invalid reserved bytes.");
		return false;
	}

	if (out_hhea->metric_data_format != 0)
	{
		SDL_Log("[TTF] hhea metric_data_format != 0.");
		return false;
	}

	if (out_hhea->num_of_long_hor_metrics == 0)
	{
		SDL_Log("[TTF] hhea num_of_long_hor_metrics == 0.");
		return false;
	}

	return true;
}

static bool hmtx_parse(u32 size, void *data, table *table, maxp *maxp, hhea *hhea, hmtx *out_hmtx)
{
	// NOTE: table_find can fail
	assert(table);

	memory_stream stream = stream_make(size, data, table);

	out_hmtx->num_glyphs = maxp->num_glyphs;
	out_hmtx->metrics = SDL_calloc(maxp->num_glyphs, sizeof(hmtx_glyph_metric));
	assert(out_hmtx->metrics);

	u16 last_advance;
	u32 index = 0;

	for (u32 i = 0; i < hhea->num_of_long_hor_metrics; i++)
	{
		hmtx_glyph_metric metric = {
			.advance_width = memory_stream_read_u16_be(&stream)	,
			.left_side_bearing = memory_stream_read_i16_be(&stream),
		};
		out_hmtx->metrics[index++] = metric;

		last_advance = metric.advance_width;
	}

	for (u32 i = hhea->num_of_long_hor_metrics; i < maxp->num_glyphs; i++)
	{
		hmtx_glyph_metric metric = {
			.advance_width = last_advance,
			.left_side_bearing = memory_stream_read_i16_be(&stream),
		};
		out_hmtx->metrics[index++] = metric;
	}

	return true;
}

static bool hmtx_glyph_metrics_get(hmtx *hmtx, u16 glyph_index, u16 *out_advance, i16 *out_left_side_bearing)
{
	if (glyph_index >= hmtx->num_glyphs)
	{
		SDL_Log("[TTF] gylph_index out of bounds (%u/%u).", glyph_index, hmtx->num_glyphs);
		return false;
	}
	
	hmtx_glyph_metric *metric = &hmtx->metrics[glyph_index];
	assert(metric);

	*out_advance = metric->advance_width;
	*out_left_side_bearing = metric->left_side_bearing;

	return true;
}

static bool glyf_simple_parse(memory_stream *stream, glyf *out_glyf)
{

	return true;
}

static bool glyf_compound_parse(memory_stream *stream, glyf *out_glyf)
{

	return true;
}

static bool glyf_parse(u32 size, void *data, table *table, loca *loca, u16 glyph_index, glyf *out_glyf)
{
	assert(table);

	if (glyph_index > loca->num_glyphs)
	{
		SDL_Log("[TTF] Invalid glyph_index (%u/%u)", glyph_index, loca->num_glyphs);
		return false;
	}

	u32 start = loca->offsets[glyph_index];
	u32 end = loca->offsets[glyph_index + 1];

	if (end < start)
	{
		return false;
	}

	u32 len = end - start;

	memory_stream stream = memory_stream_reader(len, (u8 *)data + table->offset + start);

	out_glyf->number_of_contours = memory_stream_read_i16_be(&stream);
	out_glyf->x_min = memory_stream_read_i16_be(&stream);
	out_glyf->y_min = memory_stream_read_i16_be(&stream);
	out_glyf->x_max = memory_stream_read_i16_be(&stream);
	out_glyf->y_max = memory_stream_read_i16_be(&stream);

	if (out_glyf->number_of_contours >= 0)
	{
		SDL_Log("[TTF] Simple glyph.");
		return glyf_simple_parse(&stream, out_glyf);
	}
	else
	{
		SDL_Log("[TTF] Compound glyph.");
		return glyf_compound_parse(&stream, out_glyf);
	}

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

	cmap cmap;
	if (!cmap_parse(size, data, table_find(&directory, TAG_CMAP), &cmap))
	{
		SDL_Log("[TTF] Failed to parse cmap.");
		return false;
	}

	loca loca;
	if (!loca_parse(size, data, table_find(&directory, TAG_LOCA), &head, &maxp, &loca))
	{
		SDL_Log("[TTF] Failed to parse loca.");
		return false;
	}

	hhea hhea;
	if (!hhea_parse(size, data, table_find(&directory, TAG_HHEA), &hhea))
	{
		SDL_Log("[TTF] Failed to parse hhea.");
		return false;
	}

	hmtx hmtx;
	if (!hmtx_parse(size, data, table_find(&directory, TAG_HMTX), &maxp, &hhea, &hmtx))
	{
		SDL_Log("[TTF] Failed to parse hmtx.");
		return false;
	}

	const u32 atlas_width = 3000;
	const u32 atlas_height = 3000;

	image_raw image = {
		.width = atlas_width,
		.height = atlas_height,
		.data = SDL_calloc(atlas_width * atlas_height * 4, sizeof(u32)),
	};

	out_font->pack = (ttf_pack){
		.max_width = 3000,
		.max_height = 3000,
		.skyline = SDL_calloc(atlas_width * 2, sizeof(u16)),
	};

	SDL_Log("[TTF] num_glyphs: %u", maxp.num_glyphs);

	table *glyf_table = table_find(&directory, TAG_GLYF);
	assert(glyf_table);

	for (u8 c = 'A'; c <= 'Z'; c++)
	{
		u16 glyph_index = cmap_lookup(&cmap, c);

		u16 advance;
		i16 left_bearing;

		if (!hmtx_glyph_metrics_get(&hmtx, glyph_index, &advance, &left_bearing))
		{
			SDL_Log("[TTF] Failed to get glyph metrics for '%c'.", c);
			continue;
		}

		glyf glyf;
		if (!glyf_parse(size, data, glyf_table, &loca, glyph_index, &glyf))
		{
			SDL_Log("[TTF] Failed to parse glyf for '%c'.", c);
			continue;
		}
	}
	
	return true;	
}

void ttf_destroy(ttf_font *font)
{
	
}
