#pragma once

#include <deque>
#include <functional>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vk/descriptor.h>

namespace VkRenderer {
    struct AllocatedBuffer {
        VkBuffer _buffer;
        VmaAllocation _allocation;
    };

    struct AllocatedImage {
        VkImage _image;
        VmaAllocation _allocation;
    };

    struct Camera {
        glm::vec3 position;
        glm::mat4 view;
        glm::mat4 projection;
        float angle;
        float rotateVelocity;
        float velocity;
        float sprint_multiplier;
    };

    struct GPUCameraData {
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 viewproj;
    };

    struct GPUSceneData {
        glm::vec4 fogColor; // w for exponent
        glm::vec4 fogDistances; // x for min, y for max, zw unusued
        glm::vec4 ambientColor;
        glm::vec4 sunlightDirection; // w for sun power
        glm::vec4 sunlightColor;
    };

    struct FrameData {
        VkSemaphore _presentSemaphore, _renderSemaphore;
        VkFence _renderFence;
        VkCommandPool _commandPool;
        VkCommandBuffer _mainCommandBuffer;
        AllocatedBuffer cameraBuffer;
        VkRenderer::descriptor::Allocator *_descriptorAllocator;
    };

    struct UploadContext {
        VkFence _uploadFence;
        VkCommandPool _commandPool;
        VkCommandBuffer _commandBuffer;
    };

    struct MatrixPushConstant {
        glm::vec4 data;
        glm::mat4 matrix;
    };

    struct DeletionQueue {
        std::deque<std::function<void()>> deletors;

        void push_function(std::function<void()> &&function) {
            deletors.push_back(function);
        }

        void flush() {
            // reverse iterate deletion queue and call all functions
            for (auto i = deletors.rbegin(); i != deletors.rend(); i++) {
                (*i)();
            }

            deletors.clear();
        }
    };
}