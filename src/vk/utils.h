#include <vk/types.h>

namespace VkRenderer::utils {
    AllocatedBuffer create_buffer(VmaAllocator &allocator, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

    size_t pad_uniform_buffer_size(VkPhysicalDeviceProperties gpuProperties, size_t originalSize);

    void immediate_submit(ResourceHandles *resources, std::function<void(VkCommandBuffer cmd)> &&function);
}