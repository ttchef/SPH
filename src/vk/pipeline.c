
#include <math/types.h>
#include <vk/context.h>
#include <vk/pipeline.h>
#include <vk/utils.h>

#include <SDL3/SDL_log.h>
#include <vulkan/vulkan_core.h>

typedef bool (*pipeline_create_func)(vulkan *vulkan, vulkan_pipeline_desc *desc, vulkan_pipeline *out_pipeline);

// TODO: Replace with relative to executable path or embed shader
static VkShaderModule shader_module_create(vulkan *vulkan, const char *path)
{
    assert(vulkan);
    assert(path);

    usize size;
    u8   *data = SDL_LoadFile(path, &size);
    if (!data)
    {
        SDL_Log("[VULKAN] %s", SDL_GetError());
        assert(0);
    }

    VkShaderModuleCreateInfo info = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode    = (u32 *)data,
    };

    VkShaderModule result = {0};
    if (vkCreateShaderModule(vulkan->device, &info, NULL, &result) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to create shader module: %s.", path);
        SDL_free(data);

        return result;
    }

    SDL_free(data);

    return result;
}

static bool pipeline_layout_create(vulkan *vulkan, vulkan_pipeline_desc *desc, vulkan_pipeline *out_pipeline)
{
    VkPushConstantRange push_constant = {
        .offset     = 0,
        .size       = desc->push_constant_size,
        .stageFlags = desc->push_constants_stages,
    };

    VkDescriptorSetLayout layouts[] = {
          vulkan->bindless.layout,
          vulkan->bindless.scene_layout,  
    };

    VkPipelineLayoutCreateInfo layout_info = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pPushConstantRanges    = desc->push_constant_size == 0 ? NULL : &push_constant,
        .pushConstantRangeCount = desc->push_constant_size == 0 ? 0 : 1,
        .pSetLayouts            = layouts,
        .setLayoutCount         = ARRAY_COUNT(layouts),
    };

    if (vkCreatePipelineLayout(vulkan->device, &layout_info, NULL, &out_pipeline->layout) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to create graphics pipeline layout.");
        return false;
    }

    return true;
}

static bool graphics_pipeline_create(vulkan *vulkan, vulkan_pipeline_desc *desc, vulkan_pipeline *out_pipeline)
{
    assert(vulkan);
    assert(out_pipeline);

    out_pipeline->type = VULKAN_PIPELINE_TYPE_GRAPHICS;

    VkShaderModule vertex_module   = shader_module_create(vulkan, desc->vertex_path);
    VkShaderModule fragment_module = shader_module_create(vulkan, desc->fragment_path);

    VkPipelineShaderStageCreateInfo stages[2] = {
        {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertex_module,
            .pName  = desc->vertex_entry == NULL ? "main" : desc->vertex_entry,
        },
        {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragment_module,
            .pName  = desc->fragment_entry == NULL ? "main" : desc->fragment_entry,
        },
    };

    VkPipelineRenderingCreateInfo rendering = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount    = 1,
        .pColorAttachmentFormats = &vulkan->swapchain.fmt,
        .depthAttachmentFormat   = VK_FORMAT_D32_SFLOAT,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    };

    VkPipelineVertexInputStateCreateInfo vertex_input = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = desc->vertex_attribute_count == 0 ? 0 : 1,
        .pVertexBindingDescriptions      = desc->vertex_attribute_count == 0 ? NULL : &desc->vertex_binding,
        .vertexAttributeDescriptionCount = desc->vertex_attribute_count,
        .pVertexAttributeDescriptions    = desc->vertex_attributes,
    };

    VkPipelineInputAssemblyStateCreateInfo assembly_input = {
        .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = desc->topology,
    };

    VkPipelineViewportStateCreateInfo viewport = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount  = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterization = {
        .sType            = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .lineWidth        = 1.0f,
        .depthClampEnable = VK_FALSE,
        .depthBiasEnable  = VK_FALSE,
        .polygonMode      = desc->polygon_mode,
        .cullMode         = VK_CULL_MODE_NONE,
        .frontFace        = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    };

    VkPipelineMultisampleStateCreateInfo multisample = {
        .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil = {
        .sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthCompareOp   = VK_COMPARE_OP_LESS,
        .depthTestEnable  = desc->depth_test,
        .depthWriteEnable = desc->depth_write,
        .minDepthBounds   = 0.0f,
        .maxDepthBounds   = 1.0f,
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                               VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable         = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp        = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp        = VK_BLEND_OP_ADD,
    };

    VkPipelineColorBlendStateCreateInfo color_blend = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pAttachments    = &color_blend_attachment,
        .attachmentCount = 1,
    };

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamic = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = ARRAY_COUNT(dynamic_states),
        .pDynamicStates    = dynamic_states,
    };

    VkGraphicsPipelineCreateInfo info = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pStages             = stages,
        .stageCount          = ARRAY_COUNT(stages),
        .pNext               = &rendering,
        .pVertexInputState   = &vertex_input,
        .pInputAssemblyState = &assembly_input,
        .pViewportState      = &viewport,
        .pRasterizationState = &rasterization,
        .pMultisampleState   = &multisample,
        .pDepthStencilState  = &depth_stencil,
        .pColorBlendState    = &color_blend,
        .pDynamicState       = &dynamic,
        .layout              = out_pipeline->layout,
    };

    if (vkCreateGraphicsPipelines(vulkan->device, VK_NULL_HANDLE, 1, &info, NULL, &out_pipeline->handle) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to create graphics pipeline.");
        vkDestroyShaderModule(vulkan->device, vertex_module, NULL);
        vkDestroyShaderModule(vulkan->device, fragment_module, NULL);
        return false;
    }

    vkDestroyShaderModule(vulkan->device, vertex_module, NULL);
    vkDestroyShaderModule(vulkan->device, fragment_module, NULL);

    return true;
}

static bool compute_pipeline_create(vulkan *vulkan, vulkan_pipeline_desc *desc, vulkan_pipeline *out_pipeline)
{
    out_pipeline->type = VULKAN_PIPELINE_TYPE_COMPUTE;

    VkShaderModule shader_module = shader_module_create(vulkan, desc->compute_path);

    VkPipelineShaderStageCreateInfo stage = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shader_module,
        .pName  = desc->compute_entry == NULL ? "main" : desc->compute_entry,
    };

    VkComputePipelineCreateInfo info = {
        .sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .layout = out_pipeline->layout,
        .stage  = stage,
    };

    if (vkCreateComputePipelines(vulkan->device, VK_NULL_HANDLE, 1, &info, NULL, &out_pipeline->handle) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to create compute pipeline.");
        vkDestroyShaderModule(vulkan->device, shader_module, NULL);

        return false;
    }

    vkDestroyShaderModule(vulkan->device, shader_module, NULL);

    return true;
}

static void pipeline_destroy(vulkan *vulkan, vulkan_pipeline *pipeline)
{
    assert(vulkan);
    assert(pipeline);

    vkDestroyPipelineLayout(vulkan->device, pipeline->layout, NULL);
    vkDestroyPipeline(vulkan->device, pipeline->handle, NULL);
}

bool vulkan_pipeline_manager_create(vulkan_pipeline_manager *manager)
{
    assert(manager);

    manager->count = 0;

    return true;
}

void vulkan_pipeline_manager_destroy(vulkan *vulkan, vulkan_pipeline_manager *manager)
{
    assert(vulkan);
    assert(manager);

    for (u32 i = 0; i < manager->count; i++)
    {
        pipeline_destroy(vulkan, &manager->pipelines[i]);
    }
}

vulkan_pipeline_id vulkan_pipeline_create(vulkan *vulkan, vulkan_pipeline_desc *desc)
{
    assert(vulkan);
    assert(desc);

    vulkan_pipeline_manager *manager = &vulkan->pipeline_manager;

    if (manager->count + 1 > ARRAY_COUNT(manager->pipelines))
    {
        SDL_Log("[VULKAN] Failed to create pipeline no space left (%u/%zu)", manager->count, ARRAY_COUNT(manager->pipelines));
        return VULKAN_INVALID_PIPELINE;
    }

    vulkan_pipeline *pipeline = &manager->pipelines[manager->count];
    if (!pipeline_layout_create(vulkan, desc, pipeline))
    {
        return VULKAN_INVALID_PIPELINE;
    }

    pipeline_create_func pipeline_create = desc->type == VULKAN_PIPELINE_TYPE_GRAPHICS ? graphics_pipeline_create : compute_pipeline_create;
    if (!pipeline_create(vulkan, desc, pipeline))
    {
        vkDestroyPipelineLayout(vulkan->device, pipeline->layout, NULL);
        return VULKAN_INVALID_PIPELINE;
    }

    vulkan_object_name_set(vulkan, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (u64)pipeline->layout, "pipeline_layout:%s", desc->name);
    vulkan_object_name_set(vulkan, VK_OBJECT_TYPE_PIPELINE, (u64)pipeline->handle, "pipeline:%s", desc->name);

    manager->count++;

    return manager->count;
}

vulkan_pipeline *vulkan_pipeline_get(vulkan *vulkan, vulkan_pipeline_id id)
{
    assert(vulkan);

    vulkan_pipeline_manager *manager = &vulkan->pipeline_manager;
    assert(id != VULKAN_INVALID_PIPELINE && id <= manager->count);

    vulkan_pipeline *result = &manager->pipelines[id - 1];

    return result;
}
