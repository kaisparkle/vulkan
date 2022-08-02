#define VMA_IMPLEMENTATION

#include <vk_mem_alloc.h>

#include <iostream>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <glm/gtx/transform.hpp>
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_vulkan.h>
#include <VkBootstrap.h>
#include <vk/check.h>
#include <vk/info.h>

#include "renderer.h"

namespace VkRenderer {
    void Renderer::init() {
        // initialize SDL and create a window
        SDL_Init(SDL_INIT_VIDEO);
        auto window_flags = (SDL_WindowFlags) (SDL_WINDOW_VULKAN);
        _window = SDL_CreateWindow(
                "Vulkan",
                SDL_WINDOWPOS_UNDEFINED,
                SDL_WINDOWPOS_UNDEFINED,
                _windowExtent.width,
                _windowExtent.height,
                window_flags
        );
        SDL_SetHintWithPriority(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1", SDL_HINT_OVERRIDE);
        SDL_SetRelativeMouseMode(SDL_TRUE);

        // initialize camera
        _flyCamera = new FlyCamera(glm::perspective(glm::radians(90.0f), (float) _windowExtent.width / (float) _windowExtent.height, 0.1f, 2000.0f));

        init_vulkan();
        init_swapchain();
        init_commands();
        init_default_renderpass();
        init_framebuffers();
        init_sync_structures();
        init_descriptors();
        init_materials();
        init_imgui();
        init_scene();

        _isInitialized = true;
    }

    void Renderer::init_vulkan() {
        vkb::InstanceBuilder builder;

        // create Vulkan instance with debugging features
        auto inst_ret = builder.request_validation_layers(true)
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
        VkPhysicalDeviceShaderDrawParametersFeatures shader_draw_parameters_features = {};
        shader_draw_parameters_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
        shader_draw_parameters_features.pNext = nullptr;
        shader_draw_parameters_features.shaderDrawParameters = VK_TRUE;
        vkb::Device vkbDevice = deviceBuilder.add_pNext(&shader_draw_parameters_features).build().value();

        // set the device handles
        _device = vkbDevice.device;
        _chosenGPU = physicalDevice.physical_device;
        _gpuProperties = vkbDevice.physical_device.properties;

        std::cout << "The GPU has a minimum buffer alignment of " << _gpuProperties.limits.minUniformBufferOffsetAlignment << std::endl;

        // get a graphics queue
        _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
        _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

        // initialize memory allocator
        VmaAllocatorCreateInfo allocatorInfo = VkRenderer::info::allocator_create_info(_chosenGPU, _device, _instance);
        vmaCreateAllocator(&allocatorInfo, &_allocator);
    }

    void Renderer::init_swapchain() {
        // set up a swapchain
        vkb::SwapchainBuilder swapchainBuilder{_chosenGPU, _device, _surface};
        vkb::Swapchain vkbSwapchain = swapchainBuilder
                .use_default_format_selection()
                .set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR)
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
        VkImageCreateInfo depthImageInfo = VkRenderer::info::image_create_info(_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

        // allocate memory on GPU only
        VmaAllocationCreateInfo depthImageAllocInfo = VkRenderer::info::allocation_create_info(VMA_MEMORY_USAGE_GPU_ONLY,
                                                                                               VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
        vmaCreateImage(_allocator, &depthImageInfo, &depthImageAllocInfo, &_depthImage._image, &_depthImage._allocation, nullptr);

        // build image view for depth image
        VkImageViewCreateInfo depthViewInfo = VkRenderer::info::imageview_create_info(_depthFormat, _depthImage._image, VK_IMAGE_ASPECT_DEPTH_BIT);
        VK_CHECK(vkCreateImageView(_device, &depthViewInfo, nullptr, &_depthImageView));

        _mainDeletionQueue.push_function([=]() {
            vkDestroyImageView(_device, _depthImageView, nullptr);
            vmaDestroyImage(_allocator, _depthImage._image, _depthImage._allocation);
            vkDestroySwapchainKHR(_device, _swapchain, nullptr);
        });
    }

    void Renderer::init_commands() {
        // create command pools for each frame in flight, allow resettable command buffers
        VkCommandPoolCreateInfo commandPoolInfo = VkRenderer::info::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        for (auto &_frame: _frames) {
            VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frame._commandPool));
            // allocate default command buffer
            VkCommandBufferAllocateInfo cmdAllocInfo = VkRenderer::info::command_buffer_allocate_info(_frame._commandPool, 1);
            VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frame._mainCommandBuffer));

            _mainDeletionQueue.push_function([=]() {
                vkDestroyCommandPool(_device, _frame._commandPool, nullptr);
            });
        }

        // create a command pool for upload context
        VkCommandPoolCreateInfo uploadCommandPoolInfo = VkRenderer::info::command_pool_create_info(_graphicsQueueFamily);
        VK_CHECK(vkCreateCommandPool(_device, &uploadCommandPoolInfo, nullptr, &_uploadContext._commandPool));
        _mainDeletionQueue.push_function([=]() {
            vkDestroyCommandPool(_device, _uploadContext._commandPool, nullptr);
        });

        // allocate command buffer too
        VkCommandBufferAllocateInfo uploadCmdAllocInfo = VkRenderer::info::command_buffer_allocate_info(_uploadContext._commandPool, 1);
        VK_CHECK(vkAllocateCommandBuffers(_device, &uploadCmdAllocInfo, &_uploadContext._commandBuffer));
    }

    void Renderer::init_default_renderpass() {
        // describe color attachment for renderpass
        VkAttachmentDescription color_attachment = VkRenderer::info::attachment_description(_swapchainImageFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                                                            VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                                                                            VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED,
                                                                                            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        VkAttachmentReference color_attachment_ref = VkRenderer::info::attachment_reference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        // depth attachment
        VkAttachmentDescription depth_attachment = VkRenderer::info::attachment_description(_depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                                                            VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                                                            VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED,
                                                                                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        VkAttachmentReference depth_attachment_ref = VkRenderer::info::attachment_reference(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

        // describe one subpass, minimum possible
        VkSubpassDescription subpass = VkRenderer::info::subpass_description(VK_PIPELINE_BIND_POINT_GRAPHICS, 1, &color_attachment_ref, &depth_attachment_ref);

        // subpass dependencies for sync
        VkSubpassDependency color_dependency = VkRenderer::info::subpass_dependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
                                                                                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
        VkSubpassDependency depth_dependency = VkRenderer::info::subpass_dependency(VK_SUBPASS_EXTERNAL, 0,
                                                                                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0,
                                                                                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                                                                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

        // attachment and dependency arrays
        VkAttachmentDescription attachments[2] = {color_attachment, depth_attachment};
        VkSubpassDependency dependencies[2] = {color_dependency, depth_dependency};

        // describe the render pass, connect attachments and subpass
        VkRenderPassCreateInfo render_pass_info = VkRenderer::info::renderpass_create_info(2, &attachments[0], 2, &dependencies[0], 1, &subpass);
        VK_CHECK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_renderPass));

        _mainDeletionQueue.push_function([=]() {
            vkDestroyRenderPass(_device, _renderPass, nullptr);
        });
    }

    void Renderer::init_framebuffers() {
        // create framebuffers for each swapchain image
        VkFramebufferCreateInfo fbInfo = VkRenderer::info::framebuffer_create_info(_renderPass, 0, nullptr, _windowExtent.width, _windowExtent.height, 1);

        const uint32_t swapchain_imagecount = _swapchainImages.size();
        _framebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

        for (size_t i = 0; i < swapchain_imagecount; i++) {
            VkImageView attachments[2];
            attachments[0] = _swapchainImageViews[i];
            attachments[1] = _depthImageView;

            fbInfo.attachmentCount = 2;
            fbInfo.pAttachments = attachments;

            VK_CHECK(vkCreateFramebuffer(_device, &fbInfo, nullptr, &_framebuffers[i]));

            _mainDeletionQueue.push_function([=]() {
                vkDestroyFramebuffer(_device, _framebuffers[i], nullptr);
                vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
            });
        }
    }

    void Renderer::init_sync_structures() {
        // create sync structures for each frame in flight
        VkFenceCreateInfo fenceCreateInfo = VkRenderer::info::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
        VkSemaphoreCreateInfo semaphoreCreateInfo = VkRenderer::info::semaphore_create_info();
        for (auto &_frame: _frames) {
            VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frame._renderFence));
            _mainDeletionQueue.push_function([=]() {
                vkDestroyFence(_device, _frame._renderFence, nullptr);
            });

            VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frame._presentSemaphore));
            VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frame._renderSemaphore));
            _mainDeletionQueue.push_function([=]() {
                vkDestroySemaphore(_device, _frame._presentSemaphore, nullptr);
                vkDestroySemaphore(_device, _frame._renderSemaphore, nullptr);
            });
        }

        // create fence for upload context
        VkFenceCreateInfo uploadFenceCreateInfo = VkRenderer::info::fence_create_info();
        VK_CHECK(vkCreateFence(_device, &uploadFenceCreateInfo, nullptr, &_uploadContext._uploadFence));
        _mainDeletionQueue.push_function([=]() {
            vkDestroyFence(_device, _uploadContext._uploadFence, nullptr);
        });
    }

    void Renderer::init_descriptors() {
        // create the layout cache
        _descriptorLayoutCache = new VkRenderer::descriptor::LayoutCache{};
        _descriptorLayoutCache->init(_device);

        // create global set layout
        VkDescriptorSetLayoutBinding camBufferBinding = VkRenderer::descriptor::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
        VkDescriptorSetLayoutBinding sceneBufferBinding = VkRenderer::descriptor::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                                                                                                                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);
        VkDescriptorSetLayoutBinding bindings[] = {camBufferBinding, sceneBufferBinding};
        VkDescriptorSetLayoutCreateInfo globalLayoutInfo = VkRenderer::info::descriptor_set_layout_create_info(2, bindings, 0);
        _globalSetLayout = _descriptorLayoutCache->create_descriptor_layout(&globalLayoutInfo);

        // create scene parameter buffer
        const size_t sceneParamBufferSize = FRAME_OVERLAP * pad_uniform_buffer_size(sizeof(GPUSceneData));
        _sceneParameterBuffer = create_buffer(sceneParamBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        _mainDeletionQueue.push_function([=]() {
            vmaDestroyBuffer(_allocator, _sceneParameterBuffer._buffer, _sceneParameterBuffer._allocation);
        });

        // set up buffers for each frame in flight
        for (auto &_frame: _frames) {
            // create descriptor allocators
            _frame._descriptorAllocator = new VkRenderer::descriptor::Allocator{};
            _frame._descriptorAllocator->init(_device);

            // create camera buffer
            _frame.cameraBuffer = create_buffer(sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
            _mainDeletionQueue.push_function([=]() {
                vmaDestroyBuffer(_allocator, _frame.cameraBuffer._buffer, _frame.cameraBuffer._allocation);
            });
        }
    }

    void Renderer::init_materials() {
        // create default material
        VkDescriptorSetLayout setLayouts[] = {_globalSetLayout};
        MaterialCreateInfo materialInfo = {};
        materialInfo.vertShaderPath = "../shaders/default_mesh.vert.spv";
        materialInfo.fragShaderPath = "../shaders/default_lit.frag.spv";
        materialInfo.setLayoutCount = 1;
        materialInfo.setLayouts = setLayouts;
        materialInfo.device = _device;
        materialInfo.extent = _windowExtent;
        materialInfo.renderPass = _renderPass;
        materialInfo.name = "default_mesh";
        _materialManager.create_material(&materialInfo);

        // reuse info struct and create static material
        materialInfo.fragShaderPath = "../shaders/static.frag.spv";
        materialInfo.name = "static";
        _materialManager.create_material(&materialInfo);

        _mainDeletionQueue.push_function([=]() {
            _materialManager.cleanup(_device);
        });
    }

    void Renderer::init_imgui() {
        // create a big descriptor pool for imgui directly
        VkDescriptorPoolSize poolSizes[] = {
                {VK_DESCRIPTOR_TYPE_SAMPLER,                1000},
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1000},
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1000},
                {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1000},
                {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1000},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1000},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1000},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1000}
        };
        VkDescriptorPoolCreateInfo poolInfo = VkRenderer::info::descriptor_pool_create_info(1000, std::size(poolSizes), poolSizes,
                                                                                            VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
        VkDescriptorPool imguiPool;
        VK_CHECK(vkCreateDescriptorPool(_device, &poolInfo, nullptr, &imguiPool));
        _mainDeletionQueue.push_function([=]() {
            vkDestroyDescriptorPool(_device, imguiPool, nullptr);
        });

        // initialize imgui for SDL and Vulkan
        ImGui::CreateContext();
        ImGui_ImplSDL2_InitForVulkan(_window);
        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance = _instance;
        initInfo.PhysicalDevice = _chosenGPU;
        initInfo.Device = _device;
        initInfo.Queue = _graphicsQueue;
        initInfo.DescriptorPool = imguiPool;
        initInfo.MinImageCount = 3;
        initInfo.ImageCount = 3;
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        ImGui_ImplVulkan_Init(&initInfo, _renderPass);

        // upload font textures to GPU
        immediate_submit([&](VkCommandBuffer cmd) {
            ImGui_ImplVulkan_CreateFontsTexture(cmd);
        });
        ImGui_ImplVulkan_DestroyFontUploadObjects();

        // initialization done
        _mainDeletionQueue.push_function([=]() {
            ImGui_ImplVulkan_Shutdown();
        });
    }

    void Renderer::init_scene() {
        _modelManager.create_model("../assets/SciFiHelmet.gltf", "helmet", _materialManager.get_material("default_mesh"), _allocator, _mainDeletionQueue);
        _modelManager.create_model("../assets/sponza-gltf-pbr/sponza.glb", "sponza", _materialManager.get_material("default_mesh"), _allocator, _mainDeletionQueue);
        _modelManager.models["sponza"].set_model_matrix(glm::scale(glm::mat4{1.0f}, glm::vec3(0.1f, 0.1f, 0.1f)));
        _modelManager.models["helmet"].set_model_matrix(glm::scale(glm::mat4{1.0f}, glm::vec3(10.0f, 10.0f, 10.0f)));
    }

    AllocatedBuffer Renderer::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
        // allocate buffer
        VkBufferCreateInfo bufferInfo = VkRenderer::info::buffer_create_info(allocSize, usage);
        VmaAllocationCreateInfo vmaAllocInfo = VkRenderer::info::allocation_create_info(memoryUsage, 0);
        AllocatedBuffer newBuffer;
        VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaAllocInfo, &newBuffer._buffer, &newBuffer._allocation, nullptr));
        return newBuffer;
    }

    size_t Renderer::pad_uniform_buffer_size(size_t originalSize) {
        // calculate alignment
        size_t minUboAlignment = _gpuProperties.limits.minUniformBufferOffsetAlignment;
        size_t alignedSize = originalSize;
        if (minUboAlignment > 0) {
            alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
        }

        return alignedSize;
    }

    void Renderer::immediate_submit(std::function<void(VkCommandBuffer cmd)> &&function) {
        // begin buffer recording
        VkCommandBuffer cmd = _uploadContext._commandBuffer;
        VkCommandBufferBeginInfo cmdBeginInfo = VkRenderer::info::command_buffer_begin_info(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        // execute passed function
        function(cmd);

        // end recording and submit
        VK_CHECK(vkEndCommandBuffer(cmd));
        VkSubmitInfo submitInfo = VkRenderer::info::submit_info(nullptr, 0, nullptr, 0, nullptr, 1, &cmd);
        VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _uploadContext._uploadFence));

        // block on fence
        vkWaitForFences(_device, 1, &_uploadContext._uploadFence, true, 1000000000);
        vkResetFences(_device, 1, &_uploadContext._uploadFence);

        // reset pool
        vkResetCommandPool(_device, _uploadContext._commandPool, 0);
    }

    FrameData &Renderer::get_current_frame() {
        return _frames[_frameNumber % FRAME_OVERLAP];
    }

    void Renderer::draw_objects(VkCommandBuffer cmd) {
        // set up camera parameters and copy
        GPUCameraData camData;
        camData.proj = _flyCamera->_projection;
        camData.view = _flyCamera->get_view_matrix();
        camData.viewproj = _flyCamera->_projection * _flyCamera->get_view_matrix();
        void *data;
        vmaMapMemory(_allocator, get_current_frame().cameraBuffer._allocation, &data);
        memcpy(data, &camData, sizeof(GPUCameraData));
        vmaUnmapMemory(_allocator, get_current_frame().cameraBuffer._allocation);

        // set up scene parameters and copy
        _sceneParameters.ambientColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        char *sceneData;
        vmaMapMemory(_allocator, _sceneParameterBuffer._allocation, (void **) &sceneData);
        int frameIndex = _frameNumber % FRAME_OVERLAP;
        sceneData += pad_uniform_buffer_size(sizeof(GPUSceneData)) * frameIndex;
        memcpy(sceneData, &_sceneParameters, sizeof(GPUSceneData));
        vmaUnmapMemory(_allocator, _sceneParameterBuffer._allocation);

        // get buffer infos and build sets
        VkDescriptorBufferInfo camBufferInfo = VkRenderer::info::descriptor_buffer_info(get_current_frame().cameraBuffer._buffer, 0, sizeof(GPUCameraData));
        VkDescriptorBufferInfo sceneBufferInfo = VkRenderer::info::descriptor_buffer_info(_sceneParameterBuffer._buffer, 0, sizeof(GPUSceneData));
        VkDescriptorSet globalSet;
        VkRenderer::descriptor::Builder::begin(_descriptorLayoutCache, get_current_frame()._descriptorAllocator)
                .bind_buffer(0, &camBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
                .bind_buffer(1, &sceneBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
                .build(globalSet);

        Material *defaultMaterial = _materialManager.get_material("default_mesh");
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, defaultMaterial->pipeline);
        // calculate scene buffer offset and bind global set
        uint32_t uniform_offset = pad_uniform_buffer_size(sizeof(GPUSceneData)) * frameIndex;
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, defaultMaterial->pipelineLayout, 0, 1, &globalSet, 1, &uniform_offset);

        for (auto &it: _modelManager.models) {
            it.second.draw_model(cmd);
        }
    }

    void Renderer::draw() {
        // start the GUI frame
        ImGui::Render();

        // wait until previous frame is rendered, timeout 1sec
        VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000));
        VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

        // start rendering the next frame
        auto frameTimerStart = std::chrono::high_resolution_clock::now();

        // check for camera movement - use previous frametime as a delta
        _flyCamera->process_keyboard(_previousFrameTime);

        // grab image from swapchain, timeout 1sec
        uint32_t swapchainImageIndex;
        VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, get_current_frame()._presentSemaphore, nullptr, &swapchainImageIndex));

        // reset the command buffer
        VK_CHECK(vkResetCommandBuffer(get_current_frame()._mainCommandBuffer, 0));
        get_current_frame()._descriptorAllocator->reset_pools();

        // start new command buffer
        VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;
        VkCommandBufferBeginInfo cmdBeginInfo = VkRenderer::info::command_buffer_begin_info(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        // clear screen to black
        VkClearValue clearValue;
        clearValue.color = {0.0f, 0.0f, 0.0f, 1.0f};

        // clear depth
        VkClearValue depthClear;
        depthClear.depthStencil.depth = 1.0f;

        // clear values array
        VkClearValue clearValues[2] = {clearValue, depthClear};

        // start the render pass
        VkRenderPassBeginInfo rpInfo = VkRenderer::info::renderpass_begin_info(_renderPass, 0, 0, _windowExtent, _framebuffers[swapchainImageIndex], 2, &clearValues[0]);
        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

        // draw scene and GUI
        draw_objects(cmd);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

        vkCmdEndRenderPass(cmd);
        VK_CHECK(vkEndCommandBuffer(cmd));

        // submit to queue and check result - wait on present semaphore so swapchain is ready, signal render semaphore when we're done
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit = VkRenderer::info::submit_info(&waitStage, 1, &get_current_frame()._presentSemaphore, 1,
                                                            &get_current_frame()._renderSemaphore, 1, &cmd);
        VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, get_current_frame()._renderFence));

        // present image and check result
        VkPresentInfoKHR presentInfo = VkRenderer::info::present_info(1, &_swapchain, 1, &get_current_frame()._renderSemaphore, &swapchainImageIndex);
        VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

        // end the frame
        _frameNumber++;
        auto frameTimerEnd = std::chrono::high_resolution_clock::now();

        // convert the timer to double and store as previous time
        std::chrono::duration<double, std::milli> frameDuration = frameTimerEnd - frameTimerStart;
        _previousFrameTime = frameDuration.count();
    }

    void Renderer::run() {
        SDL_Event e;
        bool bQuit = false;

        while (!bQuit) {
            // disable mouse capture and cursor hide if UI is toggled
            if (_toggleUI) {
                SDL_SetHintWithPriority(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "0", SDL_HINT_OVERRIDE);
                SDL_SetRelativeMouseMode(SDL_FALSE);
            } else {
                SDL_SetHintWithPriority(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1", SDL_HINT_OVERRIDE);
                SDL_SetRelativeMouseMode(SDL_TRUE);
            }

            // handle input events
            while (SDL_PollEvent(&e) != 0) {
                if(_toggleUI) ImGui_ImplSDL2_ProcessEvent(&e);
                // close on Alt+F4 or exit button
                if (e.type == SDL_QUIT) {
                    bQuit = true;
                }
                // enable or disable camera to use UI
                if (e.type == SDL_KEYDOWN) {
                    if(e.key.keysym.sym == SDLK_TAB) {
                        _toggleUI = !_toggleUI;
                    }
                }
                // capture mouse movement and process
                if(e.type == SDL_MOUSEMOTION && !_toggleUI) {
                    _flyCamera->process_mouse(e.motion.xrel, e.motion.yrel);
                }
            }

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL2_NewFrame(_window);
            ImGui::NewFrame();
            ImGui::ShowDemoWindow();

            draw();
        }
    }

    void Renderer::cleanup() {
        if (_isInitialized) {
            // block until GPU finishes
            vkDeviceWaitIdle(_device);

            // flush the deletion queue
            _mainDeletionQueue.flush();

            // clean up caches
            for (auto &_frame: _frames) {
                _frame._descriptorAllocator->cleanup();
            }
            _descriptorLayoutCache->cleanup();

            // manually destroy remaining objects
            vmaDestroyAllocator(_allocator);
            vkDestroySurfaceKHR(_instance, _surface, nullptr);
            vkDestroyDevice(_device, nullptr);
            vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
            vkDestroyInstance(_instance, nullptr);

            SDL_DestroyWindow(_window);
        }
    }
}