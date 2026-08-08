
#include <sph/pipelines.h>
#include <sph/utils.h>

#include <SDL3/SDL_log.h>

static vulkan_pipeline_desc table[PIPELINE_COUNT] = {
    [PIPELINE_TEXTURED_QUAD] = {
        .name                  = "textured_quad",
        .type                  = VULKAN_PIPELINE_TYPE_GRAPHICS,
        .vertex_path           = "spv/textured_quad.spv",
        .fragment_path         = "spv/textured_quad.spv",
        .vertex_entry          = "vertexMain",
        .fragment_entry        = "fragmentMain",
        .topology              = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .depth_test            = VK_FALSE,
        .depth_write           = VK_FALSE,
        .push_constant_size    = sizeof(textured_quad_pc),
        .push_constants_stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
    },
    [PIPELINE_CUBE_LINES] = {
        .name                  = "cube_lines",
        .type                  = VULKAN_PIPELINE_TYPE_GRAPHICS,
        .vertex_path           = "spv/cube_lines.spv",
        .fragment_path         = "spv/cube_lines.spv",
        .vertex_entry          = "vertexMain",
        .fragment_entry        = "fragmentMain",
        .topology              = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
        .depth_test            = VK_TRUE,
        .depth_write           = VK_TRUE,
        .push_constant_size    = sizeof(cube_lines_pc),
        .push_constants_stages = VK_SHADER_STAGE_VERTEX_BIT,
    },
    [PIPELINE_PARTICLE_RENDER] = {
        .name                  = "particle_render",
        .type                  = VULKAN_PIPELINE_TYPE_GRAPHICS,
        .vertex_path           = "spv/particle_render.spv",
        .fragment_path         = "spv/particle_render.spv",
        .vertex_entry          = "vertexMain",
        .fragment_entry        = "fragmentMain",
        .topology              = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .depth_test            = VK_TRUE,
        .depth_write           = VK_TRUE,
        .push_constant_size    = sizeof(particle_render_pc),
        .push_constants_stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
    },
    [PIPELINE_PARTICLE_UPDATE] = {
        .name                  = "particle_update",
        .type                  = VULKAN_PIPELINE_TYPE_COMPUTE,
        .compute_path          = "spv/particle_update.spv",
        .compute_entry         = "main",
        .push_constant_size    = sizeof(particle_update_pc),
        .push_constants_stages = VK_SHADER_STAGE_COMPUTE_BIT,
    },
    [PIPELINE_PARTICLE_DENSITY] = {
        .name                  = "particle_density",
        .type                  = VULKAN_PIPELINE_TYPE_COMPUTE,
        .compute_path          = "spv/particle_density.spv",
        .compute_entry         = "main",
        .push_constant_size    = sizeof(particle_density_pc),
        .push_constants_stages = VK_SHADER_STAGE_COMPUTE_BIT,
    },
    [PIPELINE_SPATIAL_LOOKUP_WRITE] = {
        .name                  = "spatial_lookup_write",
        .type                  = VULKAN_PIPELINE_TYPE_COMPUTE,
        .compute_path          = "spv/spatial_lookup_write.spv",
        .compute_entry         = "main",
        .push_constant_size    = sizeof(spatial_lookup_write_pc),
        .push_constants_stages = VK_SHADER_STAGE_COMPUTE_BIT,
    },
    [PIPELINE_SPATIAL_LOOKUP_HISTOGRAMS] = {
        .name                  = "spatial_lookup_histograms",
        .type                  = VULKAN_PIPELINE_TYPE_COMPUTE,
        .compute_path          = "spv/spatial_lookup_histograms.spv",
        .compute_entry         = "main",
        .push_constant_size    = sizeof(spatial_lookup_histograms_pc),
        .push_constants_stages = VK_SHADER_STAGE_COMPUTE_BIT,
    },
    [PIPELINE_SPATIAL_LOOKUP_SORT] = {
        .name                  = "spatial_lookup_sort",
        .type                  = VULKAN_PIPELINE_TYPE_COMPUTE,
        .compute_path          = "spv/spatial_lookup_sort.spv",
        .compute_entry         = "main",
        .push_constant_size    = sizeof(spatial_lookup_sort_pc),
        .push_constants_stages = VK_SHADER_STAGE_COMPUTE_BIT,
    },
   [PIPELINE_START_INDICES] = {
        .name                  = "start_indices",
        .type                  = VULKAN_PIPELINE_TYPE_COMPUTE,
        .compute_path          = "spv/start_indices.spv",
        .compute_entry         = "main",
        .push_constant_size    = sizeof(start_indices_pc),
        .push_constants_stages = VK_SHADER_STAGE_COMPUTE_BIT,
    },
    [PIPELINE_COLOR_PICKER] = {
        .name                  = "color picker",
        .type                  = VULKAN_PIPELINE_TYPE_GRAPHICS,
        .vertex_path           = "spv/color_picker.spv",
        .fragment_path         = "spv/color_picker.spv",
        .vertex_entry          = "vertexMain",
        .fragment_entry        = "fragmentMain",
        .topology              = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .depth_test            = VK_TRUE,
        .depth_write           = VK_TRUE,
        .push_constant_size    = sizeof(color_picker_pc),
        .push_constants_stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
    },
    [PIPELINE_WRITE_GRADIENT] = {
        .name                  = "write_gradient",
        .type                  = VULKAN_PIPELINE_TYPE_COMPUTE,
        .compute_path          = "spv/write_gradient.spv",
        .compute_entry         = "main",
        .push_constant_size    = sizeof(write_gradient_pc),
        .push_constants_stages = VK_SHADER_STAGE_COMPUTE_BIT,
    },
};

bool pipelines_create(vulkan *vulkan, vulkan_pipeline_id *pipelines, u32 count)
{
    if (count != PIPELINE_COUNT)
    {
        SDL_Log("[ENGINE] Failed to create pipelines: count != PARTICLE_COUNT Failed to create pipelines: count != PIPELINE_COUNT.");
        return false;
    }

    for (u32 i = 0; i < PIPELINE_COUNT; i++)
    {
        vulkan_pipeline_desc desc = table[i];

        // NOTE: Convert shader paths to aboslute paths
        desc.vertex_path   = path_abs(desc.vertex_path);
        desc.fragment_path = path_abs(desc.fragment_path);
        desc.compute_path  = path_abs(desc.compute_path);

        pipelines[i] = vulkan_pipeline_create(vulkan, &desc);

        if (pipelines[i] == VULKAN_INVALID_PIPELINE)
        {
            SDL_Log("[ENGINE] Failed to create pipeline: %s", desc.name);
            return false;
        }
    }

    return true;
}
