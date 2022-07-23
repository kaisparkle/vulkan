﻿#include "vk_initializers.h"

VkCommandPoolCreateInfo vkinit::command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) {
    // describe a new command pool
    VkCommandPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext = nullptr;
    info.queueFamilyIndex = queueFamilyIndex;
    info.flags = flags;

    return info;
}

VkCommandBufferAllocateInfo vkinit::command_buffer_allocate_info(VkCommandPool pool, uint32_t count, VkCommandBufferLevel level) {
    // describe allocation for new command buffer(s)
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext = nullptr;
    info.commandPool = pool;
    info.commandBufferCount = count;
    info.level = level;

    return info;
}

VkFenceCreateInfo vkinit::fence_create_info(VkFenceCreateFlags flags) {
    // describe new fence
    VkFenceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = flags;

    return info;
}

VkSemaphoreCreateInfo vkinit::semaphore_create_info(VkSemaphoreCreateFlags flags) {
    // describe new semaphore
    VkSemaphoreCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = flags;

    return info;
}

VkPipelineShaderStageCreateInfo vkinit::pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule) {
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

VkPipelineVertexInputStateCreateInfo vkinit::vertex_input_state_create_info() {
    // describe a new vertex input state
    VkPipelineVertexInputStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    info.pNext = nullptr;
    // no vertex bindings or attributes
    info.vertexBindingDescriptionCount = 0;
    info.vertexAttributeDescriptionCount = 0;

    return info;
}

VkPipelineInputAssemblyStateCreateInfo vkinit::input_assembly_create_info(VkPrimitiveTopology topology) {
    // describe a new input assembly state
    VkPipelineInputAssemblyStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    info.pNext = nullptr;
    info.topology = topology;
    info.primitiveRestartEnable = VK_FALSE;

    return info;
}

VkPipelineRasterizationStateCreateInfo vkinit::rasterization_state_create_info(VkPolygonMode polygonMode) {
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

VkPipelineMultisampleStateCreateInfo vkinit::multisampling_state_create_info() {
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

VkPipelineColorBlendAttachmentState vkinit::color_blend_attachment_state() {
    // describe a new color blend attachment state
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // disable blending
    colorBlendAttachment.blendEnable = VK_FALSE;

    return colorBlendAttachment;
}

VkPipelineLayoutCreateInfo vkinit::pipeline_layout_create_info() {
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