#include <iostream>
#include <vk/info.h>
#include <vk/check.h>
#include <vk/utils.h>

#include "mesh.h"

namespace VkRenderer {
    void Mesh::upload_mesh(VmaAllocator &allocator, VkDevice &device, VkQueue &queue, UploadContext &uploadContext, DeletionQueue &deletionQueue) {
        // create CPU-side staging buffers
        VmaAllocationCreateInfo stagingAllocInfo = VkRenderer::info::allocation_create_info(VMA_MEMORY_USAGE_CPU_ONLY, 0);

        // vertices
        const size_t vertexBufferSize = _vertices.size() * sizeof(Vertex);
        VkBufferCreateInfo vertexStagingInfo = VkRenderer::info::buffer_create_info(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        AllocatedBuffer vertexStaging;
        VK_CHECK(vmaCreateBuffer(allocator, &vertexStagingInfo, &stagingAllocInfo, &vertexStaging._buffer, &vertexStaging._allocation, nullptr));

        // copy data
        void *vertexData;
        vmaMapMemory(allocator, vertexStaging._allocation, &vertexData);
        memcpy(vertexData, _vertices.data(), _vertices.size() * sizeof(Vertex));
        vmaUnmapMemory(allocator, vertexStaging._allocation);

        // indices
        const size_t indexBufferSize = _indices.size() * sizeof(uint16_t);
        VkBufferCreateInfo indexStagingInfo = VkRenderer::info::buffer_create_info(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        AllocatedBuffer indexStaging;
        VK_CHECK(vmaCreateBuffer(allocator, &indexStagingInfo, &stagingAllocInfo, &indexStaging._buffer, &indexStaging._allocation, nullptr));

        // copy data
        void *indexData;
        vmaMapMemory(allocator, indexStaging._allocation, &indexData);
        memcpy(indexData, _indices.data(), _indices.size() * sizeof(uint16_t));
        vmaUnmapMemory(allocator, indexStaging._allocation);

        // reuse alloc info for GPU-side buffers
        stagingAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        // vertices
        VkBufferCreateInfo vertexBufferInfo = VkRenderer::info::buffer_create_info(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        VK_CHECK(vmaCreateBuffer(allocator, &vertexBufferInfo, &stagingAllocInfo, &_vertexBuffer._buffer, &_vertexBuffer._allocation, nullptr));
        deletionQueue.push_function([=]() {
            vmaDestroyBuffer(allocator, _vertexBuffer._buffer, _vertexBuffer._allocation);
        });

        // copy data to GPU
        VkRenderer::utils::immediate_submit(device, queue, uploadContext, [=](VkCommandBuffer cmd) {
            VkBufferCopy copy;
            copy.dstOffset = 0;
            copy.srcOffset = 0;
            copy.size = vertexBufferSize;
            vkCmdCopyBuffer(cmd, vertexStaging._buffer, _vertexBuffer._buffer, 1, &copy);
        });

        // indices
        VkBufferCreateInfo indexBufferInfo = VkRenderer::info::buffer_create_info(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        VK_CHECK(vmaCreateBuffer(allocator, &indexBufferInfo, &stagingAllocInfo, &_indexBuffer._buffer, &_indexBuffer._allocation, nullptr));
        deletionQueue.push_function([=]() {
            vmaDestroyBuffer(allocator, _vertexBuffer._buffer, _vertexBuffer._allocation);
        });

        // copy data to GPU
        VkRenderer::utils::immediate_submit(device, queue, uploadContext, [=](VkCommandBuffer cmd) {
            VkBufferCopy copy;
            copy.dstOffset = 0;
            copy.srcOffset = 0;
            copy.size = indexBufferSize;
            vkCmdCopyBuffer(cmd, indexStaging._buffer, _indexBuffer._buffer, 1, &copy);
        });

        // we're done with the staging buffers
        vmaDestroyBuffer(allocator, vertexStaging._buffer, vertexStaging._allocation);
        vmaDestroyBuffer(allocator, indexStaging._buffer, indexStaging._allocation);

        // allocate the vertex buffer and check
        /*VkBufferCreateInfo bufferInfo = VkRenderer::info::buffer_create_info(_vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
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
        vmaUnmapMemory(allocator, _indexBuffer._allocation);*/
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