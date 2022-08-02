#include <iostream>
#include <vk/info.h>
#include <vk/check.h>

#include "utils.h"

VkRenderer::AllocatedBuffer VkRenderer::utils::create_buffer(VmaAllocator &allocator, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
    // allocate buffer
    VkBufferCreateInfo bufferInfo = VkRenderer::info::buffer_create_info(allocSize, usage);
    VmaAllocationCreateInfo vmaAllocInfo = VkRenderer::info::allocation_create_info(memoryUsage, 0);
    AllocatedBuffer newBuffer;
    VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &newBuffer._buffer, &newBuffer._allocation, nullptr));
    return newBuffer;
}

size_t VkRenderer::utils::pad_uniform_buffer_size(VkPhysicalDeviceProperties gpuProperties, size_t originalSize) {
    // calculate alignment
    size_t minUboAlignment = gpuProperties.limits.minUniformBufferOffsetAlignment;
    size_t alignedSize = originalSize;
    if (minUboAlignment > 0) {
        alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
    }

    return alignedSize;
}

void VkRenderer::utils::immediate_submit(VkDevice &device, VkQueue &queue, UploadContext &uploadContext, std::function<void(VkCommandBuffer cmd)> &&function) {
    // begin buffer recording
    VkCommandBuffer cmd = uploadContext._commandBuffer;
    VkCommandBufferBeginInfo cmdBeginInfo = VkRenderer::info::command_buffer_begin_info(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    // execute passed function
    function(cmd);

    // end recording and submit
    VK_CHECK(vkEndCommandBuffer(cmd));
    VkSubmitInfo submitInfo = VkRenderer::info::submit_info(nullptr, 0, nullptr, 0, nullptr, 1, &cmd);
    VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, uploadContext._uploadFence));

    // block on fence
    vkWaitForFences(device, 1, &uploadContext._uploadFence, true, 1000000000);
    vkResetFences(device, 1, &uploadContext._uploadFence);

    // reset pool
    vkResetCommandPool(device, uploadContext._commandPool, 0);
}