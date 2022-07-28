#pragma once

#include <vector>
#include <vk/common.h>

namespace vkinit::pipeline {
    VkPipelineColorBlendAttachmentState color_blend_attachment_state();

    class PipelineBuilder {
    public:
        std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
        VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
        VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
        VkPipelineRasterizationStateCreateInfo _rasterizer;
        VkPipelineColorBlendAttachmentState _colorBlendAttachment;
        VkPipelineMultisampleStateCreateInfo _multisampling;
        VkPipelineDepthStencilStateCreateInfo _depthStencil;
        VkPipelineLayout _pipelineLayout;
        VkViewport _viewport;
        VkRect2D _scissor;

        VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);
    };
}