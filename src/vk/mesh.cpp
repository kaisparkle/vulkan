#include <iostream>
#include <vk/info.h>
#include <vk/check.h>
#include <vk/utils.h>

#include "mesh.h"

namespace VkRenderer {
    void Mesh::upload_mesh(ResourceHandles *resources) {
        // create CPU-side staging buffers
        VmaAllocationCreateInfo stagingAllocInfo = VkRenderer::info::allocation_create_info(VMA_MEMORY_USAGE_CPU_ONLY, 0);

        // vertices
        const size_t vertexBufferSize = _vertices.size() * sizeof(Vertex);
        VkBufferCreateInfo vertexStagingInfo = VkRenderer::info::buffer_create_info(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        AllocatedBuffer vertexStaging;
        VK_CHECK(vmaCreateBuffer(resources->allocator, &vertexStagingInfo, &stagingAllocInfo, &vertexStaging._buffer, &vertexStaging._allocation, nullptr));

        // copy data
        void *vertexData;
        vmaMapMemory(resources->allocator, vertexStaging._allocation, &vertexData);
        memcpy(vertexData, _vertices.data(), _vertices.size() * sizeof(Vertex));
        vmaUnmapMemory(resources->allocator, vertexStaging._allocation);

        // indices
        const size_t indexBufferSize = _indices.size() * sizeof(uint16_t);
        VkBufferCreateInfo indexStagingInfo = VkRenderer::info::buffer_create_info(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        AllocatedBuffer indexStaging;
        VK_CHECK(vmaCreateBuffer(resources->allocator, &indexStagingInfo, &stagingAllocInfo, &indexStaging._buffer, &indexStaging._allocation, nullptr));

        // copy data
        void *indexData;
        vmaMapMemory(resources->allocator, indexStaging._allocation, &indexData);
        memcpy(indexData, _indices.data(), _indices.size() * sizeof(uint16_t));
        vmaUnmapMemory(resources->allocator, indexStaging._allocation);

        // reuse alloc info for GPU-side buffers
        stagingAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        // vertices
        VkBufferCreateInfo vertexBufferInfo = VkRenderer::info::buffer_create_info(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        VK_CHECK(vmaCreateBuffer(resources->allocator, &vertexBufferInfo, &stagingAllocInfo, &_vertexBuffer._buffer, &_vertexBuffer._allocation, nullptr));
        resources->mainDeletionQueue.push_function([=]() {
            vmaDestroyBuffer(resources->allocator, _vertexBuffer._buffer, _vertexBuffer._allocation);
        });

        // copy data to GPU
        VkRenderer::utils::immediate_submit(resources, [=](VkCommandBuffer cmd) {
            VkBufferCopy copy;
            copy.dstOffset = 0;
            copy.srcOffset = 0;
            copy.size = vertexBufferSize;
            vkCmdCopyBuffer(cmd, vertexStaging._buffer, _vertexBuffer._buffer, 1, &copy);
        });

        // indices
        VkBufferCreateInfo indexBufferInfo = VkRenderer::info::buffer_create_info(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        VK_CHECK(vmaCreateBuffer(resources->allocator, &indexBufferInfo, &stagingAllocInfo, &_indexBuffer._buffer, &_indexBuffer._allocation, nullptr));
        resources->mainDeletionQueue.push_function([=]() {
            vmaDestroyBuffer(resources->allocator, _vertexBuffer._buffer, _vertexBuffer._allocation);
        });

        // copy data to GPU
        VkRenderer::utils::immediate_submit(resources, [=](VkCommandBuffer cmd) {
            VkBufferCopy copy;
            copy.dstOffset = 0;
            copy.srcOffset = 0;
            copy.size = indexBufferSize;
            vkCmdCopyBuffer(cmd, indexStaging._buffer, _indexBuffer._buffer, 1, &copy);
        });

        // we're done with the staging buffers
        vmaDestroyBuffer(resources->allocator, vertexStaging._buffer, vertexStaging._allocation);
        vmaDestroyBuffer(resources->allocator, indexStaging._buffer, indexStaging._allocation);
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
        //vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _material->pipelineLayout, 1, 1, &_texture->descriptor, 0, nullptr);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _material->pipelineLayout, 1, 1, &_pbrTexture->descriptor, 0, nullptr);
        vkCmdDrawIndexed(cmd, static_cast<uint32_t>(_indices.size()), 1, 0, 0, 0);
    }
}