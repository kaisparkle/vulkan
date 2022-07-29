#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <glm/glm.hpp>
#include <vk/mesh.h>
#include <vk/material.h>
#include <vk/common.h>

namespace VkRenderer {
    struct RenderObject {
        Mesh *mesh;
        Material *material;
        glm::mat4 transformMatrix;
    };

    class RenderObjectManager {
    public:
        RenderObject *create_render_object(Mesh *mesh, Material *material, glm::mat4 transformMatrix);

        RenderObject *get_render_object(uint32_t id);

        size_t get_count();

    private:
        std::vector<RenderObject> _renderObjects;
    };
}