#pragma once

#include <vector>
#include <functional>
#include <deque>

#include "vk_types.h"

struct DeletionQueue {
    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()>&& function) {
        deletors.push_back(function);
    }

    void flush() {
        // reverse iterate deletion queue and call all functions
        for(auto i = deletors.rbegin(); i != deletors.rend(); i++) {
            (*i)();
        }

        deletors.clear();
    }
};

class VulkanEngine {
public:
    bool _isInitialized{false};
    int _frameNumber{0};

    struct SDL_Window *_window{nullptr};

    DeletionQueue _mainDeletionQueue;
    VkExtent2D _windowExtent{1700, 900};
    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debug_messenger;
    VkPhysicalDevice _chosenGPU;
    VkDevice _device;
    VkSurfaceKHR _surface;
    VkSwapchainKHR _swapchain;
    VkFormat _swapchainImageFormat;
    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;
    VkQueue _graphicsQueue;
    uint32_t _graphicsQueueFamily;
    VkCommandPool _commandPool;
    VkCommandBuffer _mainCommandBuffer;
    VkRenderPass _renderPass;
    std::vector<VkFramebuffer> _framebuffers;
    VkSemaphore _presentSemaphore, _renderSemaphore;
    VkFence _renderFence;
    VkPipelineLayout _trianglePipelineLayout;
    VkPipeline _trianglePipeline;
    VkPipeline _redTrianglePipeline;
    int _selectedShader = 0;

    //initializes everything in the engine
    void init();

    //draw loop
    void draw();

    //run main loop
    void run();

    //shuts down the engine
    void cleanup();

private:
    void init_vulkan();

    void init_swapchain();

    void init_commands();

    void init_default_renderpass();

    void init_framebuffers();

    void init_sync_structures();

    void init_pipelines();

    bool load_shader_module(const char* filePath, VkShaderModule* outShaderModule);
};
