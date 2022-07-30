#include <iostream>
#include <fstream>
#include <vk/check.h>
#include <vk/info.h>
#include <vk/pipeline.h>
#include <vk/vertex.h>

#include "material.h"

namespace VkRenderer {
    Material *MaterialManager::create_material(MaterialCreateInfo *info) {
        // attempt to load shaders
        VkShaderModule vertShader;
        if (!load_shader_module(info->device, info->vertShaderPath, &vertShader)) {
            std::cout << "Error building vertex shader module" << std::endl;
        } else {
            std::cout << "Vertex shader successfully loaded" << std::endl;
        }
        VkShaderModule fragShader;
        if (!load_shader_module(info->device, info->fragShaderPath, &fragShader)) {
            std::cout << "Error building fragment shader module" << std::endl;
        } else {
            std::cout << "Fragment shader successfully loaded" << std::endl;
        }

        // build the pipeline layout
        VkPipelineLayout pipelineLayout;
        VkPipelineLayoutCreateInfo mesh_pipeline_layout_info = VkRenderer::info::pipeline_layout_create_info();

        // set descriptor set layouts
        mesh_pipeline_layout_info.setLayoutCount = info->setLayoutCount;
        mesh_pipeline_layout_info.pSetLayouts = info->setLayouts;

        VK_CHECK(vkCreatePipelineLayout(info->device, &mesh_pipeline_layout_info, nullptr, &pipelineLayout));

        // build the pipeline
        VkPipeline pipeline;
        VkRenderer::pipeline::PipelineBuilder pipelineBuilder;

        // add shaders to pipeline
        pipelineBuilder._shaderStages.push_back(VkRenderer::info::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertShader));
        pipelineBuilder._shaderStages.push_back(VkRenderer::info::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragShader));

        // not using yet, use default
        pipelineBuilder._vertexInputInfo = VkRenderer::info::vertex_input_state_create_info();

        // using triangle list topology
        pipelineBuilder._inputAssembly = VkRenderer::info::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        // build viewport and scissor using window extents
        pipelineBuilder._viewport.x = 0.0f;
        pipelineBuilder._viewport.y = 0.0f;
        pipelineBuilder._viewport.width = (float) info->extent.width;
        pipelineBuilder._viewport.height = (float) info->extent.height;
        pipelineBuilder._viewport.minDepth = 0.0f;
        pipelineBuilder._viewport.maxDepth = 1.0f;
        pipelineBuilder._scissor.offset = {0, 0};
        pipelineBuilder._scissor.extent = info->extent;

        // drawing filled triangles
        pipelineBuilder._rasterizer = VkRenderer::info::rasterization_state_create_info(VK_POLYGON_MODE_FILL);

        // no multisampling, use default
        pipelineBuilder._multisampling = VkRenderer::info::multisampling_state_create_info();

        // one blend attachment, no blending, RGBA
        pipelineBuilder._colorBlendAttachment = VkRenderer::pipeline::color_blend_attachment_state();

        // use mesh layout we just created
        pipelineBuilder._pipelineLayout = pipelineLayout;

        // add depth stencil
        pipelineBuilder._depthStencil = VkRenderer::info::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

        // add vertex description
        VertexInputDescription vertexDescription = VkRenderer::Vertex::get_vertex_description();
        pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
        pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();
        pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
        pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();

        // build pipeline
        pipelineBuilder._pipelineLayout = pipelineLayout;
        pipeline = pipelineBuilder.build_pipeline(info->device, info->renderPass);

        // shader modules are okay to delete once pipeline is built
        vkDestroyShaderModule(info->device, vertShader, nullptr);
        vkDestroyShaderModule(info->device, fragShader, nullptr);

        // store the material
        Material newMaterial{};
        newMaterial.pipelineLayout = pipelineLayout;
        newMaterial.pipeline = pipeline;
        _materials[info->name] = newMaterial;

        std::cout << "Created material " << info->name << std::endl;
        return &_materials[info->name];
    }

    Material *MaterialManager::get_material(const std::string &name) {
        // search for material, return nullptr if not found
        auto it = _materials.find(name);
        if (it == _materials.end()) {
            return nullptr;
        } else {
            return &(*it).second;
        }
    }

    bool MaterialManager::load_shader_module(VkDevice device, const char *filePath, VkShaderModule *outShaderModule) {
        // open file, cursor at end
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            return false;
        }

        // get file size by checking location of cursor
        size_t fileSize = (size_t) file.tellg();
        // SPIR-V expects uint32_t buffer
        std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
        // return cursor to beginning
        file.seekg(0);
        // read entire file into buffer
        file.read((char *) buffer.data(), fileSize);
        // done with file, clean up
        file.close();

        // create shader module and check - not using VK_CHECK as shader errors are common
        VkShaderModuleCreateInfo createInfo = VkRenderer::info::shader_module_create_info(buffer.size() * sizeof(uint32_t), buffer.data());
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            return false;
        }
        *outShaderModule = shaderModule;
        return true;
    }

    void MaterialManager::cleanup(VkDevice device) {
        for (const auto &material: _materials) {
            vkDestroyPipeline(device, material.second.pipeline, nullptr);
            vkDestroyPipelineLayout(device, material.second.pipelineLayout, nullptr);
        }
    }
}