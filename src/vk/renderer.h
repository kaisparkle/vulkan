#pragma once

#include <functional>
#include <vk/material.h>
#include <vk/model.h>
#include <vk/types.h>
#include <camera.h>
#include <imgui.h>

namespace VkRenderer {
    constexpr unsigned int FRAME_OVERLAP = 2;

    struct ScrollingBuffer {
        int MaxSize;
        int Offset;
        ImVector<ImVec2> Data;

        ScrollingBuffer(int max_size = 10000) {
            MaxSize = max_size;
            Offset = 0;
            Data.reserve(MaxSize);
        }

        void AddPoint(float x, float y) {
            if (Data.size() < MaxSize)
                Data.push_back(ImVec2(x, y));
            else {
                Data[Offset] = ImVec2(x, y);
                Offset = (Offset + 1) % MaxSize;
            }
        }

        void Erase() {
            if (Data.size() > 0) {
                Data.shrink(0);
                Offset = 0;
            }
        }
    };

    class Renderer {
    public:
        void init();

        void run();

        void cleanup();

    private:
        ResourceHandles _resources;
        bool _isInitialized = false;
        bool _toggleUI = true;
        double _previousFrameTime;
        int _frameNumber = 0;
        ModelManager _modelManager;
        MaterialManager _materialManager;
        FrameData _frames[FRAME_OVERLAP];

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

        void update_ui();

        void draw_objects(VkCommandBuffer cmd);

        void draw();

        FrameData &get_current_frame();
    };
}