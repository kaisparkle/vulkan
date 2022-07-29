#pragma once

#include <vk/common.h>

namespace VkRenderer {
    struct AllocatedBuffer {
        VkBuffer _buffer;
        VmaAllocation _allocation;
    };

    struct AllocatedImage {
        VkImage _image;
        VmaAllocation _allocation;
    };
}