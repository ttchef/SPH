
#include <vk/command.h>
#include <vk/context.h>
#include <vk/buffer.h>

#include <SDL3/SDL_log.h>
#include <vulkan/vulkan_core.h>

#include <math/matrix.h>

typedef enum vulkan_render_command_type
{
	COMMAND_BEGIN_RENDERING,
	COMMAND_END_RENDERING,
	// NOTE: Automatically bind every discriptor set assoisiated with the pipeline
	COMMAND_BIND_PIPELINE,
	COMMAND_BIND_VERTEX_BUFFER,
	COMMAND_PUSH_CONSTANTS,
	// NOTE: vkCmdDraw, uses currently bound pipeline 
	COMMAND_DRAW,
	COMMAND_DISPATCH,
} vulkan_render_command_type;

typedef struct command_header
{
	u32 type;
	u32 size;
} command_header;

typedef struct command_begin_rendering
{
	command_header header;
} command_begin_rendering;

typedef struct command_end_rendering
{
	command_header header;
} command_end_rendering;

typedef struct command_bind_pipeline
{
	command_header header;
	vulkan_pipeline_id id;
} command_bind_pipeline;

typedef struct command_bind_vertex_buffer
{
	command_header header;
	vulkan_buffer buffer;
	vulkan_pipeline_id pipeline;
} command_bind_vertex_buffer;

typedef struct command_push_constants
{
	command_header header;
	u32 size;
	void *data;
	VkShaderStageFlags stage;
	vulkan_pipeline_id pipeline;
} command_push_constants;

typedef struct command_draw
{
	command_header header;
	u32 vertex_count;
} command_draw;

typedef struct command_dispatch
{
	command_header header;
	u32 size_x;
	u32 size_y;
	u32 size_z;
} command_dispatch;

static bool command_add(vulkan_context *ctx, void *data, u32 size)
{
	assert(ctx);
	
	vulkan_command_queue *queue = &ctx->command_handler.render_commands;

	if (size > queue->available)
	{
		SDL_Log("[VULKAN] Not enough space for render command.");
		return false;
	}

	SDL_memcpy(queue->at, data, size);
	queue->at = (u8 *)queue->at + size;
	queue->available -= size;

	return true;
}

bool vulkan_command_begin_rendering(vulkan_context *ctx)
{
	command_header header = {
		.type = COMMAND_BEGIN_RENDERING,
		.size = sizeof(command_begin_rendering),	
	};

	command_begin_rendering begin_rendering = {
		.header = header,	
	};

	return command_add(ctx, &begin_rendering, header.size);
}

bool vulkan_command_end_rendering(vulkan_context *ctx)
{
	command_header header = {
		.type = COMMAND_END_RENDERING,
		.size = sizeof(command_end_rendering),	
	};

	command_begin_rendering end_rendering = {
		.header = header,	
	};

	return command_add(ctx, &end_rendering, header.size);
}

bool vulkan_command_bind_pipeline(vulkan_context *ctx, vulkan_pipeline_id id)
{
	assert(id != INVALID_PIPELINE);

	command_header header = {
		.type = COMMAND_BIND_PIPELINE,
		.size = sizeof(command_bind_pipeline),
	};

	command_bind_pipeline bind_pipeline = {
		.header	= header,
		.id = id,
	};

	return command_add(ctx, &bind_pipeline, header.size);
}

bool vulkan_command_bind_vertex_buffer(vulkan_context *ctx, vulkan_buffer buffer, vulkan_pipeline_id pipeline)
{
	assert(ctx);

	command_header header = {
		.type = COMMAND_BIND_VERTEX_BUFFER,
		.size = sizeof(command_bind_vertex_buffer),
	};

	command_bind_vertex_buffer bind_vertex_buffer = {
		.header	= header,
		.buffer = buffer,
		.pipeline = pipeline,
	};

	return command_add(ctx, &bind_vertex_buffer, header.size);
}

bool vulkan_command_push_constants(vulkan_context *ctx, u32 size, void *data, VkShaderStageFlags stage, vulkan_pipeline_id pipeline)
{
	assert(ctx);
	assert(data);

	command_header header = {
		.type = COMMAND_PUSH_CONSTANTS,
		.size = sizeof(command_push_constants),
	};

	command_push_constants push_constants = {
		.header	= header,
		.size = size,
		.data = data,
		.stage = stage,
		.pipeline = pipeline,
	};

	return command_add(ctx, &push_constants, header.size);
}

bool vulkan_command_draw(vulkan_context *ctx, u32 vertex_count)
{
	assert(ctx);

	command_header header = {
		.type = COMMAND_DRAW,
		.size = sizeof(command_draw),
	};

	command_draw draw = {
		.header	= header,
		.vertex_count = vertex_count,
	};

	return command_add(ctx, &draw, header.size);
}

bool vulkan_command_dispatch(vulkan_context *ctx, u32 size_x, u32 size_y, u32 size_z)
{
	assert(ctx);

	command_header header = {
		.type = COMMAND_DISPATCH,
		.size = sizeof(command_dispatch),
	};

	command_dispatch dispatch = {
		.header	= header,
		.size_x = size_x,
		.size_y = size_y,
		.size_z = size_z,
	};

	return command_add(ctx, &dispatch, header.size);
}

bool vulkan_command_handler_create(vulkan_context *ctx, vulkan_command_handler *handler)
{
	assert(ctx);
	assert(handler);

	handler->frame_index = 0;
	handler->accumulated_frame_index = 0;

	VkCommandPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = ctx->graphics_queue.index,
	};

	if (vkCreateCommandPool(ctx->device, &pool_info, NULL, &handler->command_pool) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to create commnad pool.");
		return false;
	}

	VkSemaphoreCreateInfo semaphore_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,	
	};

	VkFenceCreateInfo fence_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,	
	};

	for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		VkCommandBufferAllocateInfo alloc_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = handler->command_pool,
			.commandBufferCount = 1,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,	
		};

		if (vkAllocateCommandBuffers(ctx->device, &alloc_info, &handler->frame_data[i].command_buffer) != VK_SUCCESS)
		{
			SDL_Log("[VULKAN] Failed to allocate command buffer.");
			return false;			
		}

		if (vkCreateSemaphore(ctx->device, &semaphore_info, NULL, &handler->frame_data[i].image_available) != VK_SUCCESS)
		{
			SDL_Log("[VULKAN] Failed to create image finished semaphore.");
			return false;
		}

		if (vkCreateFence(ctx->device, &fence_info, NULL, &handler->frame_data[i].in_flight_fence) != VK_SUCCESS)
		{
			return false;
		}
	}

	handler->render_commands.size = KILOBYTES(100);
	handler->render_commands.available = handler->render_commands.size;
	handler->render_commands.base = SDL_calloc(handler->render_commands.size, 1);
	assert(handler->render_commands.base);
	handler->render_commands.at = handler->render_commands.base;

	return true;
}

static void render_queue(vulkan_context *ctx)
{
	assert(ctx);

	vulkan_command_handler *handler = &ctx->command_handler;
	vulkan_command_queue *queue = &handler->render_commands;

	void *at = queue->base;

	vulkan_frame_data *frame_data = &handler->frame_data[handler->frame_index];
	assert(frame_data);

	while (at < queue->at)
	{
		command_header *header = (command_header *)at;
		assert(header);

		switch (header->type)
		{
		case COMMAND_BEGIN_RENDERING:
		{
		    VkClearValue clear_color = {
		        .color = {{0.0f, 0.0f, 0.0f, 0.0f}},
		    };

		    VkRenderingAttachmentInfo color_attachment_info = {
		        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		        .imageView = ctx->swapchain.image_views[ctx->swapchain.image_index],
		        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		        .clearValue = clear_color,
		    };

		    VkRenderingInfo render_info = {
		    	.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		    	.layerCount = 1,
		    	.colorAttachmentCount = 1,
		    	.pColorAttachments = &color_attachment_info,
		    	.renderArea =
		    	{
		    		.extent = ctx->swapchain.extent,
		    		.offset = (VkOffset2D){0, 0},
		    	},	
		    };

		    vkCmdBeginRendering(frame_data->command_buffer, &render_info);
		} break;
		case COMMAND_END_RENDERING:
		{
    		vkCmdEndRendering(frame_data->command_buffer);
		} break;
		case COMMAND_BIND_PIPELINE:
		{
			command_bind_pipeline *bind_pipeline = at;

			vulkan_pipeline *pipeline = vulkan_pipeline_get(ctx, bind_pipeline->id);
			assert(pipeline);

			VkPipelineBindPoint bind_point = pipeline->type == VULKAN_PIPELINE_TYPE_GRAPHICS ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE;
			vkCmdBindPipeline(frame_data->command_buffer, bind_point, pipeline->handle);

			for (u32 i = 0; i < pipeline->descriptor_count; i++)
			{
				vkCmdBindDescriptorSets(frame_data->command_buffer, bind_point, pipeline->layout, 0, 1, &pipeline->descriptors[i].set, 0, NULL);
			}
		} break;
		case COMMAND_BIND_VERTEX_BUFFER:
		{
			command_bind_vertex_buffer *bind_vertex_buffer = at;

			vulkan_pipeline *pipeline = vulkan_pipeline_get(ctx, bind_vertex_buffer->pipeline);
			assert(pipeline);

			VkDeviceSize offset = {0};
			vkCmdBindVertexBuffers(frame_data->command_buffer, 0, 1, &bind_vertex_buffer->buffer.handle, &offset);
		} break;
		case COMMAND_PUSH_CONSTANTS:
		{
			command_push_constants *push_constants = at;

			vulkan_pipeline *pipeline = vulkan_pipeline_get(ctx, push_constants->pipeline);
			assert(pipeline);

			vkCmdPushConstants(frame_data->command_buffer, pipeline->layout, push_constants->stage, 0, push_constants->size, push_constants->data);
		} break;
		case COMMAND_DRAW:
		{
			command_draw *draw = at;

			vkCmdDraw(frame_data->command_buffer, draw->vertex_count, 1, 0, 0);
		} break;
		case COMMAND_DISPATCH:
		{
			command_dispatch *dispatch = at;

			vkCmdDispatch(frame_data->command_buffer, dispatch->size_x, dispatch->size_y, dispatch->size_z);
		} break;
		default:
		{
			SDL_Log("[VULKAN] Unkown render command of type: %u", header->type);
		}
		}	

		at = (u8 *)at + header->size;
	}

	queue->at = queue->base;
	queue->available = queue->size;
}

// TODO: not hardcode vertex buffer
bool vulkan_command_handler_record(vulkan_context *ctx, vulkan_command_handler *handler)
{
	assert(ctx);
	assert(handler);

	vulkan_frame_data *frame_data = &handler->frame_data[handler->frame_index];
	assert(frame_data);

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	
	};

	if (vkBeginCommandBuffer(frame_data->command_buffer, &begin_info) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to begin recording into command buffer.");
		return false;
	}

    VkImageMemoryBarrier memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = ctx->swapchain.images[ctx->swapchain.image_index],
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount = 1,
                .levelCount = 1,
            },
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    vkCmdPipelineBarrier(frame_data->command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0,
                         NULL, 0, NULL, 1, &memory_barrier);

    VkViewport viewport = {
    	.width = ctx->swapchain.extent.width,
    	.height = ctx->swapchain.extent.height,
    	.maxDepth = 1.0f,	
    };

    VkRect2D scissor = {
    	.extent = ctx->swapchain.extent,
    	.offset = (VkOffset2D){0, 0},	
    };

    vkCmdSetViewport(frame_data->command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(frame_data->command_buffer, 0, 1, &scissor);

    render_queue(ctx);

    memory_barrier = (VkImageMemoryBarrier){
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image = ctx->swapchain.images[ctx->swapchain.image_index],
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount = 1,
                .levelCount = 1,
            },
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    vkCmdPipelineBarrier(frame_data->command_buffer,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0,
                         NULL, 1, &memory_barrier);
    if (vkEndCommandBuffer(frame_data->command_buffer) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to end recording into command buffer.");
		return false;
	}

	return true;	
}

void vulkan_command_handler_destroy(vulkan_context *ctx, vulkan_command_handler *handler)
{
	assert(ctx);
	assert(handler);

	SDL_free(ctx->command_handler.render_commands.base);

	vkDestroyCommandPool(ctx->device, handler->command_pool, NULL);

	for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(ctx->device, handler->frame_data[i].image_available, NULL);
		vkDestroyFence(ctx->device, handler->frame_data[i].in_flight_fence, NULL);
	}
}
