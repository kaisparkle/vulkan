#pragma once

#include <vector>
#include <vk/vertex.h>
#include <vk/types.h>
#include <vk/material.h>
#include <vk/texture.h>

namespace VkRenderer {
    struct Mesh {
        std::vector<Vertex> _vertices;
        std::vector<uint16_t> _indices;
        AllocatedBuffer _vertexBuffer;
        AllocatedBuffer _indexBuffer;
        Texture *_texture;
        Material *_material;

        void upload_mesh(ResourceHandles *resources);

        void draw_mesh(VkCommandBuffer cmd, glm::mat4 modelMatrix);
    };
}