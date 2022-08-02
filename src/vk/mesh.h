#pragma once

#include <vector>
#include <vk/vertex.h>
#include <vk/types.h>
#include <vk/material.h>

namespace VkRenderer {
    struct Mesh {
        std::vector<Vertex> _vertices;
        std::vector<uint16_t> _indices;
        AllocatedBuffer _vertexBuffer;
        AllocatedBuffer _indexBuffer;
        Material *_material;

        void upload_mesh(VmaAllocator &allocator, DeletionQueue &deletionQueue);

        void draw_mesh(VkCommandBuffer cmd, glm::mat4 modelMatrix);
    };
}