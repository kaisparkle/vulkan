#include "object.h"

namespace VkRenderer {
    RenderObject *RenderObjectManager::create_render_object(Mesh *mesh, Material *material, glm::mat4 transformMatrix) {
        RenderObject newObject;
        newObject.mesh = mesh;
        newObject.material = material;
        newObject.transformMatrix = transformMatrix;
        _renderObjects.push_back(newObject);
        return &_renderObjects.back();
    }

    RenderObject *RenderObjectManager::get_render_object(uint32_t id) {
        return &_renderObjects[id];
    }

    size_t RenderObjectManager::get_count() {
        return _renderObjects.size();
    }
}