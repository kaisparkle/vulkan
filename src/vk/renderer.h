#pragma once

#include <functional>
#include <vk/material.h>
#include <vk/model.h>
#include <vk/types.h>
#include <camera.h>

namespace VkRenderer {
    constexpr unsigned int FRAME_OVERLAP = 2;

    class Renderer {
    public:
        void init();

        void run();

        void cleanup();

    private:
        bool _isInitialized = false;

        struct SDL_Window *_window{nullptr};
        FlyCamera *_flyCamera;
        bool _toggleUI = true;

        MaterialManager _materialManager;
        ModelManager _modelManager;

        double _previousFrameTime;

        int _frameNumber = 0;
        FrameData _frames[FRAME_OVERLAP];
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
        AllocatedBuffer _sceneParameterBuffer;

        void init_vulkan();

        void init_swapchain();

        void init_commands();

        void init_default_renderpass();

        void init_framebuffers();

        void init_sync_structures();

        void init_descriptors();

        void init_materials();

        void init_imgui();

        void init_scene();

        void draw_objects(VkCommandBuffer cmd);

        void draw();

        FrameData &get_current_frame();
    };
}