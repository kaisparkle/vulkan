#define VMA_IMPLEMENTATION

#include <vk_mem_alloc.h>

#include <iostream>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <glm/gtx/transform.hpp>
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_vulkan.h>
#include <implot.h>
#include <VkBootstrap.h>
#include <vk/check.h>
#include <vk/info.h>
#include <vk/utils.h>

#include "renderer.h"

namespace VkRenderer {
    void Renderer::init() {
        // initialize SDL and create a window
        SDL_Init(SDL_INIT_VIDEO);
        auto window_flags = (SDL_WindowFlags) (SDL_WINDOW_VULKAN);
        _resources.window = SDL_CreateWindow(
                "Vulkan",
                SDL_WINDOWPOS_UNDEFINED,
                SDL_WINDOWPOS_UNDEFINED,
                _resources.windowExtent.width,
                _resources.windowExtent.height,
                window_flags
        );
        SDL_SetHintWithPriority(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1", SDL_HINT_OVERRIDE);
        SDL_SetRelativeMouseMode(SDL_TRUE);

        // initialize camera
        _resources.flyCamera = new FlyCamera(
                glm::perspective(glm::radians(90.0f), (float) _resources.windowExtent.width / (float) _resources.windowExtent.height, 0.1f, 2000.0f));

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
        _resources.instance = vkb_inst.instance;
        _resources.debug_messenger = vkb_inst.debug_messenger;

        // grab SDL window surface
        SDL_Vulkan_CreateSurface(_resources.window, _resources.instance, &_resources.surface);

        // just pick a GPU that supports Vulkan 1.1
        vkb::PhysicalDeviceSelector selector{vkb_inst};
        vkb::PhysicalDevice physicalDevice = selector
                .set_minimum_version(1, 1)
                .set_surface(_resources.surface)
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
        _resources.device = vkbDevice.device;
        _resources.chosenGPU = physicalDevice.physical_device;
        _resources.gpuProperties = vkbDevice.physical_device.properties;

        std::cout << "The GPU has a minimum buffer alignment of " << _resources.gpuProperties.limits.minUniformBufferOffsetAlignment << std::endl;

        // get a graphics queue
        _resources.graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
        _resources.graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

        // initialize memory allocator
        VmaAllocatorCreateInfo allocatorInfo = VkRenderer::info::allocator_create_info(_resources.chosenGPU, _resources.device, _resources.instance);
        vmaCreateAllocator(&allocatorInfo, &_resources.allocator);
    }

    void Renderer::init_swapchain() {
        // set up a swapchain
        vkb::SwapchainBuilder swapchainBuilder{_resources.chosenGPU, _resources.device, _resources.surface};
        vkb::Swapchain vkbSwapchain = swapchainBuilder
                .use_default_format_selection()
                .set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR)
                .set_desired_extent(_resources.windowExtent.width, _resources.windowExtent.height)
                .build()
                .value();

        // set swapchain and image handles
        _resources.swapchain = vkbSwapchain.swapchain;
        _resources.swapchainImages = vkbSwapchain.get_images().value();
        _resources.swapchainImageViews = vkbSwapchain.get_image_views().value();
        _resources.swapchainImageFormat = vkbSwapchain.image_format;

        // set up depth image
        VkExtent3D depthImageExtent = {
                _resources.windowExtent.width,
                _resources.windowExtent.height,
                1
        };
        _resources.depthFormat = VK_FORMAT_D32_SFLOAT;
        VkImageCreateInfo depthImageInfo = VkRenderer::info::image_create_info(_resources.depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

        // allocate memory on GPU only
        VmaAllocationCreateInfo depthImageAllocInfo = VkRenderer::info::allocation_create_info(VMA_MEMORY_USAGE_GPU_ONLY,
                                                                                               VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
        vmaCreateImage(_resources.allocator, &depthImageInfo, &depthImageAllocInfo, &_resources.depthImage._image, &_resources.depthImage._allocation, nullptr);

        // build image view for depth image
        VkImageViewCreateInfo depthViewInfo = VkRenderer::info::imageview_create_info(_resources.depthFormat, _resources.depthImage._image, VK_IMAGE_ASPECT_DEPTH_BIT);
        VK_CHECK(vkCreateImageView(_resources.device, &depthViewInfo, nullptr, &_resources.depthImageView));

        _resources.mainDeletionQueue.push_function([=]() {
            vkDestroyImageView(_resources.device, _resources.depthImageView, nullptr);
            vmaDestroyImage(_resources.allocator, _resources.depthImage._image, _resources.depthImage._allocation);
            vkDestroySwapchainKHR(_resources.device, _resources.swapchain, nullptr);
        });
    }

    void Renderer::init_commands() {
        // create command pools for each frame in flight, allow resettable command buffers
        VkCommandPoolCreateInfo commandPoolInfo = VkRenderer::info::command_pool_create_info(_resources.graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        for (auto &_frame: _frames) {
            VK_CHECK(vkCreateCommandPool(_resources.device, &commandPoolInfo, nullptr, &_frame._commandPool));
            // allocate default command buffer
            VkCommandBufferAllocateInfo cmdAllocInfo = VkRenderer::info::command_buffer_allocate_info(_frame._commandPool, 1);
            VK_CHECK(vkAllocateCommandBuffers(_resources.device, &cmdAllocInfo, &_frame._mainCommandBuffer));

            _resources.mainDeletionQueue.push_function([=]() {
                vkDestroyCommandPool(_resources.device, _frame._commandPool, nullptr);
            });
        }

        // create a command pool for upload context
        VkCommandPoolCreateInfo uploadCommandPoolInfo = VkRenderer::info::command_pool_create_info(_resources.graphicsQueueFamily);
        VK_CHECK(vkCreateCommandPool(_resources.device, &uploadCommandPoolInfo, nullptr, &_resources.uploadContext._commandPool));
        _resources.mainDeletionQueue.push_function([=]() {
            vkDestroyCommandPool(_resources.device, _resources.uploadContext._commandPool, nullptr);
        });

        // allocate command buffer too
        VkCommandBufferAllocateInfo uploadCmdAllocInfo = VkRenderer::info::command_buffer_allocate_info(_resources.uploadContext._commandPool, 1);
        VK_CHECK(vkAllocateCommandBuffers(_resources.device, &uploadCmdAllocInfo, &_resources.uploadContext._commandBuffer));
    }

    void Renderer::init_default_renderpass() {
        // describe color attachment for renderpass
        VkAttachmentDescription color_attachment = VkRenderer::info::attachment_description(_resources.swapchainImageFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                                                            VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                                                                            VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED,
                                                                                            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        VkAttachmentReference color_attachment_ref = VkRenderer::info::attachment_reference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        // depth attachment
        VkAttachmentDescription depth_attachment = VkRenderer::info::attachment_description(_resources.depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR,
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
        VK_CHECK(vkCreateRenderPass(_resources.device, &render_pass_info, nullptr, &_resources.renderPass));

        _resources.mainDeletionQueue.push_function([=]() {
            vkDestroyRenderPass(_resources.device, _resources.renderPass, nullptr);
        });
    }

    void Renderer::init_framebuffers() {
        // create framebuffers for each swapchain image
        VkFramebufferCreateInfo fbInfo = VkRenderer::info::framebuffer_create_info(_resources.renderPass, 0, nullptr, _resources.windowExtent.width, _resources.windowExtent.height,
                                                                                   1);

        const uint32_t swapchain_imagecount = _resources.swapchainImages.size();
        _resources.framebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

        for (size_t i = 0; i < swapchain_imagecount; i++) {
            VkImageView attachments[2];
            attachments[0] = _resources.swapchainImageViews[i];
            attachments[1] = _resources.depthImageView;

            fbInfo.attachmentCount = 2;
            fbInfo.pAttachments = attachments;

            VK_CHECK(vkCreateFramebuffer(_resources.device, &fbInfo, nullptr, &_resources.framebuffers[i]));

            _resources.mainDeletionQueue.push_function([=]() {
                vkDestroyFramebuffer(_resources.device, _resources.framebuffers[i], nullptr);
                vkDestroyImageView(_resources.device, _resources.swapchainImageViews[i], nullptr);
            });
        }
    }

    void Renderer::init_sync_structures() {
        // create sync structures for each frame in flight
        VkFenceCreateInfo fenceCreateInfo = VkRenderer::info::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
        VkSemaphoreCreateInfo semaphoreCreateInfo = VkRenderer::info::semaphore_create_info();
        for (auto &_frame: _frames) {
            VK_CHECK(vkCreateFence(_resources.device, &fenceCreateInfo, nullptr, &_frame._renderFence));
            _resources.mainDeletionQueue.push_function([=]() {
                vkDestroyFence(_resources.device, _frame._renderFence, nullptr);
            });

            VK_CHECK(vkCreateSemaphore(_resources.device, &semaphoreCreateInfo, nullptr, &_frame._presentSemaphore));
            VK_CHECK(vkCreateSemaphore(_resources.device, &semaphoreCreateInfo, nullptr, &_frame._renderSemaphore));
            _resources.mainDeletionQueue.push_function([=]() {
                vkDestroySemaphore(_resources.device, _frame._presentSemaphore, nullptr);
                vkDestroySemaphore(_resources.device, _frame._renderSemaphore, nullptr);
            });
        }

        // create fence for upload context
        VkFenceCreateInfo uploadFenceCreateInfo = VkRenderer::info::fence_create_info();
        VK_CHECK(vkCreateFence(_resources.device, &uploadFenceCreateInfo, nullptr, &_resources.uploadContext._uploadFence));
        _resources.mainDeletionQueue.push_function([=]() {
            vkDestroyFence(_resources.device, _resources.uploadContext._uploadFence, nullptr);
        });
    }

    void Renderer::init_descriptors() {
        // create the layout cache
        _resources.descriptorLayoutCache = new VkRenderer::descriptor::LayoutCache{};
        _resources.descriptorLayoutCache->init(_resources.device);

        // create global set layout
        VkDescriptorSetLayoutBinding camBufferBinding = VkRenderer::descriptor::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0);
        VkDescriptorSetLayoutCreateInfo globalLayoutInfo = VkRenderer::info::descriptor_set_layout_create_info(1, &camBufferBinding, 0);
        _resources.globalSetLayout = _resources.descriptorLayoutCache->create_descriptor_layout(&globalLayoutInfo);

        // create texture set layout
        VkDescriptorSetLayoutBinding textureBind = VkRenderer::descriptor::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,
                                                                                                         0);
        VkDescriptorSetLayoutCreateInfo textureLayoutInfo = VkRenderer::info::descriptor_set_layout_create_info(1, &textureBind, 0);
        _resources.textureSetLayout = _resources.descriptorLayoutCache->create_descriptor_layout(&textureLayoutInfo);

        VkDescriptorSetLayoutBinding pbrBind1 = VkRenderer::descriptor::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
        VkDescriptorSetLayoutBinding pbrBind2 = VkRenderer::descriptor::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
        VkDescriptorSetLayoutBinding pbrBind3 = VkRenderer::descriptor::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2);
        VkDescriptorSetLayoutBinding pbrBind4 = VkRenderer::descriptor::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3);
        VkDescriptorSetLayoutBinding pbrBinds[] = {pbrBind1, pbrBind2, pbrBind3, pbrBind4};
        VkDescriptorSetLayoutCreateInfo pbrLayoutInfo = VkRenderer::info::descriptor_set_layout_create_info(4, pbrBinds, 0);
        _resources.pbrSetLayout = _resources.descriptorLayoutCache->create_descriptor_layout(&pbrLayoutInfo);

        // set up buffers for each frame in flight
        for (auto &_frame: _frames) {
            // create descriptor allocators
            _frame._descriptorAllocator = new VkRenderer::descriptor::Allocator{};
            _frame._descriptorAllocator->init(_resources.device);

            // create camera buffer
            _frame.cameraBuffer = VkRenderer::utils::create_buffer(_resources.allocator, sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
            _resources.mainDeletionQueue.push_function([=]() {
                vmaDestroyBuffer(_resources.allocator, _frame.cameraBuffer._buffer, _frame.cameraBuffer._allocation);
            });
        }
    }

    void Renderer::init_materials() {
        // create default material
        VkDescriptorSetLayout defaultSetLayouts[] = {_resources.globalSetLayout};
        MaterialCreateInfo defaultMaterialInfo = {};
        defaultMaterialInfo.vertShaderPath = "../shaders/default_mesh.vert.spv";
        defaultMaterialInfo.fragShaderPath = "../shaders/default_lit.frag.spv";
        defaultMaterialInfo.setLayoutCount = 1;
        defaultMaterialInfo.setLayouts = defaultSetLayouts;
        defaultMaterialInfo.device = _resources.device;
        defaultMaterialInfo.extent = _resources.windowExtent;
        defaultMaterialInfo.renderPass = _resources.renderPass;
        defaultMaterialInfo.name = "default_mesh";
        _materialManager.create_material(&defaultMaterialInfo);

        // reuse info struct and create static material
        defaultMaterialInfo.fragShaderPath = "../shaders/static.frag.spv";
        defaultMaterialInfo.name = "static";
        _materialManager.create_material(&defaultMaterialInfo);

        // create textured material
        VkDescriptorSetLayout texturedSetLayouts[] = {_resources.globalSetLayout, _resources.textureSetLayout};
        MaterialCreateInfo texturedMaterialInfo = {};
        texturedMaterialInfo.vertShaderPath = "../shaders/default_mesh.vert.spv";
        texturedMaterialInfo.fragShaderPath = "../shaders/textured_lit.frag.spv";
        texturedMaterialInfo.setLayoutCount = 2;
        texturedMaterialInfo.setLayouts = texturedSetLayouts;
        texturedMaterialInfo.device = _resources.device;
        texturedMaterialInfo.extent = _resources.windowExtent;
        texturedMaterialInfo.renderPass = _resources.renderPass;
        texturedMaterialInfo.name = "textured_mesh";
        _materialManager.create_material(&texturedMaterialInfo);

        // create pbr material
        VkDescriptorSetLayout pbrSetLayouts[] = {_resources.globalSetLayout, _resources.pbrSetLayout};
        MaterialCreateInfo pbrMaterialInfo = {};
        pbrMaterialInfo.vertShaderPath = "../shaders/default_mesh.vert.spv";
        pbrMaterialInfo.fragShaderPath = "../shaders/pbr.frag.spv";
        pbrMaterialInfo.setLayoutCount = 2;
        pbrMaterialInfo.setLayouts = pbrSetLayouts;
        pbrMaterialInfo.device = _resources.device;
        pbrMaterialInfo.extent = _resources.windowExtent;
        pbrMaterialInfo.renderPass = _resources.renderPass;
        pbrMaterialInfo.name = "pbr";
        _materialManager.create_material(&pbrMaterialInfo);

        _resources.mainDeletionQueue.push_function([=]() {
            _materialManager.cleanup(_resources.device);
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
        VK_CHECK(vkCreateDescriptorPool(_resources.device, &poolInfo, nullptr, &imguiPool));
        _resources.mainDeletionQueue.push_function([=]() {
            vkDestroyDescriptorPool(_resources.device, imguiPool, nullptr);
        });

        // initialize imgui for SDL and Vulkan
        ImGui::CreateContext();
        ImPlot::CreateContext();
        ImGui_ImplSDL2_InitForVulkan(_resources.window);
        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance = _resources.instance;
        initInfo.PhysicalDevice = _resources.chosenGPU;
        initInfo.Device = _resources.device;
        initInfo.Queue = _resources.graphicsQueue;
        initInfo.DescriptorPool = imguiPool;
        initInfo.MinImageCount = 3;
        initInfo.ImageCount = 3;
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        ImGui_ImplVulkan_Init(&initInfo, _resources.renderPass);
        ImGui::StyleColorsDark();

        // upload font textures to GPU
        VkRenderer::utils::immediate_submit(&_resources, [&](VkCommandBuffer cmd) {
            ImGui_ImplVulkan_CreateFontsTexture(cmd);
        });
        ImGui_ImplVulkan_DestroyFontUploadObjects();

        // initialization done
        _resources.mainDeletionQueue.push_function([=]() {
            ImGui_ImplVulkan_Shutdown();
        });
    }

    void Renderer::init_scene() {
        _modelManager.init(&_resources);

        _modelManager.create_model("../assets/sponza-gltf-pbr/sponza.glb", "sponza", _materialManager.get_material("pbr"));
        _modelManager.models["sponza"].scale[0] = 0.1f;
        _modelManager.models["sponza"].scale[1] = 0.1f;
        _modelManager.models["sponza"].scale[2] = 0.1f;

        _modelManager.create_model("../assets/SciFiHelmet.gltf", "helmet", _materialManager.get_material("pbr"));

    }

    FrameData &Renderer::get_current_frame() {
        return _frames[_frameNumber % FRAME_OVERLAP];
    }

    void Renderer::update_ui() {
        // frametime plot
        static ScrollingBuffer sdata;
        static float t = 0;
        t += ImGui::GetIO().DeltaTime;
        sdata.AddPoint(t, _previousFrameTime);
        static float history = 5.0f;

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                       ImGuiWindowFlags_NoNav;
        ImVec2 frametimeWindowPos = {0.0f + 10.0f, 0.0f + 10.0f};
        ImGui::SetNextWindowPos(frametimeWindowPos);
        ImGui::Begin("Frametime Plot", nullptr, windowFlags);
        ImGui::PushItemWidth(500);
        ImGui::SliderFloat("##History", &history, 1, 15, "%.1f s");

        if (ImPlot::BeginPlot("##Frametime Plot", ImVec2(500, 150))) {
            ImPlot::SetupAxes(nullptr, nullptr);
            ImPlot::SetupAxisLimits(ImAxis_X1, t - history, t, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 5);
            ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
            ImPlot::PlotLine("Frametime (ms)", &sdata.Data[0].x, &sdata.Data[0].y, sdata.Data.size(), 0, sdata.Offset, 2 * sizeof(float));
            ImPlot::EndPlot();
        }
        ImGui::End();

        // scene editor
        ImVec2 sceneWindowPos = {static_cast<float>(_resources.windowExtent.width), 0.0f};
        ImVec2 sceneWindowPivot = {1.0f, 0.0f};
        ImVec2 sceneWindowSize = {-1, ImGui::GetIO().DisplaySize.y};
        ImGui::SetNextWindowPos(sceneWindowPos, 0, sceneWindowPivot);
        ImGui::SetNextWindowSize(sceneWindowSize);
        ImGui::Begin("Scene", nullptr);
        for (auto &it: _modelManager.models) {
            if(ImGui::TreeNode(it.first.c_str())) {
                ImGui::DragFloat3("Translation", it.second.translation, 1.0f, 0.0f, 0.0f, "%.1f");
                ImGui::DragFloat3("Rotation", it.second.rotation, 1.0f, -360.0f, 360.0f, "%.1f deg");
                ImGui::DragFloat3("Scale", it.second.scale, 1.0f, 0.0f, 0.0f, "%.1f");
                ImGui::TreePop();
            }
        }
        ImGui::End();
    }

    void Renderer::draw_objects(VkCommandBuffer cmd) {
        // set up camera parameters and copy
        GPUCameraData camData;
        camData.proj = _resources.flyCamera->_projection;
        camData.view = _resources.flyCamera->get_view_matrix();
        camData.viewproj = _resources.flyCamera->_projection * _resources.flyCamera->get_view_matrix();
        camData.camPos = _resources.flyCamera->_position;
        void *data;
        vmaMapMemory(_resources.allocator, get_current_frame().cameraBuffer._allocation, &data);
        memcpy(data, &camData, sizeof(GPUCameraData));
        vmaUnmapMemory(_resources.allocator, get_current_frame().cameraBuffer._allocation);

        // get buffer info and build set
        VkDescriptorBufferInfo camBufferInfo = VkRenderer::info::descriptor_buffer_info(get_current_frame().cameraBuffer._buffer, 0, sizeof(GPUCameraData));
        VkDescriptorSet globalSet;
        VkRenderer::descriptor::Builder::begin(_resources.descriptorLayoutCache, get_current_frame()._descriptorAllocator)
                .bind_buffer(0, &camBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
                .build(globalSet);

        Material *defaultMaterial = _materialManager.get_material("pbr");
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, defaultMaterial->pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, defaultMaterial->pipelineLayout, 0, 1, &globalSet, 0, nullptr);

        for (auto &it: _modelManager.models) {
            it.second.update_transform();
            it.second.draw_model(cmd);
        }
    }

    void Renderer::draw() {
        // start the GUI frame
        ImGui::Render();

        // wait until previous frame is rendered, timeout 1sec
        VK_CHECK(vkWaitForFences(_resources.device, 1, &get_current_frame()._renderFence, true, 1000000000));
        VK_CHECK(vkResetFences(_resources.device, 1, &get_current_frame()._renderFence));

        // check for camera movement - use previous frametime as a delta
        _resources.flyCamera->process_keyboard(_previousFrameTime);

        // grab image from swapchain, timeout 1sec
        uint32_t swapchainImageIndex;
        VK_CHECK(vkAcquireNextImageKHR(_resources.device, _resources.swapchain, 1000000000, get_current_frame()._presentSemaphore, nullptr, &swapchainImageIndex));

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
        VkRenderPassBeginInfo rpInfo = VkRenderer::info::renderpass_begin_info(_resources.renderPass, 0, 0, _resources.windowExtent, _resources.framebuffers[swapchainImageIndex],
                                                                               2, &clearValues[0]);
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
        VK_CHECK(vkQueueSubmit(_resources.graphicsQueue, 1, &submit, get_current_frame()._renderFence));

        // present image and check result
        VkPresentInfoKHR presentInfo = VkRenderer::info::present_info(1, &_resources.swapchain, 1, &get_current_frame()._renderSemaphore, &swapchainImageIndex);
        VK_CHECK(vkQueuePresentKHR(_resources.graphicsQueue, &presentInfo));

        // end the frame
        _frameNumber++;
    }

    void Renderer::run() {
        SDL_Event e;
        bool bQuit = false;

        while (!bQuit) {
            // start timing the frame
            auto frameTimerStart = std::chrono::high_resolution_clock::now();

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
                if (_toggleUI) ImGui_ImplSDL2_ProcessEvent(&e);
                // close on Alt+F4 or exit button
                if (e.type == SDL_QUIT) {
                    bQuit = true;
                }
                // enable or disable camera to use UI
                if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym == SDLK_TAB) {
                        _toggleUI = !_toggleUI;
                    }
                }
                // capture mouse movement and process
                if (e.type == SDL_MOUSEMOTION && !_toggleUI) {
                    _resources.flyCamera->process_mouse(e.motion.xrel, e.motion.yrel);
                }
            }

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL2_NewFrame(_resources.window);
            ImGui::NewFrame();
            update_ui();

            draw();

            // finish timing the frame, convert to double and store
            auto frameTimerEnd = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> frameDuration = frameTimerEnd - frameTimerStart;
            _previousFrameTime = frameDuration.count();
        }
    }

    void Renderer::cleanup() {
        if (_isInitialized) {
            // block until GPU finishes
            vkDeviceWaitIdle(_resources.device);

            // flush the deletion queues
            _resources.mainDeletionQueue.flush();
            //_modelManager.cleanup();

            // clean up caches
            for (auto &_frame: _frames) {
                _frame._descriptorAllocator->cleanup();
            }
            _resources.descriptorLayoutCache->cleanup();

            // manually destroy remaining objects
            ImPlot::DestroyContext();
            ImGui::DestroyContext();
            vmaDestroyAllocator(_resources.allocator);
            vkDestroySurfaceKHR(_resources.instance, _resources.surface, nullptr);
            vkDestroyDevice(_resources.device, nullptr);
            vkb::destroy_debug_utils_messenger(_resources.instance, _resources.debug_messenger);
            vkDestroyInstance(_resources.instance, nullptr);

            SDL_DestroyWindow(_resources.window);
        }
    }
}