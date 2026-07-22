#include <sph/render.h>
#include <vk/context.h>
#include <math/core.h>
#include <vulkan/vulkan_core.h>

// NOTE: Used for both cube and cube_lines
typedef struct cube_pc
{
	m4 mvp;
	v4 color;
} cube_pc;

typedef struct quad_pc
{
	m4 mvp;
	v4 color;
} quad_pc;


bool render_create(vulkan_context *vulkan, render *render)
{
	vulkan_pipeline_desc cube_description = vulkan_pipeline_default(VULKAN_PIPELINE_TYPE_GRAPHICS);
	vulkan_pipeline_desc_set_push_constant(&cube_description, sizeof(cube_pc), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	vulkan_pipeline_desc_set_shaders(&cube_description, "src/shaders/spv/cube.vert.spv", "src/shaders/spv/cube.frag.spv", NULL);

	render->cube_pipeline = vulkan_pipeline_create(vulkan, &cube_description);
	assert(render->cube_pipeline != INVALID_PIPELINE);

	vulkan_pipeline_desc cube_lines_description = vulkan_pipeline_default(VULKAN_PIPELINE_TYPE_GRAPHICS);

	vulkan_pipeline_desc_set_push_constant(&cube_lines_description, sizeof(cube_pc), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	vulkan_pipeline_desc_set_shaders(&cube_lines_description, "src/shaders/spv/cube_line.vert.spv", "src/shaders/spv/cube_line.frag.spv", NULL);
	vulkan_pipeline_desc_set_topology(&cube_lines_description, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);

	render->cube_lines_pipeline = vulkan_pipeline_create(vulkan, &cube_lines_description);
	assert(render->cube_lines_pipeline != INVALID_PIPELINE);

	vulkan_pipeline_desc quad_description = vulkan_pipeline_default(VULKAN_PIPELINE_TYPE_GRAPHICS);

	vulkan_pipeline_desc_set_push_constant(&quad_description, sizeof(quad_pc), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	vulkan_pipeline_desc_set_shaders(&quad_description, "src/shaders/spv/quad.vert.spv", "src/shaders/spv/quad.frag.spv", NULL);

	render->quad_pipeline = vulkan_pipeline_create(vulkan, &quad_description);
	assert(render->quad_pipeline != INVALID_PIPELINE);

	vulkan_pipeline_desc_set_depth(&quad_description, VK_FALSE, VK_FALSE);
	
	render->quad_scene_pipeline = vulkan_pipeline_create(vulkan, &quad_description);
	assert(render->quad_scene_pipeline != INVALID_PIPELINE);
	
	return true;
}

void render_cube(vulkan_context *vulkan, render *render, v3 pos, v3 size, color4 color, m4 view_proj)
{
	vulkan_command_bind_pipeline(vulkan, render->cube_pipeline);
	
	m4 translate = m4translate(pos.x, pos.y, pos.z);
	m4 scale = m4scale(size.x, size.y, size.z);

	m4 model = m4mul(scale, translate);
	m4 mvp = m4mul(view_proj, model);

	cube_pc pc = {
		.mvp = mvp,
		.color = v4fromcolor4(color),
	};
	
	vulkan_command_push_constants(vulkan, sizeof(pc), &pc, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, render->cube_pipeline);
	vulkan_command_draw(vulkan,  36);
}

void render_cube_lines(vulkan_context *vulkan, render *render, v3 pos, v3 size, color4 color, m4 view_proj)
{
	vulkan_command_bind_pipeline(vulkan, render->cube_lines_pipeline);
	
	m4 translate = m4translate(pos.x, pos.y, pos.z);
	m4 scale = m4scale(size.x, size.y, size.z);

	m4 model = m4mul(scale, translate);
	m4 mvp = m4mul(view_proj, model);

	cube_pc pc = {
		.mvp = mvp,
		.color = v4fromcolor4(color),
	};
	
	vulkan_command_push_constants(vulkan, sizeof(pc), &pc, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, render->cube_lines_pipeline);
	vulkan_command_draw(vulkan,  36);
}

void render_quad(vulkan_context *vulkan, render *render, v3 pos, v2 size, color4 color, m4 view_proj)
{
	vulkan_command_bind_pipeline(vulkan, render->quad_pipeline);
	
	m4 translate = m4translate(pos.x, pos.y, pos.z);
	m4 scale = m4scale(size.x, size.y, 1.0f);

	m4 model = m4mul(scale, translate);
	m4 mvp = m4mul(view_proj, model);

	quad_pc pc = {
		.mvp = mvp,
		.color = v4fromcolor4(color),
	};
	
	vulkan_command_push_constants(vulkan, sizeof(pc), &pc, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, render->quad_pipeline);
	vulkan_command_draw(vulkan, 6);
}

void render_screen_quad(vulkan_context *vulkan, render *render, v2 pos, v2 size, color4 color, m4 orthographic)
{
	vulkan_command_bind_pipeline(vulkan, render->quad_scene_pipeline);

	pos = v2add(pos, v2scale(size, 0.5f));	
	m4 translate = m4translate(pos.x, pos.y, 0.0f);
	m4 scale = m4scale(size.x, size.y, 1.0f);

	m4 model = m4mul(translate, scale);
	m4 mvp = m4mul(orthographic, model);

	quad_pc pc = {
		.mvp = mvp,
		.color = v4fromcolor4(color),
	};
	
	vulkan_command_push_constants(vulkan, sizeof(pc), &pc, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, render->quad_scene_pipeline);
	vulkan_command_draw(vulkan, 6);
}

void render_text(vulkan_context *vulkan, render *render, ttf_font *font, const char *text, v2 pos, f32 scale, m4 orthographic)
{
	v2 size = v2make(scale * 64, scale * 64);
	for (u32 i = 0; i < SDL_strlen(text); i++)
	{
		render_screen_quad(vulkan, render, pos, size, RED, orthographic);

		pos.x += font->glyphs[text[i] - 'A'].advance * scale;	
	}
}
