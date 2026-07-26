
#pragma once

#include <types.h>
#include <vk/types.h>
#include <math/types.h>

#include <vulkan/vulkan_core.h>

typedef struct
{
	VkImage handle;
	VkImageView view;
	VkDeviceMemory memory;

	VkImageLayout layout;
	VkAccessFlags access;
	VkImageAspectFlags aspect;
} vulkan_image;

typedef struct
{
	VkSampler handle;
} vulkan_sampler;

bool vulkan_image_create(vulkan *vulkan, v2u dimensions, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect, vulkan_image *out_image);

void vulkan_image_destroy(vulkan *vulkan, vulkan_image image);

bool vulkan_image_transition(vulkan *vulkan, vulkan_image *image, VkImageLayout old_layout, VkImageLayout new_layout,
                         	 VkAccessFlags src_access, VkAccessFlags dst_access, VkPipelineStageFlags src_stage,
                    		 VkPipelineStageFlags dst_stage, VkImageAspectFlags aspect_mask);

bool vulkan_image_data_upload(vulkan *vulkan, vulkan_image *image, u32 size, void *data, v2u dimensions, VkImageLayout layout, VkAccessFlags access, VkPipelineStageFlags dst_stage);

bool vulkan_sampler_create(vulkan *vulkan, vulkan_sampler *out_sampler);

void vulkan_sampler_destroy(vulkan *vulkan, vulkan_sampler *sampler);
