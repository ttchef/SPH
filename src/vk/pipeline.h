
#pragma once

#include <types.h>
#include <vk/types.h>
#include <vk/buffer.h>
#include <vk/descriptor.h>

#include <vulkan/vulkan_core.h>

#define VULKAN_PIPELINE_DESC_MAX_VERTEX_ATTRIBUTES 8

typedef u32 vulkan_pipeline_id;
#define INVALID_PIPELINE 0 

typedef enum vulkan_pipeline_type
{
	VULKAN_PIPELINE_TYPE_GRAPHICS,
	VULKAN_PIPELINE_TYPE_COMPUTE,
} vulkan_pipeline_type;

typedef struct vulkan_pipeline_desc
{
	vulkan_pipeline_type type;

	const char *vertex_path;
	const char *fragment_path;
	const char *compute_path;

	VkVertexInputBindingDescription vertex_binding;
	VkVertexInputAttributeDescription vertex_attributes[VULKAN_PIPELINE_DESC_MAX_VERTEX_ATTRIBUTES];
	u32 vertex_attribute_count;

	vulkan_descriptor descriptors[12];
	u32 descriptor_count;

	u32 push_constant_size;
	VkShaderStageFlags push_constants_stages;

	bool has_specialization_constant;
	void *specialization_constant_data;
	u32 specialization_constant_size;
	VkShaderStageFlags specialitation_shader_stage;
} vulkan_pipeline_desc;

typedef struct vulkan_pipeline
{
	vulkan_pipeline_type type;

	VkPipeline handle;
	VkPipelineLayout layout;
	
	vulkan_descriptor descriptors[12];
	u32 descriptor_count;	
} vulkan_pipeline;

typedef struct vulkan_pipeline_manager
{
	vulkan_pipeline pipelines[64];
	u32 count;
} vulkan_pipeline_manager;

//
// NOTE: Pipeline builder
//

vulkan_pipeline_desc vulkan_pipeline_default(vulkan_pipeline_type type);

void vulkan_pipeline_desc_set_vertex_input(vulkan_pipeline_desc *desc, u32 vertex_stride, VkVertexInputAttributeDescription *attribues, u32 attribute_count);

void vulkan_pipeline_desc_add_storage_buffer(vulkan_pipeline_desc *desc, vulkan_context *ctx, vulkan_buffer buffer, u32 binding, VkShaderStageFlags stage);

void vulkan_pipeline_desc_set_push_constant(vulkan_pipeline_desc *desc, u32 size, VkShaderStageFlags stages);

void vulkan_pipeline_desc_set_shaders(vulkan_pipeline_desc *desc, const char *vertex, const char *fragment, const char *compute);

void vulkan_pipeline_desc_set_specialization_constant(vulkan_pipeline_desc *desc, u32 size, void *data, VkShaderStageFlags stage);

//
// NOTE: Pipeline manger
//       Is used to manage the pipelines so they dont need to be destroyed manually
//

bool vulkan_pipeline_manager_create(vulkan_pipeline_manager *manager);

void vulkan_pipeline_manager_destroy(vulkan_context *ctx, vulkan_pipeline_manager *manager);
vulkan_pipeline_id vulkan_pipeline_create(vulkan_context *ctx, vulkan_pipeline_desc *desc);

vulkan_pipeline *vulkan_pipeline_get(vulkan_context *ctx, vulkan_pipeline_id id);
