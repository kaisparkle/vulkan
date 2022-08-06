#include <iostream>
#include <vk/info.h>
#include <vk/check.h>

#include "utils.h"

namespace VkRenderer::utils {
    AllocatedBuffer create_buffer(VmaAllocator &allocator, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
        // allocate buffer
        VkBufferCreateInfo bufferInfo = VkRenderer::info::buffer_create_info(allocSize, usage);
        VmaAllocationCreateInfo vmaAllocInfo = VkRenderer::info::allocation_create_info(memoryUsage, 0);
        AllocatedBuffer newBuffer;
        VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &newBuffer._buffer, &newBuffer._allocation, nullptr));
        return newBuffer;
    }

    size_t pad_uniform_buffer_size(VkPhysicalDeviceProperties gpuProperties, size_t originalSize) {
        // calculate alignment
        size_t minUboAlignment = gpuProperties.limits.minUniformBufferOffsetAlignment;
        size_t alignedSize = originalSize;
        if (minUboAlignment > 0) {
            alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
        }

        return alignedSize;
    }

    void immediate_submit(ResourceHandles *resources, std::function<void(VkCommandBuffer cmd)> &&function) {
        // begin buffer recording
        VkCommandBuffer cmd = resources->uploadContext._commandBuffer;
        VkCommandBufferBeginInfo cmdBeginInfo = VkRenderer::info::command_buffer_begin_info(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        // execute passed function
        function(cmd);

        // end recording and submit
        VK_CHECK(vkEndCommandBuffer(cmd));
        VkSubmitInfo submitInfo = VkRenderer::info::submit_info(nullptr, 0, nullptr, 0, nullptr, 1, &cmd);
        VK_CHECK(vkQueueSubmit(resources->graphicsQueue, 1, &submitInfo, resources->uploadContext._uploadFence));

        // block on fence
        vkWaitForFences(resources->device, 1, &resources->uploadContext._uploadFence, true, 1000000000);
        vkResetFences(resources->device, 1, &resources->uploadContext._uploadFence);

        // reset pool
        vkResetCommandPool(resources->device, resources->uploadContext._commandPool, 0);
    }
}