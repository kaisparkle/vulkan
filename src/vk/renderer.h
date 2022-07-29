#pragma once

#include <vector>
#include <functional>
#include <deque>
#include <glm/glm.hpp>
#include <unordered_map>
#include <string>
#include <vk/common.h>
#include <vk/types.h>
#include <vk/mesh.h>
#include <vk/material.h>
#include <vk/object.h>
#include <vk/descriptor.h>

namespace VkRenderer {
    constexpr unsigned int FRAME_OVERLAP = 2;

    struct Camera {
        glm::vec3 position;
        glm::mat4 view;
        glm::mat4 projection;
        float speed;
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

    struct GPUObjectData {
        glm::mat4 modelMatrix;
    };

    struct FrameData {
        VkSemaphore _presentSemaphore, _renderSemaphore;
        VkFence _renderFence;
        VkCommandPool _commandPool;
        VkCommandBuffer _mainCommandBuffer;
        AllocatedBuffer cameraBuffer;
        AllocatedBuffer objectBuffer;
        VkRenderer::descriptor::Allocator *_descriptorAllocator;
    };

    struct UploadContext {
        VkFence _uploadFence;
        VkCommandPool _commandPool;
        VkCommandBuffer _commandBuffer;
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

    class Renderer {
    public:
        void init();

        void run();

        void cleanup();

    private:
        bool _isInitialized = false;

        struct SDL_Window *_window{nullptr};

        MaterialManager _materialManager;
        MeshManager _meshManager;
        RenderObjectManager _renderObjectManager;

        int _frameNumber = 0;
        FrameData _frames[FRAME_OVERLAP];
        Camera _camera;
        GPUSceneData _sceneParameters;
        UploadContext _uploadContext;

        DeletionQueue _mainDeletionQueue;
        VmaAllocator _allocator;
        VkExtent2D _windowExtent{1700, 900};
        VkInstance _instance;
        VkDebugUtilsMessengerEXT _debug_messenger;
        VkPhysicalDevice _chosenGPU;
        VkPhysicalDeviceProperties _gpuProperties;
        VkDevice _device;
        VkSurfaceKHR _surface;
        VkSwapchainKHR _swapchain;
        VkFormat _swapchainImageFormat;
        std::vector<VkImage> _swapchainImages;
        std::vector<VkImageView> _swapchainImageViews;
        VkImageView _depthImageView;
        AllocatedImage _depthImage;
        VkFormat _depthFormat;
        VkQueue _graphicsQueue;
        uint32_t _graphicsQueueFamily;
        VkRenderPass _renderPass;
        std::vector<VkFramebuffer> _framebuffers;
        VkRenderer::descriptor::LayoutCache *_descriptorLayoutCache;
        VkDescriptorSetLayout _globalSetLayout;
        VkDescriptorSetLayout _objectSetLayout;
        AllocatedBuffer _sceneParameterBuffer;

        void init_vulkan();

        void init_swapchain();

        void init_commands();

        void init_default_renderpass();

        void init_framebuffers();

        void init_sync_structures();

        void init_descriptors();

        void init_pipelines();

        void init_imgui();

        void load_meshes();

        void upload_mesh(Mesh *mesh);

        void init_scene();

        void draw_objects(VkCommandBuffer cmd);

        void draw();

        FrameData &get_current_frame();

        AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

        size_t pad_uniform_buffer_size(size_t originalSize);

        void immediate_submit(std::function<void(VkCommandBuffer cmd)> &&function);
    };
}