// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>
#include <fstream>
#include <vector>
namespace vkinit 
{
	//vulkan init code goes here

    VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex,
        VkCommandPoolCreateFlags flags /*= 0*/);

    VkCommandBufferAllocateInfo command_buffer_allocate_info(
        VkCommandPool pool, uint32_t count /*= 1*/);

    VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags);

    VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags);

    VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags);

    VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspectMask);

    VkSemaphoreSubmitInfo semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);

    VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer cmd);

    VkSubmitInfo2 submit_info(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo);

    VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);

    VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);

    VkRenderingAttachmentInfo attachment_info(VkImageView view, VkClearValue* clear, VkImageLayout layout);

    VkRenderingInfo rendering_info(VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment);

    VkPresentInfoKHR present_info();

    VkWriteDescriptorSet write_descriptor_image(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info();

    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char* entry);

    VkRenderingAttachmentInfo depth_attachment_info(
        VkImageView view, VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/);
}

namespace vkutil
{
    void generate_mipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize);
    void copy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
    void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
    bool load_shader_module(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);
}