
#include <types.h>
#include <sph/ttf.h>
#include <sph/memory.h>
#include <math/vector.h>

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_endian.h>
#include <vulkan/vulkan_core.h>

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
	u8 *flags;
	i16 *x;
	i16 *y;

	u32 point_count;
} glyf_simple;

typedef struct
{
	u16 flags;
	u16 glyph_index;
	i16 arg_1;
	i16 arg_2;
} glyf_compound;

typedef enum
{
	GLYF_TYPE_SIMPLE,
	GLYF_TYPE_COMPOUND,
} glyf_type;

typedef struct
{
	glyf_type type;

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
	GLYF_FLAG_ON_CURVE,
	GLYF_FLAG_X_SHORT_VECTOR,
	GLYF_FLAG_Y_SHORT_VECTOR,
	GLYF_FLAG_REPEAT,
	GLYF_FLAG_POSITIVE_X_VECTOR,
	GLYF_FLAG_POSITIVE_Y_VECTOR,
};

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

typedef struct
{
	v2 start;
	v2 end;
} line_segment;

static ttf_pack pack_create(u32 width, u32 height)
{
	return (ttf_pack){
		.max_width = width,
		.max_height = height,
		.skyline_count = 1,
		.skyline = SDL_calloc(width * 2, sizeof(u16)),
	};
}

static void pack_destroy(ttf_pack *pack)
{
	if (pack && pack->skyline)
	{
		SDL_free(pack->skyline);
	}
}

static inline u16 pack_skyline_x(ttf_pack *pack, u16 i)
{
	return pack->skyline[i * 2 + 0];
}

static inline u16 pack_skyline_y(ttf_pack *pack, u16 i)
{
	return pack->skyline[i * 2 + 1];
}

static inline void pack_skyline_set(ttf_pack *pack, u16 i, u16 x, u16 y)
{
	pack->skyline[i * 2 + 0] = x;
	pack->skyline[i * 2 + 1] = y;
}

static bool pack_add(ttf_pack *pack, v2u size, v2u *out_pos)
{
	if (size.x == 0 || size.y == 0)
	{
		return false;
	}

	if (pack->max_width == 0 || pack->max_height == 0)
	{
		return false;
	}

	if (size.x > pack->max_width || size.y > pack->max_height)
	{
		return false;
	}

	u16 best = UINT16_MAX;
	u16 best2 = UINT16_MAX;

	u16 best_x = 0;
	u16 best_y = UINT16_MAX;

	for (u16 i = 0; i < pack->skyline_count; i++)
	{
		u16 x = pack_skyline_x(pack, i);
		u16 y = pack_skyline_y(pack, i);

		if (x > pack->max_width || y > pack->max_height)
		{
			return false;
		}

		if (size.x > (pack->max_width - x))
		{
			break;
		}

		if (y >= best_y)
		{
			continue;
		}

		u16 x_max = x + size.x;
		u16 i2 = i + 1;

		for (; i2 < pack->skyline_count; i2++)
		{
			if (x_max <= pack_skyline_x(pack, i2))
			{
				break;
			}

			u16 y2 = pack_skyline_y(pack, i2);
			if (y < y2)
			{
				y = y2;
			}
		}

		if (y >= best_y)
		{
			continue;
		}

		if (size.y > (pack->max_height - y))
		{
			continue;
		}

		best = i;
		best2 = i2;
		best_x = x;
		best_y = y;
	}

	if (best == UINT16_MAX)
	{
		return false;
	}

	u16 removed_count = best2 - best;
	u16 new_top_left_x = best_x;
	u16 new_top_left_y = best_y + size.y;
	u16 new_bottom_right_x = best_x + size.x;
	u16 new_bottom_right_y = pack_skyline_y(pack, best2 - 1);

	bool bottom_right_point = (best2 < pack->skyline_count) ? (new_bottom_right_x < pack_skyline_x(pack, best2)) : (new_bottom_right_x < pack->max_width);

	u16 inserted_count = (u16)(1 + (bottom_right_point ? 1 : 0));

	if ((pack->skyline_count + inserted_count - removed_count) > pack->max_width)
	{
		return false;	
	}

	if (inserted_count > removed_count)
    {
        u16 delta = inserted_count - removed_count;

        for (u16 source = pack->skyline_count; source > best2; --source)
        {
            u16 src = source - 1;
            pack_skyline_set(pack, src + delta, pack_skyline_x(pack, src), pack_skyline_y(pack, src));    
        }
    }
	else if (inserted_count < removed_count)
	{
		u16 delta = removed_count - inserted_count;

		for (u16 source = best2; source < pack->skyline_count; ++source)
		{
			pack_skyline_set(pack, source - delta, pack_skyline_x(pack, source), pack_skyline_y(pack, source));
		}
	}

	pack->skyline_count = pack->skyline_count + inserted_count - removed_count;
	pack_skyline_set(pack, best, new_top_left_x, new_top_left_y);

	if (bottom_right_point)
	{
		pack_skyline_set(pack, best + 1, new_bottom_right_x, new_bottom_right_y);
	}

	out_pos->x = best_x;
	out_pos->y = best_y;

	return true;
}

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

	if (!out_cmap->format_4.entries && !out_cmap->format_12.groups)
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
	out_loca->offsets = SDL_calloc(maxp->num_glyphs + 1, sizeof(u32));
	assert(out_loca->offsets);

	if (head->index_to_loc_format == 0)
	{
		for (u32 i = 0; i < maxp->num_glyphs + 1; i++)
		{
			out_loca->offsets[i] = (u32)memory_stream_read_u16_be(&stream) * 2;
		}
	}
	else
	{
		for (u32 i = 0; i < maxp->num_glyphs + 1; i++)
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

static void glyf_simple_coordinates_read(memory_stream *stream, u32 point_count, u8 short_flag, u8 positive_flag, u8 *flags, i16 *out_coords)
{
    i16 prev = 0;
    for (u32 i = 0; i < point_count; i++)
    {
        i16 delta = 0;

        if (IS_BIT_SET(flags[i], short_flag))
        {
            u8 val = memory_stream_read_u8(stream);
            delta = IS_BIT_SET(flags[i], positive_flag) ? (i16)val : -(i16)val;
        }
        else
        {
            if (!IS_BIT_SET(flags[i], positive_flag))
            {
                delta = memory_stream_read_i16_be(stream);
            }
        }

        prev += delta;
        out_coords[i] = prev;
    }
}

static bool glyf_simple_parse(memory_stream *stream, glyf *out_glyf)
{
	out_glyf->type = GLYF_TYPE_SIMPLE;

	glyf_simple *simple = &out_glyf->simple;

	simple->end_pts_of_contours = SDL_calloc(out_glyf->number_of_contours, sizeof(u16));
	assert(simple->end_pts_of_contours);

	for (i16 i = 0; i < out_glyf->number_of_contours; i++)
	{
		simple->end_pts_of_contours[i] = memory_stream_read_u16_be(stream);
	}

	u16 instruction_length = memory_stream_read_u16_be(stream);
	memory_stream_consume(stream, instruction_length);

	simple->point_count = simple->end_pts_of_contours[out_glyf->number_of_contours - 1] + 1;
	if (simple->point_count < 0 || simple->point_count > 0xFFFF)
	{
		SDL_Log("[TTF] Invalid glyf point count: %u.", simple->point_count);
		return false;
	}

	simple->flags = SDL_calloc(simple->point_count, sizeof(u8));
	simple->x = SDL_calloc(simple->point_count, sizeof(i16));
	simple->y = SDL_calloc(simple->point_count, sizeof(i16));
	assert(simple->flags);
	assert(simple->x);
	assert(simple->y);

	for (u32 i = 0; i < simple->point_count; i++)
	{
	    u8 flag = memory_stream_read_u8(stream);
	    simple->flags[i] = flag;

	    if (IS_BIT_SET(flag, GLYF_FLAG_REPEAT))
	    {
	        u8 num_copies = memory_stream_read_u8(stream);
	        for (u32 j = 0; j < num_copies && i + 1 < simple->point_count; j++)
	        {
	            simple->flags[++i] = flag;
	        }
	    }
	}

	glyf_simple_coordinates_read(stream, simple->point_count, GLYF_FLAG_X_SHORT_VECTOR, GLYF_FLAG_POSITIVE_X_VECTOR, simple->flags, simple->x);
	glyf_simple_coordinates_read(stream, simple->point_count, GLYF_FLAG_Y_SHORT_VECTOR, GLYF_FLAG_POSITIVE_Y_VECTOR, simple->flags, simple->y);

	return true;
}

static bool glyf_compound_parse(memory_stream *stream, glyf *out_glyf)
{
	out_glyf->type = GLYF_TYPE_COMPOUND;
	return true;
}

static bool glyf_parse(u32 size, void *data, table *table, loca *loca, u16 glyph_index, glyf *out_glyf)
{
	assert(table);

	if (glyph_index >= loca->num_glyphs)
	{
		SDL_Log("[TTF] Invalid glyph_index (%u/%u)", glyph_index, loca->num_glyphs);
		return false;
	}

    out_glyf->type = GLYF_TYPE_SIMPLE;
    out_glyf->number_of_contours = 0;
    out_glyf->x_min = 0;
    out_glyf->y_min = 0;
    out_glyf->x_max = 0;
    out_glyf->y_max = 0;

	u32 start = loca->offsets[glyph_index];
	u32 end = loca->offsets[glyph_index + 1];

	if (end < start)
	{
		return false;
	}

	u32 len = end - start;
	if (len == 0)
	{
		SDL_Log("[TTF] glyf len == 0.");
	    return false;
	}

	memory_stream stream = memory_stream_reader(len, (u8 *)data + table->offset + start);

	out_glyf->number_of_contours = memory_stream_read_i16_be(&stream);
	out_glyf->x_min = memory_stream_read_i16_be(&stream);
	out_glyf->y_min = memory_stream_read_i16_be(&stream);
	out_glyf->x_max = memory_stream_read_i16_be(&stream);
	out_glyf->y_max = memory_stream_read_i16_be(&stream);

	if (out_glyf->number_of_contours == 0)
	{
		return true;	
	}
	else if (out_glyf->number_of_contours > 0)
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

static void line_segment_add(line_segment **segments, u32 *segment_capacity, u32 *segment_count, line_segment segment)	
{
	u32 capacity = *segment_capacity;
	u32 count = *segment_count;

	if (count + 1 > capacity)
	{
		capacity *= 2;
		line_segment *new = SDL_realloc(*segments, capacity * sizeof(line_segment));
		assert(new);

		*segments = new;
		*segment_capacity = capacity;			
	}
	(*segments)[count++] = segment;
	*segment_count = count;
}

static void line_segment_bezier(v2 a, v2 b, v2 control, u32 resolution, line_segment *out_segments)
{
	f32 step_size = 1.0f / (f32)(resolution - 1);

	u32 index = 0;
	v2 start = a;

	for (f32 t = 0.0f; t < 1.0f; t += step_size)
	{
		v2 p0 = v2lerp(a, control, t);
		v2 p1 = v2lerp(control, b, t);

		v2 end = v2lerp(p0, p1, t);

		out_segments[index++] = (line_segment){
			.start = start,
			.end = end,
		};
		start = end;

		assert(index < resolution);
	}
	
	assert(index + 1 == resolution);

	out_segments[index++] = (line_segment){
		.start = start,
		.end = b,
	};
}

static line_segment *glyph_segments_generate(glyf *glyf, u32 *out_count)
{
	u32 seg_count = 0;
	u32 seg_capacity = 1;
	line_segment *seg = SDL_calloc(seg_capacity, sizeof(line_segment));
	assert(seg);

	const u32 segment_resolution = 12;

	i32 index = 0;
	for (i32 contour = 0; contour < glyf->number_of_contours; contour++)
	{
		i32 start_index = index;
		i32 end_index = glyf->simple.end_pts_of_contours[contour];
		i32 count = end_index - start_index + 1;

		i32 first_on_curve = -1;
		for (i32 i = 0; i < count; i++)
		{
			if (IS_BIT_SET(glyf->simple.flags[start_index + i], GLYF_FLAG_ON_CURVE))
			{
				first_on_curve = i;
				break;
			}
		}

		if (first_on_curve == -1)
		{
			SDL_Log("[TTF] Did not find on curve point for contour... skipping.");
			continue;
		}

		v2 start = {
			.x = glyf->simple.x[start_index + first_on_curve],
			.y = glyf->simple.y[start_index + first_on_curve],
		};

		bool has_control_point = false;
		v2 control = {0};

		for (i32 j = 1; j <= count; j++)
		{
			i32 point_index = start_index + (first_on_curve + j) % count;

			v2 point = {
				.x = glyf->simple.x[point_index],	
				.y = glyf->simple.y[point_index],	
			};

			if (IS_BIT_SET(glyf->simple.flags[point_index], GLYF_FLAG_ON_CURVE))
			{
				if (has_control_point)
				{
					line_segment s[segment_resolution];
					line_segment_bezier(start, point, control, segment_resolution, s);

					for(u32 k = 0; k < segment_resolution; k++)
					{
						line_segment_add(&seg, &seg_capacity, &seg_count, s[k]);
					}
				}
				else
				{
					line_segment s = {
						.start = start,
						.end = point,	
					};
					line_segment_add(&seg, &seg_capacity, &seg_count, s);
				}

				start = point;
				has_control_point = false;
			}
			else
			{
				if (has_control_point)
				{
					v2 middle = v2lerp(control, point, 0.5f);

					line_segment s[segment_resolution];
					line_segment_bezier(start, middle, control, segment_resolution, s);

					for(u32 k = 0; k < segment_resolution; k++)
					{
						line_segment_add(&seg, &seg_capacity, &seg_count, s[k]);
					}
					start = middle;
				}
				has_control_point = true;
				control = point;
			}
		}

		index += count;
	}

	*out_count = seg_count;
	return seg;
}

static image_raw glyph_rasterize(glyf *glyf, u32 width, u32 height)
{
	image_raw result = {0};

	result.width = width;
	result.height = height;
	result.data = SDL_calloc(width * height, sizeof(u32));
	assert(result.data);

	u32 *pixels = (u32 *)result.data;

	u32 segment_count;
	line_segment *segments = glyph_segments_generate(glyf, &segment_count);

	for (u32 y = 0; y < height; y++)
	{
		for (u32 x = 0; x < width; x++)
		{
			// NOTE: Used to determent if pixel is inside or outside the shape
			i32 intersect_count = 0;

			u32 flipped_y = height - y;
			f32 normalized_y = (f32)flipped_y / (f32)height;
			f32 normalized_x = (f32)x / (f32)width;

			v2 ray = v2make(glyf->x_min + normalized_x * (glyf->x_max - glyf->x_min), glyf->y_min + normalized_y * (glyf->y_max - glyf->y_min));

			for (u32 i = 0; i < segment_count; i++)
			{
				line_segment *segment = &segments[i];

				f32 x0 = segment->start.x;
				f32 y0 = segment->start.y;

				f32 x1 = segment->end.x;
				f32 y1 = segment->end.y;

				if ((y0 > ray.y) != (y1 > ray.y))
			    {
			        f32 intersect_x = x0 + (ray.y - y0) * (x1 - x0) / (y1 - y0);

			        if (intersect_x >= ray.x)
			        {
			            if (y0 > ray.y)
			            {
			            	intersect_count--;
			            }
			            else
			            {
			            	intersect_count++;
			            }
			        }
			    }
			}

			if (intersect_count != 0)
			{
				pixels[y * width + x] = 0xFFFFFFFF;
			}		
		}
	}

	SDL_free(segments);

	return result;	
}

bool ttf_create(vulkan_context *vulkan, u32 size, void *data, ttf_font *out_font)
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

	const u32 atlas_width = 400;
	const u32 atlas_height = 400;

	image_raw atlas_raw = {
		.width = atlas_width,
		.height = atlas_height,
		.data = SDL_calloc(atlas_width * atlas_height, sizeof(u32)),
	};

	out_font->pack = pack_create(atlas_width, atlas_height);

	table *glyf_table = table_find(&directory, TAG_GLYF);
	assert(glyf_table);

	for (u8 c = 'A'; c <= 'Z'; c++)
	{
		u16 glyph_index = cmap_lookup(&cmap, c);

		SDL_Log("[TTF] Index %c: %u", c, glyph_index);

		u16 glyph_advance;
		i16 glyph_left_bearing;

		if (!hmtx_glyph_metrics_get(&hmtx, glyph_index, &glyph_advance, &glyph_left_bearing))
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

		if (glyf.type == GLYF_TYPE_COMPOUND)
		{
			SDL_Log("[TTF] No support for compound glyf's yet: %c.", c);
			continue;
		}

		i32 glyph_width_funits = glyf.x_max - glyf.x_min;
		i32 glyph_height_funits = glyf.y_max - glyf.y_min;

		if (glyph_height_funits <= 0 || glyph_width_funits <= 0)
		{
			SDL_Log("[TTF] Glyph has negative height: %c.", c);
		    continue;
		}

		f32 scale = 64.0f / (f32)head.units_per_em;

		f32 glyph_width_px = (f32)glyph_width_funits * scale;
		f32 glyph_height_px = (f32)glyph_height_funits * scale;

		f32 aspect_ratio = glyph_width_px / glyph_height_px;
		f32 new_width = glyph_height_px * aspect_ratio;
		image_raw glyph_data = glyph_rasterize(&glyf, (u32)new_width, (u32)glyph_height_px);

		f32 advance = (f32)glyph_advance * scale;

		ttf_glyph glyph = {
			.size.x = (u32)new_width,
			.size.y = (u32)glyph_height_px,
			.advance = advance,
		};

		if (!pack_add(&out_font->pack, glyph.size, &glyph.pos))
		{
			SDL_Log("[TTF] Failed to pack: %c.", c);
			continue;	
		}

		SDL_Log("%u: (%u:%u)", c, glyph.pos.x, glyph.pos.y);

		// NOTE: Write into atlas
		u32 *atlas_pixels = (u32 *)atlas_raw.data;
		u32 *glyph_pixels = (u32 *)glyph_data.data;

		for (u32 y = 0; y < glyph.size.y; y++)
		{
		    for (u32 x = 0; x < glyph.size.x; x++)
		    {
		        u32 atlas_x = glyph.pos.x + x;  
		        u32 atlas_y = glyph.pos.y + y;

		        atlas_pixels[atlas_y * atlas_width + atlas_x] = glyph_pixels[y * glyph_data.width + x];
		    }
		}

		out_font->glyphs[c - 'A'] = glyph;
	}
		
	// NOTE: turn into vulkan image
	if (!vulkan_image_create(vulkan, v2umake(atlas_raw.width, atlas_raw.height), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, &out_font->atlas))
	{
		SDL_Log("[TTF] Failed to create vulkan image.");
		return false;
	}
	vulkan_image_data_upload(vulkan, &out_font->atlas, atlas_raw.width * atlas_raw.height * 4, atlas_raw.data, v2umake(atlas_raw.width, atlas_raw.height), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	if (atlas_raw.data)
	{
		SDL_free(atlas_raw.data);
	}
	if (loca.offsets)
	{
		SDL_free(loca.offsets);
	}
	if (cmap.format_12.groups)
	{
		SDL_free(cmap.format_12.groups);
	}
	if (cmap.format_4.entries)
	{
		SDL_free(cmap.format_4.entries);
	}
	if (hmtx.metrics)
	{
		SDL_free(hmtx.metrics);
	}
	
	return true;	
}

void ttf_destroy(ttf_font *font)
{
	pack_destroy(&font->pack);
}
