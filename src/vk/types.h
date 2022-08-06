#pragma once

#include <deque>
#include <functional>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vk/descriptor.h>
#include <camera.h>

namespace VkRenderer {
    struct AllocatedBuffer {
        VkBuffer _buffer;
        VmaAllocation _allocation;
    };

    struct AllocatedImage {
        VkImage _image;
        VmaAllocation _allocation;
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

    struct ResourceHandles {
        struct SDL_Window *window{nullptr};
        FlyCamera *flyCamera;
        GPUSceneData sceneParameters;
        UploadContext uploadContext;
        DeletionQueue mainDeletionQueue;
        VmaAllocator allocator;
        VkExtent2D windowExtent{1700, 900};
        VkInstance instance;
        VkDebugUtilsMessengerEXT debug_messenger;
        VkPhysicalDevice chosenGPU;
        VkPhysicalDeviceProperties gpuProperties;
        VkDevice device;
        VkSurfaceKHR surface;
        VkSwapchainKHR swapchain;
        VkFormat swapchainImageFormat;
        std::vector<VkImage> swapchainImages;
        std::vector<VkImageView> swapchainImageViews;
        VkImageView depthImageView;
        AllocatedImage depthImage;
        VkFormat depthFormat;
        VkQueue graphicsQueue;
        uint32_t graphicsQueueFamily;
        VkRenderPass renderPass;
        std::vector<VkFramebuffer> framebuffers;
        VkRenderer::descriptor::LayoutCache *descriptorLayoutCache;
        VkDescriptorSetLayout globalSetLayout;
        VkDescriptorSetLayout textureSetLayout;
    };
}