#pragma once

#include <vk/common.h>

#define VK_CHECK(x)                                                    \
    do                                                                 \
    {                                                                  \
        VkResult err = x;                                              \
        if (err)                                                       \
        {                                                              \
            std::cout <<"Detected Vulkan error: " << err << std::endl; \
            abort();                                                   \
        }                                                              \
    } while (0)
