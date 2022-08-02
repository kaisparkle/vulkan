#include <iostream>
#include <vk/info.h>
#include <vk/check.h>

#include "mesh.h"

namespace VkRenderer {
    void Mesh::upload_mesh(VmaAllocator &allocator, DeletionQueue &deletionQueue) {
        // allocate the vertex buffer and check
        VkBufferCreateInfo bufferInfo = VkRenderer::info::buffer_create_info(_vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        VmaAllocationCreateInfo vmaAllocInfo = VkRenderer::info::allocation_create_info(VMA_MEMORY_USAGE_CPU_TO_GPU, 0);
        VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo,
                                 &_vertexBuffer._buffer,
                                 &_vertexBuffer._allocation,
                                 nullptr));

        deletionQueue.push_function([=]() {
            vmaDestroyBuffer(allocator, _vertexBuffer._buffer, _vertexBuffer._allocation);
        });

        // copy vertex data into new buffer - map to pointer, write, unmap
        void *data;
        vmaMapMemory(allocator, _vertexBuffer._allocation, &data);
        memcpy(data, _vertices.data(), _vertices.size() * sizeof(Vertex));
        vmaUnmapMemory(allocator, _vertexBuffer._allocation);

        // allocate index buffer
        VkBufferCreateInfo indexBufferInfo = VkRenderer::info::buffer_create_info(_indices.size() * sizeof(uint16_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        VmaAllocationCreateInfo vmaIndexAllocInfo = VkRenderer::info::allocation_create_info(VMA_MEMORY_USAGE_CPU_TO_GPU, 0);
        VK_CHECK(vmaCreateBuffer(allocator, &indexBufferInfo, &vmaIndexAllocInfo,
                                 &_indexBuffer._buffer,
                                 &_indexBuffer._allocation,
                                 nullptr));
        deletionQueue.push_function([=]() {
            vmaDestroyBuffer(allocator, _indexBuffer._buffer, _indexBuffer._allocation);
        });

        // copy
        void *indexData;
        vmaMapMemory(allocator, _indexBuffer._allocation, &indexData);
        memcpy(indexData, _indices.data(), _indices.size() * sizeof(uint16_t));
        vmaUnmapMemory(allocator, _indexBuffer._allocation);
    }

    void Mesh::draw_mesh(VkCommandBuffer cmd, glm::mat4 modelMatrix) {
        // push model matrix through push constant
        MatrixPushConstant constant{};
        constant.matrix = modelMatrix;
        vkCmdPushConstants(cmd, _material->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MatrixPushConstant), &constant);

        // bind buffers and draw
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &_vertexBuffer._buffer, &offset);
        vkCmdBindIndexBuffer(cmd, _indexBuffer._buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(cmd, static_cast<uint32_t>(_indices.size()), 1, 0, 0, 0);
    }
}