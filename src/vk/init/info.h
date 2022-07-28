#pragma once

#include <vk/common.h>

namespace vkinit::info {
    VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);

    VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);

    VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0);

    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule);

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info();

    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info(VkPrimitiveTopology topology);

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info(VkPolygonMode polygonMode);

    VkPipelineMultisampleStateCreateInfo multisampling_state_create_info();

    VkPipelineLayoutCreateInfo pipeline_layout_create_info();

    VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);

    VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);

    VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp);

    VkDescriptorPoolCreateInfo descriptor_pool_create_info(int maxSets, uint32_t poolSizeCount, VkDescriptorPoolSize *poolSizes, VkDescriptorPoolCreateFlags flags = 0);

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info(VkDescriptorPool pool, uint32_t setCount, VkDescriptorSetLayout *layouts);

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info(uint32_t bindingCount, VkDescriptorSetLayoutBinding *bindings, VkDescriptorSetLayoutCreateFlags flags = 0);

    VkDescriptorBufferInfo descriptor_buffer_info(VkBuffer buffer, uint32_t offset, uint32_t range);

    VkRenderPassCreateInfo renderpass_create_info(uint32_t attachmentCount, VkAttachmentDescription *attachments, uint32_t dependencyCount, VkSubpassDependency *dependencies,
                                                  uint32_t subpassCount, VkSubpassDescription *subpasses);

    VkRenderPassBeginInfo renderpass_begin_info(VkRenderPass renderPass, int32_t offsetX, int32_t offsetY, VkExtent2D extent, VkFramebuffer framebuffer,
                                                uint32_t clearValueCount, VkClearValue *clearValues);

    VkAttachmentDescription attachment_description(VkFormat format, VkSampleCountFlagBits samples, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
                                                   VkAttachmentLoadOp stencilLoadOp, VkAttachmentStoreOp stencilStoreOp, VkImageLayout initialLayout, VkImageLayout finalLayout);

    VkAttachmentReference attachment_reference(uint32_t attachment, VkImageLayout layout);

    VkSubpassDescription subpass_description(VkPipelineBindPoint pipelineBindPoint, uint32_t colorAttachmentCount, VkAttachmentReference *colorAttachments,
                                             VkAttachmentReference *depthStencilAttachment);

    VkSubpassDependency subpass_dependency(uint32_t srcSubpass, uint32_t dstSubpass, VkPipelineStageFlags srcStageMask, VkAccessFlags srcAccessMask,
                                           VkPipelineStageFlags dstStageMask, VkAccessFlags dstAccessMask);

    VkFramebufferCreateInfo framebuffer_create_info(VkRenderPass renderPass, uint32_t attachmentCount, VkImageView *attachments, uint32_t width, uint32_t height, uint32_t layers);

    VkBufferCreateInfo buffer_create_info(VkDeviceSize size, VkBufferUsageFlags usage);

    VmaAllocatorCreateInfo allocator_create_info(VkPhysicalDevice physicalDevice, VkDevice device, VkInstance instance);

    VmaAllocationCreateInfo allocation_create_info(VmaMemoryUsage usage, VkMemoryPropertyFlags requiredFlags);

    VkShaderModuleCreateInfo shader_module_create_info(size_t codeSize, const uint32_t *code);

    VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferInheritanceInfo *inheritanceInfo, VkCommandBufferUsageFlags flags);

    VkSubmitInfo submit_info(const VkPipelineStageFlags *waitStage, uint32_t waitSemaphoreCount, VkSemaphore *waitSemaphores,
                             uint32_t signalSemaphoreCount, VkSemaphore *signalSemaphores, uint32_t commandBufferCount, VkCommandBuffer *commandBuffers);

    VkPresentInfoKHR present_info(uint32_t swapchainCount, VkSwapchainKHR *swapchains, uint32_t waitSemaphoreCount, VkSemaphore *waitSemaphores, const uint32_t *imageIndices);
}