#include "info.h"

namespace VkRenderer::info {
    VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) {
        // describe a new command pool
        VkCommandPoolCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.pNext = nullptr;
        info.queueFamilyIndex = queueFamilyIndex;
        info.flags = flags;

        return info;
    }

    VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count, VkCommandBufferLevel level) {
        // describe allocation for new command buffer(s)
        VkCommandBufferAllocateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.pNext = nullptr;
        info.commandPool = pool;
        info.commandBufferCount = count;
        info.level = level;

        return info;
    }

    VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags) {
        // describe new fence
        VkFenceCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = flags;

        return info;
    }

    VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags) {
        // describe new semaphore
        VkSemaphoreCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = flags;

        return info;
    }

    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule) {
        // describe a new pipeline shader stage
        VkPipelineShaderStageCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.pNext = nullptr;
        info.stage = stage;
        info.module = shaderModule;
        // shader's entry point
        info.pName = "main";

        return info;
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info() {
        // describe a new vertex input state
        VkPipelineVertexInputStateCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        info.pNext = nullptr;
        // no vertex bindings or attributes
        info.vertexBindingDescriptionCount = 0;
        info.vertexAttributeDescriptionCount = 0;

        return info;
    }

    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info(VkPrimitiveTopology topology) {
        // describe a new input assembly state
        VkPipelineInputAssemblyStateCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        info.pNext = nullptr;
        info.topology = topology;
        info.primitiveRestartEnable = VK_FALSE;

        return info;
    }

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info(VkPolygonMode polygonMode) {
        // describe a new rasterization state
        VkPipelineRasterizationStateCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        info.pNext = nullptr;
        info.depthClampEnable = VK_FALSE;
        info.rasterizerDiscardEnable = VK_FALSE;
        info.polygonMode = polygonMode;
        info.lineWidth = 1.0f;
        // disable backface culling
        info.cullMode = VK_CULL_MODE_NONE;
        info.frontFace = VK_FRONT_FACE_CLOCKWISE;
        // disable depth bias
        info.depthBiasEnable = VK_FALSE;
        info.depthBiasConstantFactor = 0.0f;
        info.depthBiasClamp = 0.0f;
        info.depthBiasSlopeFactor = 0.0f;

        return info;
    }

    VkPipelineMultisampleStateCreateInfo multisampling_state_create_info() {
        // describe a new multisample state
        VkPipelineMultisampleStateCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        info.pNext = nullptr;
        // disable multisampling
        info.sampleShadingEnable = VK_FALSE;
        info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        info.minSampleShading = 1.0f;
        info.pSampleMask = nullptr;
        info.alphaToCoverageEnable = VK_FALSE;
        info.alphaToOneEnable = VK_FALSE;

        return info;
    }

    VkPipelineLayoutCreateInfo pipeline_layout_create_info() {
        VkPipelineLayoutCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        info.pNext = nullptr;
        // empty defaults
        info.flags = 0;
        info.setLayoutCount = 0;
        info.pSetLayouts = nullptr;
        info.pushConstantRangeCount = 0;
        info.pPushConstantRanges = nullptr;

        return info;
    }

    VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent) {
        // describe a new VkImage
        VkImageCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.pNext = nullptr;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = format;
        info.extent = extent;
        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = usageFlags;

        return info;
    }

    VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags) {
        // describe a new VkImageView
        VkImageViewCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.pNext = nullptr;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.image = image;
        info.format = format;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;
        info.subresourceRange.aspectMask = aspectFlags;

        return info;
    }

    VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp) {
        // describe a new depth stencil state
        VkPipelineDepthStencilStateCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        info.pNext = nullptr;
        info.depthTestEnable = bDepthTest ? VK_TRUE : VK_FALSE;
        info.depthWriteEnable = bDepthWrite ? VK_TRUE : VK_FALSE;
        info.depthCompareOp = bDepthTest ? compareOp : VK_COMPARE_OP_ALWAYS;
        info.depthBoundsTestEnable = VK_FALSE;
        info.minDepthBounds = 0.0f;
        info.maxDepthBounds = 1.0f;
        info.stencilTestEnable = VK_FALSE;

        return info;
    }

    VkDescriptorPoolCreateInfo descriptor_pool_create_info(int maxSets, uint32_t poolSizeCount, VkDescriptorPoolSize *poolSizes, VkDescriptorPoolCreateFlags flags) {
        // describe a new descriptor pool
        VkDescriptorPoolCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.pNext = nullptr;
        info.maxSets = maxSets;
        info.poolSizeCount = poolSizeCount;
        info.pPoolSizes = poolSizes;
        info.flags = flags;

        return info;
    }

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info(VkDescriptorPool pool, uint32_t setCount, VkDescriptorSetLayout *layouts) {
        // describe a new set allocation
        VkDescriptorSetAllocateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        info.pNext = nullptr;
        info.descriptorPool = pool;
        info.descriptorSetCount = setCount;
        info.pSetLayouts = layouts;

        return info;
    }

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info(uint32_t bindingCount, VkDescriptorSetLayoutBinding *bindings, VkDescriptorSetLayoutCreateFlags flags) {
        VkDescriptorSetLayoutCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.pNext = nullptr;
        info.bindingCount = bindingCount;
        info.pBindings = bindings;
        info.flags = flags;

        return info;
    }

    VkDescriptorBufferInfo descriptor_buffer_info(VkBuffer buffer, uint32_t offset, uint32_t range) {
        VkDescriptorBufferInfo info = {};
        info.buffer = buffer;
        info.offset = offset;
        info.range = range;

        return info;
    }

    VkRenderPassCreateInfo renderpass_create_info(uint32_t attachmentCount, VkAttachmentDescription *attachments, uint32_t dependencyCount, VkSubpassDependency *dependencies,
                                                  uint32_t subpassCount, VkSubpassDescription *subpasses) {
        VkRenderPassCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = attachmentCount;
        info.pAttachments = attachments;
        info.dependencyCount = dependencyCount;
        info.pDependencies = dependencies;
        info.subpassCount = subpassCount;
        info.pSubpasses = subpasses;

        return info;
    }

    VkRenderPassBeginInfo renderpass_begin_info(VkRenderPass renderPass, int32_t offsetX, int32_t offsetY, VkExtent2D extent, VkFramebuffer framebuffer,
                                                uint32_t clearValueCount, VkClearValue *clearValues) {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.pNext = nullptr;
        info.renderPass = renderPass;
        info.renderArea.offset.x = offsetX;
        info.renderArea.offset.y = offsetY;
        info.renderArea.extent = extent;
        info.framebuffer = framebuffer;
        info.clearValueCount = clearValueCount;
        info.pClearValues = clearValues;

        return info;
    }

    VkAttachmentDescription attachment_description(VkFormat format, VkSampleCountFlagBits samples, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
                                                   VkAttachmentLoadOp stencilLoadOp, VkAttachmentStoreOp stencilStoreOp, VkImageLayout initialLayout, VkImageLayout finalLayout) {
        VkAttachmentDescription description = {};
        description.format = format;
        description.samples = samples;
        description.loadOp = loadOp;
        description.storeOp = storeOp;
        description.stencilLoadOp = stencilLoadOp;
        description.stencilStoreOp = stencilStoreOp;
        description.initialLayout = initialLayout;
        description.finalLayout = finalLayout;

        return description;
    }

    VkAttachmentReference attachment_reference(uint32_t attachment, VkImageLayout layout) {
        VkAttachmentReference reference = {};
        reference.attachment = attachment;
        reference.layout = layout;

        return reference;
    }

    VkSubpassDescription subpass_description(VkPipelineBindPoint pipelineBindPoint, uint32_t colorAttachmentCount, VkAttachmentReference *colorAttachments,
                                             VkAttachmentReference *depthStencilAttachment) {
        VkSubpassDescription description = {};
        description.pipelineBindPoint = pipelineBindPoint;
        description.colorAttachmentCount = colorAttachmentCount;
        description.pColorAttachments = colorAttachments;
        description.pDepthStencilAttachment = depthStencilAttachment;

        return description;
    }

    VkSubpassDependency subpass_dependency(uint32_t srcSubpass, uint32_t dstSubpass, VkPipelineStageFlags srcStageMask, VkAccessFlags srcAccessMask,
                                           VkPipelineStageFlags dstStageMask, VkAccessFlags dstAccessMask) {
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = srcSubpass;
        dependency.dstSubpass = dstSubpass;
        dependency.srcStageMask = srcStageMask;
        dependency.srcAccessMask = srcAccessMask;
        dependency.dstStageMask = dstStageMask;
        dependency.dstAccessMask = dstAccessMask;

        return dependency;
    }

    VkFramebufferCreateInfo framebuffer_create_info(VkRenderPass renderPass, uint32_t attachmentCount, VkImageView *attachments, uint32_t width, uint32_t height, uint32_t layers) {
        VkFramebufferCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.pNext = nullptr;
        info.renderPass = renderPass;
        info.attachmentCount = attachmentCount;
        info.pAttachments = attachments;
        info.width = width;
        info.height = height;
        info.layers = layers;

        return info;
    }

    VkBufferCreateInfo buffer_create_info(VkDeviceSize size, VkBufferUsageFlags usage) {
        VkBufferCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.pNext = nullptr;
        info.size = size;
        info.usage = usage;

        return info;
    }

    VmaAllocatorCreateInfo allocator_create_info(VkPhysicalDevice physicalDevice, VkDevice device, VkInstance instance) {
        VmaAllocatorCreateInfo info = {};
        info.physicalDevice = physicalDevice;
        info.device = device;
        info.instance = instance;

        return info;
    }

    VmaAllocationCreateInfo allocation_create_info(VmaMemoryUsage usage, VkMemoryPropertyFlags requiredFlags) {
        VmaAllocationCreateInfo info = {};
        info.usage = usage;
        info.requiredFlags = requiredFlags;

        return info;
    }

    VkShaderModuleCreateInfo shader_module_create_info(size_t codeSize, const uint32_t *code) {
        VkShaderModuleCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        info.pNext = nullptr;
        info.codeSize = codeSize;
        info.pCode = code;

        return info;
    }

    VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferInheritanceInfo *inheritanceInfo, VkCommandBufferUsageFlags flags) {
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.pNext = nullptr;
        info.pInheritanceInfo = inheritanceInfo;
        info.flags = flags;

        return info;
    }

    VkSubmitInfo submit_info(const VkPipelineStageFlags *waitStage, uint32_t waitSemaphoreCount, VkSemaphore *waitSemaphores,
                             uint32_t signalSemaphoreCount, VkSemaphore *signalSemaphores, uint32_t commandBufferCount, VkCommandBuffer *commandBuffers) {
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.pNext = nullptr;
        info.pWaitDstStageMask = waitStage;
        info.waitSemaphoreCount = waitSemaphoreCount;
        info.pWaitSemaphores = waitSemaphores;
        info.signalSemaphoreCount = signalSemaphoreCount;
        info.pSignalSemaphores = signalSemaphores;
        info.commandBufferCount = commandBufferCount;
        info.pCommandBuffers = commandBuffers;

        return info;
    }

    VkPresentInfoKHR present_info(uint32_t swapchainCount, VkSwapchainKHR *swapchains, uint32_t waitSemaphoreCount, VkSemaphore *waitSemaphores, const uint32_t *imageIndices) {
        VkPresentInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        info.pNext = nullptr;
        info.swapchainCount = swapchainCount;
        info.pSwapchains = swapchains;
        info.waitSemaphoreCount = waitSemaphoreCount;
        info.pWaitSemaphores = waitSemaphores;
        info.pImageIndices = imageIndices;

        return info;
    }

    VkSamplerCreateInfo sampler_create_info(VkFilter filters, VkSamplerAddressMode samplerAddressMode) {
        VkSamplerCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.pNext = nullptr;
        info.magFilter = filters;
        info.minFilter = filters;
        info.addressModeU = samplerAddressMode;
        info.addressModeV = samplerAddressMode;
        info.addressModeW = samplerAddressMode;

        return info;
    }
}