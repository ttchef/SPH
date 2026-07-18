
#include "vk/descriptor.h"
#include <vk/pipeline.h>
#include <vk/context.h>
#include <math/types.h>

#include <SDL3/SDL_log.h>
#include <vulkan/vulkan_core.h>

typedef bool (*pipeline_create_func)(vulkan_context *ctx, vulkan_pipeline_desc *desc, vulkan_pipeline *out_pipeline);

//
// NOTE: Pipeline builder
// 

vulkan_pipeline_desc vulkan_pipeline_default(vulkan_pipeline_type type)
{
	vulkan_pipeline_desc result = {0};

	switch (type)
	{
	case VULKAN_PIPELINE_TYPE_GRAPHICS:
	{
		result.type = VULKAN_PIPELINE_TYPE_GRAPHICS;
	} break;

	case VULKAN_PIPELINE_TYPE_COMPUTE:
	{
		result.type = VULKAN_PIPELINE_TYPE_COMPUTE;
	} break;

	default:
	{
		SDL_Log("[VULKAN] Warning: Pipeline description of unkwon type: %d", type);
	}
	}

	return result;
}

void vulkan_pipeline_desc_set_vertex_input(vulkan_pipeline_desc *desc, u32 vertex_stride, VkVertexInputAttributeDescription *attribues, u32 attribute_count)
{
	assert(desc);
	assert(attribues);

	assert(desc->type == VULKAN_PIPELINE_TYPE_GRAPHICS);

	VkVertexInputBindingDescription binding = {
		.binding = 0,
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		.stride = vertex_stride,	
	};

	desc->vertex_binding = binding;

	assert(attribute_count <= VULKAN_PIPELINE_DESC_MAX_VERTEX_ATTRIBUTES);

	desc->vertex_attribute_count = attribute_count;
	SDL_memcpy(desc->vertex_attributes, attribues, attribute_count * sizeof(VkVertexInputAttributeDescription));
}

void vulkan_pipeline_desc_add_storage_buffer(vulkan_pipeline_desc *desc, vulkan_context *ctx, vulkan_buffer buffer, u32 binding, VkShaderStageFlags stage)
{
	if (desc->descriptor_count + 1 > ARRAY_COUNT(desc->descriptors))
	{
		SDL_Log("[VULKAN] No space left to add descriptor to this pipeline (%u/%zu)", desc->descriptor_count, ARRAY_COUNT(desc->descriptors));
		return;
	}

	vulkan_descriptor descriptor;
	if (!vulkan_descriptor_storage_buffer_create(ctx, buffer, binding, stage, &descriptor))
	{
		return;
	}

	desc->descriptors[desc->descriptor_count] = descriptor;
	++desc->descriptor_count;
}

// TODO: Replace with relative to executable path or embed shader
static VkShaderModule shader_module_create(vulkan_context *ctx, const char *path)
{
	assert(ctx);
	assert(path);

	usize size;
	u8 *data = SDL_LoadFile(path, &size);
	assert(data);

	VkShaderModuleCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = size,
		.pCode = (u32 *)data,	
	};

	VkShaderModule result = {0};
	if (vkCreateShaderModule(ctx->device, &info, NULL, &result) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to create shader module: %s.", path);
		SDL_free(data);

		return result;
	}

	SDL_free(data);

	return result;
}

static bool pipeline_layout_create(vulkan_context *ctx, vulkan_pipeline_desc *desc, vulkan_pipeline *out_pipeline)
{
	VkPushConstantRange push_constant = {
		.offset = 0,
		.size = sizeof(m4),
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,	
	};

	VkDescriptorSetLayout descriptor_layouts[desc->descriptor_count];
	for (u32 i = 0; i < desc->descriptor_count; i++)
	{
		descriptor_layouts[i] = desc->descriptors[i].layout;
	}

	VkPipelineLayoutCreateInfo layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pPushConstantRanges = &push_constant,
		.pushConstantRangeCount = 1,
		.pSetLayouts = descriptor_layouts,
		.setLayoutCount = ARRAY_COUNT(descriptor_layouts),
	};

	if (vkCreatePipelineLayout(ctx->device, &layout_info, NULL, &out_pipeline->layout) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to create graphics pipeline layout.");
		return false;
	}

	SDL_memcpy(out_pipeline->descriptors, desc->descriptors, desc->descriptor_count * sizeof(vulkan_descriptor));
	out_pipeline->descriptor_count = desc->descriptor_count;
	out_pipeline->type = desc->type;

	return true;	
}

static bool graphics_pipeline_create(vulkan_context *ctx, vulkan_pipeline_desc *desc, vulkan_pipeline *out_pipeline)
{
	assert(ctx);
	assert(out_pipeline);

	VkShaderModule vertex_module = shader_module_create(ctx, "src/shaders/spv/shader.vert.spv");
	VkShaderModule fragment_module = shader_module_create(ctx, "src/shaders/spv/shader.frag.spv");

	VkPipelineShaderStageCreateInfo stages[2] = {
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vertex_module,
			.pName = "main",
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = fragment_module,
			.pName = "main",
		},
	};

	VkPipelineRenderingCreateInfo rendering = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = &ctx->swapchain.fmt,
	};

	VkPipelineVertexInputStateCreateInfo vertex_input = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &desc->vertex_binding,
		.vertexAttributeDescriptionCount = desc->vertex_attribute_count,
		.pVertexAttributeDescriptions = desc->vertex_attributes,
	};

	VkPipelineInputAssemblyStateCreateInfo assembly_input = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
	};

	VkPipelineViewportStateCreateInfo viewport = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1,	
	};

	VkPipelineRasterizationStateCreateInfo rasterization = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.lineWidth = 1.0f,
		.depthClampEnable = VK_FALSE,
		.depthBiasEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
	};

	VkPipelineMultisampleStateCreateInfo multisample = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,	
	};

	VkPipelineDepthStencilStateCreateInfo depth_stencil = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthTestEnable = VK_FALSE,
		.depthWriteEnable = VK_FALSE,
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f,	
	};
	
    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
    };

	VkPipelineColorBlendStateCreateInfo color_blend = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pAttachments = &color_blend_attachment,
		.attachmentCount = 1,
	};

	VkDynamicState dynamic_states[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	VkPipelineDynamicStateCreateInfo dynamic = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = ARRAY_COUNT(dynamic_states),
		.pDynamicStates = dynamic_states,	
	};

	VkGraphicsPipelineCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pStages = stages,
		.stageCount = ARRAY_COUNT(stages),
		.pNext = &rendering,
		.pVertexInputState = &vertex_input,
		.pInputAssemblyState = &assembly_input,
		.pViewportState = &viewport,
		.pRasterizationState = &rasterization,
		.pMultisampleState = &multisample,
		.pDepthStencilState = &depth_stencil,
		.pColorBlendState = &color_blend,
		.pDynamicState = &dynamic,
		.layout = out_pipeline->layout,
	};

	if (vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, &info, NULL, &out_pipeline->handle) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to create graphics pipeline.");
		vkDestroyShaderModule(ctx->device, vertex_module, NULL);
		vkDestroyShaderModule(ctx->device, fragment_module, NULL);
		return false;
	}

	vkDestroyShaderModule(ctx->device, vertex_module, NULL);
	vkDestroyShaderModule(ctx->device, fragment_module, NULL);

	return true;
}

static bool compute_pipeline_create(vulkan_context *ctx, vulkan_pipeline_desc *desc, vulkan_pipeline *out_pipeline)
{
	// TODO: Not hardcode shader
	VkShaderModule shader_module = shader_module_create(ctx, "src/shaders/spv/update.comp.spv");

	VkPipelineShaderStageCreateInfo stage = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = shader_module,
			.pName = "main",
	};

	VkComputePipelineCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.layout = out_pipeline->layout,
		.stage = stage,
	};

	if (vkCreateComputePipelines(ctx->device, VK_NULL_HANDLE, 1, &info, NULL, &out_pipeline->handle) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to create compute pipeline.");
		vkDestroyShaderModule(ctx->device, shader_module, NULL);

		return false;
	}
	
	vkDestroyShaderModule(ctx->device, shader_module, NULL);

	return true;
}

static void pipeline_destroy(vulkan_context *ctx, vulkan_pipeline *pipeline)
{
	assert(ctx);
	assert(pipeline);

	vkDestroyPipelineLayout(ctx->device, pipeline->layout, NULL);
	vkDestroyPipeline(ctx->device, pipeline->handle, NULL);

	for (u32 i = 0; i < pipeline->descriptor_count; i++)
	{
		vulkan_descriptor_destroy(ctx, &pipeline->descriptors[i]);
	}
}

bool vulkan_pipeline_manager_create(vulkan_pipeline_manager *manager)
{
	assert(manager);

	manager->count = 0;

	return true;
}

void vulkan_pipeline_manager_destroy(vulkan_context *ctx, vulkan_pipeline_manager *manager)
{
	assert(ctx);
	assert(manager);

	for (u32 i = 0; i < manager->count; i++)
	{
		pipeline_destroy(ctx, &manager->pipelines[i]);
	}
}

vulkan_pipeline_id vulkan_pipeline_create(vulkan_context *ctx, vulkan_pipeline_desc *desc)
{
	assert(ctx);
	assert(desc);

	vulkan_pipeline_manager *manager = &ctx->pipeline_manager;

	if (manager->count + 1 > ARRAY_COUNT(manager->pipelines))
	{
		SDL_Log("[VULKAN] Failed to create pipeline no space left (%u/%zu)", manager->count, ARRAY_COUNT(manager->pipelines));
		return INVALID_PIPELINE;
	}

	if (!pipeline_layout_create(ctx, desc, &manager->pipelines[manager->count]))
	{
		return INVALID_PIPELINE;
	}

	pipeline_create_func pipeline_create = desc->type == VULKAN_PIPELINE_TYPE_GRAPHICS ? graphics_pipeline_create : compute_pipeline_create;
	if (!pipeline_create(ctx, desc, &manager->pipelines[manager->count]))
	{
		return INVALID_PIPELINE;
	}

	manager->count++;

	return manager->count;
}

vulkan_pipeline *vulkan_pipeline_get(vulkan_context *ctx, vulkan_pipeline_id id)
{
	assert(ctx);
	assert(id != INVALID_PIPELINE);

	vulkan_pipeline_manager *manager = &ctx->pipeline_manager;
	
	vulkan_pipeline *result = &manager->pipelines[id - 1];
	assert(result);

	return result;
}
