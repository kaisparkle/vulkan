#include <iostream>
#include <fstream>
#include <cmath>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <VkBootstrap.h>
#include <glm/gtx/transform.hpp>

#include "vk_engine.h"
#include "vk_types.h"
#include "vk_initializers.h"
#include "vk_pipeline.h"
#include "vk_check.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

void VulkanEngine::init() {
    // initialize SDL and create a window
    SDL_Init(SDL_INIT_VIDEO);
    SDL_WindowFlags window_flags = (SDL_WindowFlags) (SDL_WINDOW_VULKAN);
    _window = SDL_CreateWindow(
            "Vulkan",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            _windowExtent.width,
            _windowExtent.height,
            window_flags
    );

    init_vulkan();
    init_swapchain();
    init_commands();
    init_default_renderpass();
    init_framebuffers();
    init_sync_structures();
    init_pipelines();
    load_meshes();
    init_scene();

    _isInitialized = true;
}

void VulkanEngine::init_vulkan() {
    vkb::InstanceBuilder builder;

    // create Vulkan instance with debugging features
    auto inst_ret = builder.set_app_name("Example Vulkan Application")
            .request_validation_layers(true)
            .require_api_version(1, 1, 0)
            .use_default_debug_messenger()
            .build();

    vkb::Instance vkb_inst = inst_ret.value();

    // set the instance and debug messenger handles
    _instance = vkb_inst.instance;
    _debug_messenger = vkb_inst.debug_messenger;

    // grab SDL window surface
    SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

    // just pick a GPU that supports Vulkan 1.1
    vkb::PhysicalDeviceSelector selector{vkb_inst};
    vkb::PhysicalDevice physicalDevice = selector
            .set_minimum_version(1, 1)
            .set_surface(_surface)
            .select()
            .value();

    // create the device
    vkb::DeviceBuilder deviceBuilder{physicalDevice};
    vkb::Device vkbDevice = deviceBuilder.build().value();

    // set the device handles
    _device = vkbDevice.device;
    _chosenGPU = physicalDevice.physical_device;

    // get a graphics queue
    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    // initialize memory allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = _chosenGPU;
    allocatorInfo.device = _device;
    allocatorInfo.instance = _instance;
    vmaCreateAllocator(&allocatorInfo, &_allocator);
}

void VulkanEngine::init_swapchain() {
    // set up a swapchain
    vkb::SwapchainBuilder swapchainBuilder{_chosenGPU, _device, _surface};
    vkb::Swapchain vkbSwapchain = swapchainBuilder
            .use_default_format_selection()
                    // hard vsync present
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(_windowExtent.width, _windowExtent.height)
            .build()
            .value();

    // set swapchain and image handles
    _swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapchainImageViews = vkbSwapchain.get_image_views().value();
    _swapchainImageFormat = vkbSwapchain.image_format;

    // set up depth image
    VkExtent3D depthImageExtent = {
            _windowExtent.width,
            _windowExtent.height,
            1
    };
    _depthFormat = VK_FORMAT_D32_SFLOAT;
    VkImageCreateInfo dimg_info = vkinit::image_create_info(_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

    // allocate memory on GPU only
    VmaAllocationCreateInfo dimg_allocinfo = {};
    dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    dimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vmaCreateImage(_allocator, &dimg_info, &dimg_allocinfo, &_depthImage._image, &_depthImage._allocation, nullptr);

    // build image view for depth image
    VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(_depthFormat, _depthImage._image, VK_IMAGE_ASPECT_DEPTH_BIT);
    VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depthImageView));

    _mainDeletionQueue.push_function([=]() {
        vkDestroyImageView(_device, _depthImageView, nullptr);
        vmaDestroyImage(_allocator, _depthImage._image, _depthImage._allocation);
        vkDestroySwapchainKHR(_device, _swapchain, nullptr);
    });
}

void VulkanEngine::init_commands() {
    // create command pool and check result
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily,
                                                                               VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_commandPool));

    // allocate one command buffer and check result
    VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_commandPool, 1);
    VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_mainCommandBuffer));

    _mainDeletionQueue.push_function([=]() {
        vkDestroyCommandPool(_device, _commandPool, nullptr);
    });
}

void VulkanEngine::init_default_renderpass() {
    // describe color attachment for renderpass
    VkAttachmentDescription color_attachment = {};
    // swapchain format
    color_attachment.format = _swapchainImageFormat;
    // one sample
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    // clear on load
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // store on renderpass end
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // don't care about stencil
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // don't know or care about starting layout
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // has to be presentation layout on renderpass end
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // depth attachment
    VkAttachmentDescription depth_attachment = {};
    depth_attachment.flags = 0;
    depth_attachment.format = _depthFormat;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref = {};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // describe one subpass, minimum possible
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    // subpass dependencies for sync
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkSubpassDependency depth_dependency = {};
    depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    depth_dependency.dstSubpass = 0;
    depth_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depth_dependency.srcAccessMask = 0;
    depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // attachment and dependency arrays
    VkAttachmentDescription attachments[2] = {color_attachment, depth_attachment};
    VkSubpassDependency dependencies[2] = {dependency, depth_dependency};

    // describe the render pass, connect attachments and subpass
    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 2;
    render_pass_info.pAttachments = &attachments[0];
    render_pass_info.dependencyCount = 2;
    render_pass_info.pDependencies = &dependencies[0];
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    // create the render pass and check result
    VK_CHECK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_renderPass));

    _mainDeletionQueue.push_function([=]() {
        vkDestroyRenderPass(_device, _renderPass, nullptr);
    });
}

void VulkanEngine::init_framebuffers() {
    // describe framebuffers for swapchain images
    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = nullptr;
    fb_info.renderPass = _renderPass;
    fb_info.attachmentCount = 1;
    fb_info.width = _windowExtent.width;
    fb_info.height = _windowExtent.height;
    fb_info.layers = 1;

    const uint32_t swapchain_imagecount = _swapchainImages.size();
    _framebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

    // create framebuffers for each swapchain image and check result
    for (size_t i = 0; i < swapchain_imagecount; i++) {
        VkImageView attachments[2];
        attachments[0] = _swapchainImageViews[i];
        attachments[1] = _depthImageView;

        fb_info.attachmentCount = 2;
        fb_info.pAttachments = attachments;

        VK_CHECK(vkCreateFramebuffer(_device, &fb_info, nullptr, &_framebuffers[i]));

        _mainDeletionQueue.push_function([=]() {
            vkDestroyFramebuffer(_device, _framebuffers[i], nullptr);
            vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
        });
    }
}

void VulkanEngine::init_sync_structures() {
    // create fence and check result
    VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_renderFence));

    _mainDeletionQueue.push_function([=]() {
        vkDestroyFence(_device, _renderFence, nullptr);
    });

    // create semaphores and check results
    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();
    VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_presentSemaphore));
    VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_renderSemaphore));

    _mainDeletionQueue.push_function([=]() {
        vkDestroySemaphore(_device, _presentSemaphore, nullptr);
        vkDestroySemaphore(_device, _renderSemaphore, nullptr);
    });
}

void VulkanEngine::init_pipelines() {
    // attempt to load colored triangle shaders
    VkShaderModule triangleFragShader;
    if(!load_shader_module("../shaders/triangle_colored.frag.spv", &triangleFragShader)) {
        std::cout << "Error building triangle fragment shader module" << std::endl;
    } else {
        std::cout << "Triangle fragment shader successfully loaded" << std::endl;
    }
    VkShaderModule triangleVertShader;
    if(!load_shader_module("../shaders/triangle_colored.vert.spv", &triangleVertShader)) {
        std::cout << "Error building triangle vertex shader module" << std::endl;
    } else {
        std::cout << "Triangle vertex shader successfully loaded" << std::endl;
    }

    // attempt to load red triangle shaders
    VkShaderModule redTriangleFragShader;
    if(!load_shader_module("../shaders/triangle.frag.spv", &redTriangleFragShader)) {
        std::cout << "Error building red triangle fragment shader module" << std::endl;
    } else {
        std::cout << "Red triangle fragment shader successfully loaded" << std::endl;
    }
    VkShaderModule redTriangleVertShader;
    if(!load_shader_module("../shaders/triangle.vert.spv", &redTriangleVertShader)) {
        std::cout << "Error building red triangle vertex shader module" << std::endl;
    } else {
        std::cout << "Red triangle vertex shader successfully loaded" << std::endl;
    }

    // build the pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_trianglePipelineLayout));

    // build the mesh pipeline layout
    VkPipelineLayoutCreateInfo mesh_pipeline_layout_info = vkinit::pipeline_layout_create_info();
    // set up push constants
    VkPushConstantRange push_constant;
    push_constant.offset = 0;
    push_constant.size = sizeof(MeshPushConstants);
    // accessible only to vertex shader
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    mesh_pipeline_layout_info.pPushConstantRanges = &push_constant;
    mesh_pipeline_layout_info.pushConstantRangeCount = 1;
    VK_CHECK(vkCreatePipelineLayout(_device, &mesh_pipeline_layout_info, nullptr, &_meshTrianglePipelineLayout));

    // prepare to build the pipeline
    PipelineBuilder pipelineBuilder;

    // build stage create info for vertex and fragment stages
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, triangleVertShader));
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));

    // not using yet, use default
    pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();

    // using triangle list topology
    pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    // build viewport and scissor using window extents
    pipelineBuilder._viewport.x = 0.0f;
    pipelineBuilder._viewport.y = 0.0f;
    pipelineBuilder._viewport.width = (float)_windowExtent.width;
    pipelineBuilder._viewport.height = (float)_windowExtent.height;
    pipelineBuilder._viewport.minDepth = 0.0f;
    pipelineBuilder._viewport.maxDepth = 1.0f;
    pipelineBuilder._scissor.offset = {0, 0};
    pipelineBuilder._scissor.extent = _windowExtent;

    // drawing filled triangles
    pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);

    // no multisampling, use default
    pipelineBuilder._multisampling = vkinit::multisampling_state_create_info();

    // one blend attachment, no blending, RGBA
    pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state();

    // use triangle layout we just created
    pipelineBuilder._pipelineLayout = _trianglePipelineLayout;

    // build the pipeline
    pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
    _trianglePipeline = pipelineBuilder.build_pipeline(_device, _renderPass);

    // clear builder shader stages
    pipelineBuilder._shaderStages.clear();

    // add red triangle shaders
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, redTriangleVertShader));
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, redTriangleFragShader));

    // build red triangle pipeline
    _redTrianglePipeline = pipelineBuilder.build_pipeline(_device, _renderPass);

    // prepare to build mesh pipeline
    VertexInputDescription vertexDescription = Vertex::get_vertex_description();
    pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
    pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();
    pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
    pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();

    // clear shader stages
    pipelineBuilder._shaderStages.clear();

    // attempt to load mesh vertex shader
    VkShaderModule meshTriangleVertShader;
    if(!load_shader_module("../shaders/triangle_mesh.vert.spv", &meshTriangleVertShader)) {
        std::cout << "Error building mesh triangle vertex shader module" << std::endl;
    } else {
        std::cout << "Mesh triangle vertex shader successfully loaded" << std::endl;
    }

    // add shaders to pipeline
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, meshTriangleVertShader));
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));

    // build mesh triangle pipeline
    pipelineBuilder._pipelineLayout = _meshTrianglePipelineLayout;
    _meshPipeline = pipelineBuilder.build_pipeline(_device, _renderPass);
    create_material(_meshPipeline, _meshTrianglePipelineLayout, "default_mesh");

    // shader modules are okay to delete once pipeline is built
    vkDestroyShaderModule(_device, meshTriangleVertShader, nullptr);
    vkDestroyShaderModule(_device, redTriangleVertShader, nullptr);
    vkDestroyShaderModule(_device, redTriangleFragShader, nullptr);
    vkDestroyShaderModule(_device, triangleFragShader, nullptr);
    vkDestroyShaderModule(_device, triangleVertShader, nullptr);

    _mainDeletionQueue.push_function([=]() {
        vkDestroyPipeline(_device, _meshPipeline, nullptr);
        vkDestroyPipeline(_device, _redTrianglePipeline, nullptr);
        vkDestroyPipeline(_device, _trianglePipeline, nullptr);
        vkDestroyPipelineLayout(_device, _meshTrianglePipelineLayout, nullptr);
        vkDestroyPipelineLayout(_device, _trianglePipelineLayout, nullptr);
    });
}

void VulkanEngine::load_meshes() {
    // three vertices for triangle
    _triangleMesh._vertices.resize(3);

    // triangle vertices
    _triangleMesh._vertices[0].position = {1.0f, 1.0f, 0.0f};
    _triangleMesh._vertices[1].position = {-1.0f, 1.0f, 0.0f};
    _triangleMesh._vertices[2].position = {0.0f, -1.0f, 0.0f};

    // vertex colors
    _triangleMesh._vertices[0].color = {1.0f, 0.0f, 0.0f};
    _triangleMesh._vertices[1].color = {0.0f, 1.0f, 0.0f};
    _triangleMesh._vertices[2].color = {0.0f, 0.0f, 1.0f};

    // load monkey
    _monkeyMesh.load_from_obj("../assets/monkey_smooth.obj");

    // upload meshes to GPU
    upload_mesh(_triangleMesh);
    upload_mesh(_monkeyMesh);

    // copying
    _meshes["monkey"] = _monkeyMesh;
    _meshes["triangle"] = _triangleMesh;
}

void VulkanEngine::upload_mesh(Mesh &mesh) {
    // define vertex buffer
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    // size of buffer in bytes
    bufferInfo.size = mesh._vertices.size() * sizeof(Vertex);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    // writable by CPU, readable by GPU
    VmaAllocationCreateInfo vmaAllocInfo = {};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    // allocate the vertex buffer and check
    VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaAllocInfo,
                             &mesh._vertexBuffer._buffer,
                             &mesh._vertexBuffer._allocation,
                             nullptr));

    _mainDeletionQueue.push_function([=]() {
        vmaDestroyBuffer(_allocator, mesh._vertexBuffer._buffer, mesh._vertexBuffer._allocation);
    });

    // copy vertex data into new buffer - map to pointer, write, unmap
    void* data;
    vmaMapMemory(_allocator, mesh._vertexBuffer._allocation, &data);
    memcpy(data, mesh._vertices.data(), mesh._vertices.size() * sizeof(Vertex));
    vmaUnmapMemory(_allocator, mesh._vertexBuffer._allocation);
}

bool VulkanEngine::load_shader_module(const char *filePath, VkShaderModule *outShaderModule) {
    // open file, cursor at end
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);

    if(!file.is_open()) {
        return false;
    }

    // get file size by checking location of cursor
    size_t fileSize = (size_t)file.tellg();
    // SPIR-V expects uint32_t buffer
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    // return cursor to beginning
    file.seekg(0);
    // read entire file into buffer
    file.read((char*)buffer.data(), fileSize);
    // done with file, clean up
    file.close();

    // describe a new shader module
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;
    // codeSize must be in bytes
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();

    // create the shader module and check - not using VK_CHECK as shader errors are common
    VkShaderModule shaderModule;
    if(vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return false;
    }
    *outShaderModule = shaderModule;
    return true;
}

Material* VulkanEngine::create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string &name) {
    Material mat;
    mat.pipeline = pipeline;
    mat.pipelineLayout = layout;
    _materials[name] = mat;
    return &_materials[name];
}

Material* VulkanEngine::get_material(const std::string &name) {
    // search for material, return nullptr if not found
    auto it = _materials.find(name);
    if(it == _materials.end()) {
        return nullptr;
    } else {
        return &(*it).second;
    }
}

Mesh* VulkanEngine::get_mesh(const std::string &name) {
    // search for mesh, return nullptr if not found
    auto it = _meshes.find(name);
    if(it == _meshes.end()) {
        return nullptr;
    } else {
        return &(*it).second;
    }
}

void VulkanEngine::init_scene() {
    RenderObject monkey;
    monkey.mesh = get_mesh("monkey");
    monkey.material = get_material("default_mesh");
    monkey.transformMatrix = glm::mat4{1.0f};
    _renderables.push_back(monkey);

    for(int x = -20; x <= 20; x++) {
        for(int y = -20; y <= 20; y++) {
            RenderObject tri;
            tri.mesh = get_mesh("triangle");
            tri.material = get_material("default_mesh");
            glm::mat4 translation = glm::translate(glm::mat4{1.0f}, glm::vec3(x, 0.0f, y));
            glm::mat4 scale = glm::scale(glm::mat4{1.0f}, glm::vec3{0.2f, 0.2f, 0.2f});
            tri.transformMatrix = translation * scale;
            _renderables.push_back(tri);
        }
    }
}

void VulkanEngine::draw_objects(VkCommandBuffer cmd, RenderObject *first, int count) {
    glm::mat4 view = glm::translate(glm::mat4(1.0f), _camPos);
    glm::mat4 projection = glm::perspective(glm::radians(70.0f), 1700.0f / 900.0f, 0.1f, 200.0f);
    projection[1][1] *= -1;

    Mesh* lastMesh = nullptr;
    Material* lastMaterial = nullptr;
    for(int i = 0; i < count; i++) {
        RenderObject& object = first[i];
        if(object.material != lastMaterial) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
            lastMaterial = object.material;
        }

        glm::mat4 model = object.transformMatrix;
        // final CPU-calculated render matrix
        glm::mat4 mesh_matrix = projection * view * model;

        MeshPushConstants constants;
        constants.render_matrix = mesh_matrix;

        // upload the matrix via push constants
        vkCmdPushConstants(cmd, object.material->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

        if(object.mesh != lastMesh) {
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &object.mesh->_vertexBuffer._buffer, &offset);
            lastMesh = object.mesh;
        }

        vkCmdDraw(cmd, object.mesh->_vertices.size(), 1, 0, 0);
    }
}

void VulkanEngine::draw() {
    // wait until previous frame is rendered, timeout 1sec
    VK_CHECK(vkWaitForFences(_device, 1, &_renderFence, true, 1000000000));
    VK_CHECK(vkResetFences(_device, 1, &_renderFence));

    // grab image from swapchain, timeout 1sec
    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, _presentSemaphore, nullptr, &swapchainImageIndex));

    // set up the command buffer, one time use
    VK_CHECK(vkResetCommandBuffer(_mainCommandBuffer, 0));
    VkCommandBuffer cmd = _mainCommandBuffer;
    VkCommandBufferBeginInfo cmdBeginInfo = {};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext = nullptr;
    cmdBeginInfo.pInheritanceInfo = nullptr;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    // derive a screen-clear color from the frame counter
    VkClearValue clearValue;
    float flash = abs(sin(_frameNumber / 120.f));
    clearValue.color = {{flash/2, flash/2, flash/2, 1.0f}};

    // clear depth
    VkClearValue depthClear;
    depthClear.depthStencil.depth = 1.0f;

    // clear values array
    VkClearValue clearValues[2] = {clearValue, depthClear};

    // start the render pass
    VkRenderPassBeginInfo rpInfo = {};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.pNext = nullptr;
    rpInfo.renderPass = _renderPass;
    rpInfo.renderArea.offset.x = 0;
    rpInfo.renderArea.offset.y = 0;
    rpInfo.renderArea.extent = _windowExtent;
    rpInfo.framebuffer = _framebuffers[swapchainImageIndex];
    rpInfo.clearValueCount = 2;
    rpInfo.pClearValues = &clearValues[0];

    uint8_t* keystate = const_cast<uint8_t *>(SDL_GetKeyboardState(nullptr));
    if(keystate[SDL_SCANCODE_W]) {
        _camPos.z += 0.1f;
    }
    if(keystate[SDL_SCANCODE_A]) {
        _camPos.x += 0.1f;
    }
    if(keystate[SDL_SCANCODE_S]) {
        _camPos.z -= 0.1f;
    }
    if(keystate[SDL_SCANCODE_D]) {
        _camPos.x -= 0.1f;
    }
    if(keystate[SDL_SCANCODE_Q]) {
        _camPos.y += 0.1f;
    }
    if(keystate[SDL_SCANCODE_E]) {
        _camPos.y -= 0.1f;
    }

    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    draw_objects(cmd, _renderables.data(), _renderables.size());
    vkCmdEndRenderPass(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd));

    // prepare to submit to queue - wait on present semaphore so swapchain is ready, signal render semaphore when we're done
    VkSubmitInfo submit = {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.pNext = nullptr;
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit.pWaitDstStageMask = &waitStage;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &_presentSemaphore;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &_renderSemaphore;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    // submit to queue and check result
    VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, _renderFence));

    // prepare to present image to screen
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &_renderSemaphore;
    presentInfo.pImageIndices = &swapchainImageIndex;

    // present image and check result
    VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

    _frameNumber++;
}

void VulkanEngine::run() {
    SDL_Event e;
    bool bQuit = false;

    while (!bQuit) {
        // handle input events
        while (SDL_PollEvent(&e) != 0) {
            // close on Alt+F4 or exit button
            if (e.type == SDL_QUIT) {
                bQuit = true;
            }
        }

        draw();
    }
}

void VulkanEngine::cleanup() {
    if (_isInitialized) {
        // block until GPU finishes
        vkDeviceWaitIdle(_device);

        // flush the deletion queue
        _mainDeletionQueue.flush();

        // manually destroy remaining objects
        vmaDestroyAllocator(_allocator);
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkDestroyDevice(_device, nullptr);
        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
        vkDestroyInstance(_instance, nullptr);

        SDL_DestroyWindow(_window);
    }
}