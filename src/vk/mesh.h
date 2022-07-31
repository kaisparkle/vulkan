#pragma once

#include <vector>
#include <vk/vertex.h>
#include <vk/types.h>

namespace VkRenderer {
    struct Mesh {
        std::vector<Vertex> _vertices;
        std::vector<uint16_t> _indices;
        AllocatedBuffer _vertexBuffer;
        AllocatedBuffer _indexBuffer;
    };
}