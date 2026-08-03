
#include <vk/buffer.h>
#include <vk/command.h>
#include <vk/context.h>

#include <SDL3/SDL_log.h>
#include <vulkan/vulkan_core.h>

#include <math/matrix.h>

typedef enum
{
    COMMAND_BARRIER,
    COMMAND_IMAGE_BARRIER,
    COMMAND_BEGIN_RENDERING,
    COMMAND_END_RENDERING,
    // NOTE: Automatically bind every discriptor set assoisiated with the pipeline
    COMMAND_BIND_PIPELINE,
    COMMAND_BIND_VERTEX_BUFFER,
    COMMAND_PUSH_CONSTANTS,
    // NOTE: vkCmdDraw, uses currently bound pipeline
    COMMAND_DRAW,
    COMMAND_DISPATCH,
    COMMAND_SET_VIEWPORT,
    COMMAND_COPY_BUFFER,
    COMMAND_COPY_IMAGE,

#if defined(DEBUG)
    COMMAND_LABEL_BEGIN,
    COMMAND_LABEL_END,
#endif
} vulkan_render_command_type;

typedef struct
{
    u32 type;
    u32 size;
} command_header;

typedef struct
{
    command_header header;

    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;
    VkAccessFlags        src_access;
    VkAccessFlags        dst_access;
} command_barrier;

typedef struct
{
    command_header       header;
    vulkan_image         image;
    VkImageLayout        new_layout;
    VkAccessFlagBits     new_access;
    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;
} command_image_barrier;

typedef struct
{
    command_header header;
} command_begin_rendering;

typedef struct
{
    command_header header;
} command_end_rendering;

typedef struct
{
    command_header     header;
    vulkan_pipeline_id id;
} command_bind_pipeline;

typedef struct
{
    command_header     header;
    vulkan_buffer      buffer;
    vulkan_pipeline_id pipeline;
} command_bind_vertex_buffer;

typedef struct
{
    command_header     header;
    u32                size;
    VkShaderStageFlags stage;
    vulkan_pipeline_id pipeline;

    // NOTE: push constant data follows in memory
} command_push_constants;

typedef struct
{
    command_header header;
    u32            vertex_count;
} command_draw;

typedef struct
{
    command_header header;
    u32            size_x;
    u32            size_y;
    u32            size_z;
} command_dispatch;

typedef struct
{
    command_header header;
    f32            x;
    f32            y;
    f32            width;
    f32            height;
} command_set_viewport;

typedef struct
{
    command_header header;
    vulkan_buffer  src;
    vulkan_buffer  dst;
} command_copy_buffer;

typedef struct
{
    command_header header;
    vulkan_buffer  src;
    vulkan_image   dst;
    VkImageLayout  new_layout;
} command_copy_image;

#if defined(DEBUG)

typedef struct
{
    command_header header;
    const char    *name;
    color4         color;
} command_label_begin;

typedef struct
{
    command_header header;
} command_label_end;

#endif

static bool command_add(vulkan_command_queue *queue, void *data, u32 size)
{
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

bool vulkan_command_barrier(vulkan_command_queue *queue, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, VkAccessFlags src_access, VkAccessFlags dst_access)
{
    command_header header = {
        .type = COMMAND_BARRIER,
        .size = sizeof(command_barrier),
    };

    command_barrier barrier = {
        .header     = header,
        .src_stage  = src_stage,
        .dst_stage  = dst_stage,
        .src_access = src_access,
        .dst_access = dst_access,
    };

    return command_add(queue, &barrier, header.size);
}

bool vulkan_command_image_barrier(vulkan_command_queue *queue, vulkan_image image, VkImageLayout new_layout, VkAccessFlagBits new_access, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage)
{
    command_header header = {
        .type = COMMAND_IMAGE_BARRIER,
        .size = sizeof(command_image_barrier),
    };

    command_image_barrier image_barrier = {
        .header     = header,
        .image      = image,
        .new_layout = new_layout,
        .new_access = new_access,
        .src_stage  = src_stage,
        .dst_stage  = dst_stage,
    };

    return command_add(queue, &image_barrier, header.size);
}

bool vulkan_command_begin_rendering(vulkan_command_queue *queue)
{
    command_header header = {
        .type = COMMAND_BEGIN_RENDERING,
        .size = sizeof(command_begin_rendering),
    };

    command_begin_rendering begin_rendering = {
        .header = header,
    };

    return command_add(queue, &begin_rendering, header.size);
}

bool vulkan_command_end_rendering(vulkan_command_queue *queue)
{
    command_header header = {
        .type = COMMAND_END_RENDERING,
        .size = sizeof(command_end_rendering),
    };

    command_end_rendering end_rendering = {
        .header = header,
    };

    return command_add(queue, &end_rendering, header.size);
}

bool vulkan_command_bind_pipeline(vulkan_command_queue *queue, vulkan_pipeline_id id)
{
    assert(id != VULKAN_INVALID_PIPELINE);

    command_header header = {
        .type = COMMAND_BIND_PIPELINE,
        .size = sizeof(command_bind_pipeline),
    };

    command_bind_pipeline bind_pipeline = {
        .header = header,
        .id     = id,
    };

    return command_add(queue, &bind_pipeline, header.size);
}

bool vulkan_command_bind_vertex_buffer(vulkan_command_queue *queue, vulkan_buffer buffer, vulkan_pipeline_id pipeline)
{
    command_header header = {
        .type = COMMAND_BIND_VERTEX_BUFFER,
        .size = sizeof(command_bind_vertex_buffer),
    };

    command_bind_vertex_buffer bind_vertex_buffer = {
        .header   = header,
        .buffer   = buffer,
        .pipeline = pipeline,
    };

    return command_add(queue, &bind_vertex_buffer, header.size);
}

bool vulkan_command_push_constants(vulkan_command_queue *queue, u32 size, void *data, VkShaderStageFlags stage, vulkan_pipeline_id pipeline)
{
    command_header header = {
        .type = COMMAND_PUSH_CONSTANTS,
        .size = sizeof(command_push_constants) + size,
    };

    command_push_constants push_constants = {
        .header   = header,
        .size     = size,
        .stage    = stage,
        .pipeline = pipeline,
    };

    if (header.size > queue->available)
    {
        SDL_Log("[VULKAN] Not enough space for render command.");
        return false;
    }

    SDL_memcpy(queue->at, &push_constants, sizeof(push_constants));
    SDL_memcpy((u8 *)queue->at + sizeof(push_constants), data, size);

    queue->at = (u8 *)queue->at + header.size;
    queue->available -= header.size;

    return true;
}

bool vulkan_command_draw(vulkan_command_queue *queue, u32 vertex_count)
{
    command_header header = {
        .type = COMMAND_DRAW,
        .size = sizeof(command_draw),
    };

    command_draw draw = {
        .header       = header,
        .vertex_count = vertex_count,
    };

    return command_add(queue, &draw, header.size);
}

bool vulkan_command_dispatch(vulkan_command_queue *queue, u32 size_x, u32 size_y, u32 size_z)
{
    command_header header = {
        .type = COMMAND_DISPATCH,
        .size = sizeof(command_dispatch),
    };

    command_dispatch dispatch = {
        .header = header,
        .size_x = size_x,
        .size_y = size_y,
        .size_z = size_z,
    };

    return command_add(queue, &dispatch, header.size);
}

bool vulkan_command_set_viewport(vulkan_command_queue *queue, f32 x, f32 y, f32 width, f32 height)
{
    command_header header = {
        .type = COMMAND_SET_VIEWPORT,
        .size = sizeof(command_set_viewport),
    };

    command_set_viewport set_viewport = {
        .header = header,
        .x      = x,
        .y      = y,
        .width  = width,
        .height = height,
    };

    return command_add(queue, &set_viewport, header.size);
}

bool vulkan_command_copy_buffer(vulkan_command_queue *queue, vulkan_buffer src, vulkan_buffer dst)
{
    command_header header = {
        .type = COMMAND_COPY_BUFFER,
        .size = sizeof(command_copy_buffer),
    };

    command_copy_buffer copy_buffer = {
        .header = header,
        .src    = src,
        .dst    = dst,
    };

    return command_add(queue, &copy_buffer, header.size);
}

bool vulkan_command_copy_image(vulkan_command_queue *queue, vulkan_buffer src, vulkan_image dst, VkImageLayout new_layout)
{
    command_header header = {
        .type = COMMAND_COPY_IMAGE,
        .size = sizeof(command_copy_image),
    };

    command_copy_image copy_image = {
        .header     = header,
        .src        = src,
        .dst        = dst,
        .new_layout = new_layout,
    };

    return command_add(queue, &copy_image, header.size);
}

#if defined(DEBUG)

bool vulkan_command_label_begin(vulkan_command_queue *queue, const char *name, color4 color)
{
    command_header header = {
        .type = COMMAND_LABEL_BEGIN,
        .size = sizeof(command_label_begin),
    };

    command_label_begin label_begin = {
        .header = header,
        .name   = name,
        .color  = color,
    };

    return command_add(queue, &label_begin, header.size);
}

bool vulkan_command_label_end(vulkan_command_queue *queue)
{
    command_header header = {
        .type = COMMAND_LABEL_END,
        .size = sizeof(command_label_end),
    };

    command_label_end label_end = {
        .header = header,
    };

    return command_add(queue, &label_end, header.size);
}

#endif // DEBUG

bool vulkan_command_handler_create(vulkan *vulkan, vulkan_command_handler *out_handler)
{
    vulkan_command_handler result = {0};

    result.frame_index             = 0;
    result.accumulated_frame_index = 0;

    VkCommandPoolCreateInfo pool_info = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = vulkan->graphics_queue.index,
    };

    if (vkCreateCommandPool(vulkan->device, &pool_info, NULL, &result.command_pool) != VK_SUCCESS)
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
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = result.command_pool,
            .commandBufferCount = 1,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        };

        if (vkAllocateCommandBuffers(vulkan->device, &alloc_info, &result.frame_data[i].command_buffer) != VK_SUCCESS)
        {
            SDL_Log("[VULKAN] Failed to allocate command buffer.");
            return false;
        }

        if (vkCreateSemaphore(vulkan->device, &semaphore_info, NULL, &result.frame_data[i].image_available) != VK_SUCCESS)
        {
            SDL_Log("[VULKAN] Failed to create image finished semaphore.");
            return false;
        }

        if (vkCreateFence(vulkan->device, &fence_info, NULL, &result.frame_data[i].in_flight_fence) != VK_SUCCESS)
        {
            return false;
        }
    }

    for (u32 i = 0; i < ARRAY_COUNT(result.contexts); i++)
    {
        vulkan_immeadiate_context *ctx = &result.contexts[i];

        VkCommandPoolCreateInfo pool_info = {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = vulkan->graphics_queue.index,
            .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        };

        if (vkCreateCommandPool(vulkan->device, &pool_info, NULL, &ctx->pool) != VK_SUCCESS)
        {
            SDL_Log("[VULKAN] Failed to create immeadiate pool.");
            return false;
        }

        VkCommandBufferAllocateInfo alloc_info = {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = ctx->pool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        if (vkAllocateCommandBuffers(vulkan->device, &alloc_info, &ctx->buffer) != VK_SUCCESS)
        {
            SDL_Log("[VULKAN] Failed to create immeadiate command buffer.");
            return false;
        }

        VkSemaphoreTypeCreateInfo type_info = {
            .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
            .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
            .initialValue  = 0,
        };

        VkSemaphoreCreateInfo semaphore_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = &type_info,
        };

        if (vkCreateSemaphore(vulkan->device, &semaphore_info, NULL, &ctx->timeline) != VK_SUCCESS)
        {
            SDL_Log("[VULKAN] Failed to create immeadiate semaphore.");
            return false;
        }

        ctx->value = 0;
    }

    *out_handler = result;

    return true;
}

vulkan_immeadiate_context *vulkan_command_handler_immeadiate_get(vulkan *vulkan, vulkan_command_handler *handler)
{
    vulkan_immeadiate_context *ctx = &handler->contexts[handler->next_context];
    handler->next_context          = (handler->next_context + 1) % ARRAY_COUNT(handler->contexts);

    u64 current;
    vkGetSemaphoreCounterValue(vulkan->device, ctx->timeline, &current);
    if (current < ctx->value)
    {
        VkSemaphoreWaitInfo wait_info = {
            .sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .semaphoreCount = 1,
            .pSemaphores    = &ctx->timeline,
            .pValues        = &ctx->value,
        };

        vkWaitSemaphores(vulkan->device, &wait_info, UINT64_MAX);
    }

    vkResetCommandBuffer(ctx->buffer, 0);
    return ctx;
}

static void execute_queue(vulkan *vulkan, vulkan_command_queue *queue, VkCommandBuffer command_buffer)
{
    void *at = queue->base;

    while (at < queue->at)
    {
        command_header *header = (command_header *)at;
        assert(header);

        switch (header->type)
        {
        case COMMAND_BARRIER:
        {
            command_barrier *barrier = at;

            VkMemoryBarrier2 memory_barrier = {
                .sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
                .srcAccessMask = barrier->src_access,
                .dstAccessMask = barrier->dst_access,
                .srcStageMask  = barrier->src_stage,
                .dstStageMask  = barrier->dst_stage,
            };

            VkDependencyInfo dep_info = {
                .sType              = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .memoryBarrierCount = 1,
                .pMemoryBarriers    = &memory_barrier,
            };

            vkCmdPipelineBarrier2(command_buffer, &dep_info);
        }
        break;
        case COMMAND_IMAGE_BARRIER:
        {
            command_image_barrier *image_barrier = at;

            VkImageMemoryBarrier2 barrier = {
                .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .image            = image_barrier->image.handle,
                .oldLayout        = image_barrier->image.layout,
                .newLayout        = image_barrier->new_layout,
                .srcAccessMask    = image_barrier->image.access,
                .dstAccessMask    = image_barrier->new_access,
                .srcStageMask     = image_barrier->src_stage,
                .dstStageMask     = image_barrier->dst_stage,
                .subresourceRange = {
                    .aspectMask = image_barrier->image.aspect,
                    .levelCount = 1,
                    .layerCount = 1,
                },
            };

            VkDependencyInfo dep_info = {
                .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers    = &barrier,
            };

            vkCmdPipelineBarrier2(command_buffer, &dep_info);
        }
        break;
        case COMMAND_BEGIN_RENDERING:
        {
            VkClearValue clear_color = {
                .color = {{0.0f, 0.0f, 0.0f, 0.0f}},
            };

            VkRenderingAttachmentInfo color_attachment_info = {
                .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView   = vulkan->swapchain.image_views[vulkan->swapchain.image_index],
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue  = clear_color,
            };

            VkRenderingAttachmentInfo depth_attachment_info = {
                .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView   = vulkan->swapchain.depth_images[vulkan->swapchain.image_index].view,
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue  = {
                    .depthStencil = {1.0f, 0.0f},
                },
            };

            VkRenderingInfo render_info = {
                .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
                .layerCount           = 1,
                .colorAttachmentCount = 1,
                .pColorAttachments    = &color_attachment_info,
                .pDepthAttachment     = &depth_attachment_info,
                .renderArea =
                    {
                        .extent = vulkan->swapchain.extent,
                        .offset = (VkOffset2D){0, 0},
                    },
            };

            vkCmdBeginRendering(command_buffer, &render_info);
        }
        break;
        case COMMAND_END_RENDERING:
        {
            vkCmdEndRendering(command_buffer);
        }
        break;
        case COMMAND_BIND_PIPELINE:
        {
            command_bind_pipeline *bind_pipeline = at;

            vulkan_pipeline *pipeline = vulkan_pipeline_get(vulkan, bind_pipeline->id);
            assert(pipeline);

            VkPipelineBindPoint bind_point = pipeline->type == VULKAN_PIPELINE_TYPE_GRAPHICS ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE;
            vkCmdBindPipeline(command_buffer, bind_point, pipeline->handle);

            // NOTE: Bind the bindless descriptor set
            vkCmdBindDescriptorSets(command_buffer, bind_point, pipeline->layout, 0, 1, &vulkan->bindless.set, 0, NULL);
        }
        break;
        case COMMAND_BIND_VERTEX_BUFFER:
        {
            command_bind_vertex_buffer *bind_vertex_buffer = at;

            VkDeviceSize offset = {0};
            vkCmdBindVertexBuffers(command_buffer, 0, 1, &bind_vertex_buffer->buffer.handle, &offset);
        }
        break;
        case COMMAND_PUSH_CONSTANTS:
        {
            command_push_constants *push_constants = at;

            vulkan_pipeline *pipeline = vulkan_pipeline_get(vulkan, push_constants->pipeline);
            assert(pipeline);

            void *data = (u8 *)at + sizeof(command_push_constants);

            vkCmdPushConstants(command_buffer, pipeline->layout, push_constants->stage, 0, push_constants->size, data);
        }
        break;
        case COMMAND_DRAW:
        {
            command_draw *draw = at;

            vkCmdDraw(command_buffer, draw->vertex_count, 1, 0, 0);
        }
        break;
        case COMMAND_DISPATCH:
        {
            command_dispatch *dispatch = at;

            vkCmdDispatch(command_buffer, dispatch->size_x, dispatch->size_y, dispatch->size_z);
        }
        break;
        case COMMAND_SET_VIEWPORT:
        {
            command_set_viewport *set_viewport = at;

            VkViewport viewport = {
                .x        = set_viewport->x,
                .y        = set_viewport->y,
                .width    = set_viewport->width,
                .height   = set_viewport->height,
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
            };

            vkCmdSetViewport(command_buffer, 0, 1, &viewport);
        }
        break;
        case COMMAND_COPY_BUFFER:
        {
            command_copy_buffer *copy_buffer = at;

            assert(copy_buffer->src.size == copy_buffer->dst.size);

            VkBufferCopy region = {
                .size = copy_buffer->src.size,
            };

            vkCmdCopyBuffer(command_buffer, copy_buffer->src.handle, copy_buffer->dst.handle, 1, &region);
        }
        break;
        case COMMAND_COPY_IMAGE:
        {
            command_copy_image *copy_image = at;

            VkBufferImageCopy region = {
                .imageSubresource = {
                    .aspectMask = copy_image->dst.aspect,
                    .layerCount = 1,
                },
                .imageExtent = {
                    .width  = copy_image->dst.width,
                    .height = copy_image->dst.height,
                    .depth  = 1,
                },
            };

            vkCmdCopyBufferToImage(command_buffer, copy_image->src.handle, copy_image->dst.handle, copy_image->new_layout, 1, &region);
        }
        break;

#if defined(DEBUG)
        case COMMAND_LABEL_BEGIN:
        {
            if (!vulkan->debug.vkCmdBeginDebugUtilsLabelEXT)
            {
                break;
            }

            command_label_begin *label_begin = at;

            VkDebugUtilsLabelEXT label = {
                .sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
                .pLabelName = label_begin->name,
                .color      = {label_begin->color.r, label_begin->color.g, label_begin->color.b, label_begin->color.a},
            };

            vulkan->debug.vkCmdBeginDebugUtilsLabelEXT(command_buffer, &label);
        }
        break;
        case COMMAND_LABEL_END:
        {
            if (!vulkan->debug.vkCmdEndDebugUtilsLabelEXT)
            {
                break;
            }

            vulkan->debug.vkCmdEndDebugUtilsLabelEXT(command_buffer);
        }
        break;
#endif // DEBUG
        default:
        {
            SDL_Log("[VULKAN] Unkown render command of type: %u", header->type);
        }
        }

        at = (u8 *)at + header->size;
    }

    queue->at        = queue->base;
    queue->available = queue->size;
}

vulkan_command_queue *vulkan_command_begin(memory_arena *arena)
{
    vulkan_command_queue *queue = memory_arena_alloc(arena, sizeof(vulkan_command_queue));

    queue->size      = KILOBYTES(100);
    queue->available = queue->size;
    queue->base      = memory_arena_alloc(arena, queue->size);
    queue->at        = queue->base;

    assert(queue->base);

    return queue;
}

bool vulkan_command_end(vulkan_command_queue *queue, vulkan *vulkan)
{
    vulkan_command_handler *handler = &vulkan->command_handler;

    VkCommandBuffer command_buffer;
    if (queue->present)
    {
        vulkan_frame_data *frame_data = &handler->frame_data[handler->frame_index];
        command_buffer                = frame_data->command_buffer;
    }
    else
    {
        vulkan_immeadiate_context *ctx = vulkan_command_handler_immeadiate_get(vulkan, handler);
        command_buffer                 = ctx->buffer;
    }

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to begin recording into command buffer.");
        return false;
    }

    if (queue->present)
    {

        VkImageMemoryBarrier memory_barrier = {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = vulkan->swapchain.images[vulkan->swapchain.image_index],
            .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .layerCount = 1,
                    .levelCount = 1,
                },
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        };

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &memory_barrier);
    }

    VkRect2D scissor = {
        .extent = vulkan->swapchain.extent,
        .offset = (VkOffset2D){0, 0},
    };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    execute_queue(vulkan, queue, command_buffer);

    if (queue->present)
    {
        VkImageMemoryBarrier memory_barrier = {
            .sType     = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .image     = vulkan->swapchain.images[vulkan->swapchain.image_index],
            .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .layerCount = 1,
                    .levelCount = 1,
                },
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        };

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &memory_barrier);
    }

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to end recording into command buffer.");
        return false;
    }

    if (!queue->present)
    {
        VkSubmitInfo submit_info = {
            .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers    = &command_buffer,
        };

        if (vkQueueSubmit(vulkan->graphics_queue.handle, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS)
        {
            SDL_Log("[VULKAN] Failed to submit graphics queue.");
            return false;
        }
    }
    return true;
}

void vulkan_command_set_present(vulkan_command_queue *queue)
{
    queue->present = true;
}

void vulkan_command_handler_destroy(vulkan *vulkan, vulkan_command_handler *handler)
{
    vkDestroyCommandPool(vulkan->device, handler->command_pool, NULL);

    for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(vulkan->device, handler->frame_data[i].image_available, NULL);
        vkDestroyFence(vulkan->device, handler->frame_data[i].in_flight_fence, NULL);
    }

    for (u32 i = 0; i < ARRAY_COUNT(handler->contexts); i++)
    {
        vulkan_immeadiate_context *ctx = &handler->contexts[i];
        vkDestroySemaphore(vulkan->device, ctx->timeline, NULL);
        vkDestroyCommandPool(vulkan->device, ctx->pool, NULL);
    }
}
