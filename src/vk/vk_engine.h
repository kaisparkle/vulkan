#pragma once

#include <vector>
#include <functional>
#include <deque>
#include <glm/glm.hpp>
#include <unordered_map>
#include <string>

#include "vk_types.h"
#include "vk_mesh.h"

struct Material {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
};

struct RenderObject {
    Mesh *mesh;
    Material *material;
    glm::mat4 transformMatrix;
};

struct MeshPushConstants {
    glm::vec4 data;
    glm::mat4 render_matrix;
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

class VulkanEngine {
public:
    bool _isInitialized{false};
    int _frameNumber{0};

    struct SDL_Window *_window{nullptr};

    glm::vec3 _camPos = {0.0f, 0.0f, -2.0f};

    DeletionQueue _mainDeletionQueue;
    VmaAllocator _allocator;
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
    VkImageView _depthImageView;
    AllocatedImage _depthImage;
    VkFormat _depthFormat;
    VkQueue _graphicsQueue;
    uint32_t _graphicsQueueFamily;
    VkCommandPool _commandPool;
    VkCommandBuffer _mainCommandBuffer;
    VkRenderPass _renderPass;
    std::vector<VkFramebuffer> _framebuffers;
    VkSemaphore _presentSemaphore, _renderSemaphore;
    VkFence _renderFence;
    VkPipelineLayout _trianglePipelineLayout;
    VkPipelineLayout _meshTrianglePipelineLayout;
    VkPipeline _trianglePipeline;
    VkPipeline _redTrianglePipeline;
    VkPipeline _meshPipeline;
    Mesh _triangleMesh;
    Mesh _monkeyMesh;
    int _selectedShader = 0;

    // initializes everything in the engine
    void init();

    // draw loop
    void draw();

    // run main loop
    void run();

    // shuts down the engine
    void cleanup();

    std::vector<RenderObject> _renderables;
    std::unordered_map<std::string,Material> _materials;
    std::unordered_map<std::string,Mesh> _meshes;

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

    void init_pipelines();

    void load_meshes();

    void upload_mesh(Mesh &mesh);

    bool load_shader_module(const char *filePath, VkShaderModule *outShaderModule);

    void init_scene();
};
