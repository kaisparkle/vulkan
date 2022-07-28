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
#include <vk/init/descriptor.h>

struct Material {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
};

struct RenderObject {
    Mesh *mesh;
    Material *material;
    glm::mat4 transformMatrix;
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
    vkinit::descriptor::Allocator *_descriptorAllocator;
};

constexpr unsigned int FRAME_OVERLAP = 2;

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

class VkRenderer {
public:
    bool _isInitialized{false};
    int _frameNumber = 0;

    struct SDL_Window *_window{nullptr};

    glm::vec3 _camPos = {0.0f, 0.0f, -2.0f};

    DeletionQueue _mainDeletionQueue;
    FrameData _frames[FRAME_OVERLAP];
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
    vkinit::descriptor::LayoutCache *_descriptorLayoutCache;
    VkDescriptorSetLayout _globalSetLayout;
    VkDescriptorSetLayout _objectSetLayout;
    VkPipelineLayout _meshPipelineLayout;
    VkPipeline _meshPipeline;
    GPUSceneData _sceneParameters;
    AllocatedBuffer _sceneParameterBuffer;
    Mesh _monkeyMesh;

    // initializes everything in the engine
    void init();

    FrameData &get_current_frame();

    // draw loop
    void draw();

    // run main loop
    void run();

    // shuts down the engine
    void cleanup();

    std::vector<RenderObject> _renderables;
    std::unordered_map<std::string, Material> _materials;
    std::unordered_map<std::string, Mesh> _meshes;

    // create a Material and add it to map
    Material *create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string &name);

    Material *get_material(const std::string &name);

    Mesh *get_mesh(const std::string &name);

    void draw_objects(VkCommandBuffer cmd, RenderObject *first, int count);

private:
    void init_vulkan();

    void init_swapchain();

    void init_commands();

    void init_default_renderpass();

    void init_framebuffers();

    void init_sync_structures();

    void init_descriptors();

    void init_pipelines();

    void load_meshes();

    void upload_mesh(Mesh &mesh);

    bool load_shader_module(const char *filePath, VkShaderModule *outShaderModule);

    AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

    size_t pad_uniform_buffer_size(size_t originalSize);

    void init_scene();
};
