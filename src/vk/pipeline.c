
#include <vk/pipeline.h>
#include <vk/context.h>
#include <math/types.h>

#include <SDL3/SDL_log.h>
#include <vulkan/vulkan_core.h>

// TODO: Replace with relative to executable path or embed shader
static VkShaderModule shader_module_init(vulkan_context *ctx, const char *path)
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

bool vulkan_pipeline_create(vulkan_context *ctx, vulkan_pipeline_desc *desc, vulkan_pipeline *out_pipeline)
{
	assert(ctx);
	assert(out_pipeline);

	VkShaderModule vertex_module = shader_module_init(ctx, "src/shaders/spv/shader.vert.spv");
	VkShaderModule fragment_module = shader_module_init(ctx, "src/shaders/spv/shader.frag.spv");

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

	VkVertexInputBindingDescription binding = {
		.binding = 0,
		.stride = sizeof(f32) * 6,
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};

	VkVertexInputAttributeDescription attributes[] = {
		// NOTE: Position
		{
			.binding = 0,
			.location = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = 0,
		},
		// NOTE: Velocity
		{
			.binding = 0,
			.location = 1,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = sizeof(f32) * 2,
		},
		// NOTE: Mass
		{
			.binding = 0,
			.location = 2,
			.format = VK_FORMAT_R32_SFLOAT,
			.offset = sizeof(f32) * 4,
		},
		// NOTE: Density
		{
			.binding = 0,
			.location = 3,
			.format = VK_FORMAT_R32_SFLOAT,
			.offset = sizeof(f32) * 5,
		},	
	};

	VkPipelineVertexInputStateCreateInfo vertex_input = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &binding,
		.vertexAttributeDescriptionCount = ARRAY_COUNT(attributes),
		.pVertexAttributeDescriptions = attributes,
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

	VkPushConstantRange push_constant = {
		.offset = 0,
		.size = sizeof(m4),
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,	
	};

	VkPipelineLayoutCreateInfo layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pPushConstantRanges = &push_constant,
		.pushConstantRangeCount = 1,
	};

	if (vkCreatePipelineLayout(ctx->device, &layout_info, NULL, &out_pipeline->layout) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to create graphics pipeline layout.");
		vkDestroyShaderModule(ctx->device, vertex_module, NULL);
		vkDestroyShaderModule(ctx->device, fragment_module, NULL);
		return false;
	}

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

	if (vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, &info, NULL, &ctx->triangle_pipeline.handle) != VK_SUCCESS)
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

void vulkan_pipeline_destroy(vulkan_context *ctx, vulkan_pipeline *pipeline)
{
	assert(ctx);
	assert(pipeline);

	vkDestroyPipelineLayout(ctx->device, pipeline->layout, NULL);
	vkDestroyPipeline(ctx->device, pipeline->handle, NULL);
}
