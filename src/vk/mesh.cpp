#include <iostream>
#include <tiny_obj_loader.h>
#include <vk/common.h>

#include "mesh.h"

VertexInputDescription Vertex::get_vertex_description() {
    VertexInputDescription description;

    // one vertex buffer binding, per-vertex rate
    VkVertexInputBindingDescription mainBinding = {};
    mainBinding.binding = 0;
    mainBinding.stride = sizeof(Vertex);
    mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    description.bindings.push_back(mainBinding);

    // position at location 0
    VkVertexInputAttributeDescription positionAttribute = {};
    positionAttribute.binding = 0;
    positionAttribute.location = 0;
    positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    positionAttribute.offset = offsetof(Vertex, position);

    // normal at location 1
    VkVertexInputAttributeDescription normalAttribute = {};
    normalAttribute.binding = 0;
    normalAttribute.location = 1;
    normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    normalAttribute.offset = offsetof(Vertex, normal);

    // color at location 2
    VkVertexInputAttributeDescription colorAttribute = {};
    colorAttribute.binding = 0;
    colorAttribute.location = 2;
    colorAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    colorAttribute.offset = offsetof(Vertex, color);

    description.attributes.push_back(positionAttribute);
    description.attributes.push_back(normalAttribute);
    description.attributes.push_back(colorAttribute);

    return description;
}

bool Mesh::load_from_obj(const char *filename) {
    // attrib will contain vertex arrays
    tinyobj::attrib_t attrib;
    // shapes will contain info for each separate object
    std::vector<tinyobj::shape_t> shapes;
    // materials will contain info for the material of each shape
    std::vector<tinyobj::material_t> materials;

    // load the obj
    std::string warn;
    std::string err;
    tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, nullptr);
    // check warnings and errors
    if (!warn.empty()) {
        std::cout << "WARN: " << warn << std::endl;
    }
    if (!err.empty()) {
        std::cerr << "ERR: " << err << std::endl;
        return false;
    }

    // loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        // loop over faces
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            // hardcode loading to triangles
            int fv = 3;
            // loop over face vertices
            for (size_t v = 0; v < fv; v++) {
                // access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                // vertex position
                tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
                // vertex normal
                tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
                tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
                tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
                // copy into our vertex
                Vertex new_vert;
                new_vert.position.x = vx;
                new_vert.position.y = vy;
                new_vert.position.z = vz;
                new_vert.normal.x = nx;
                new_vert.normal.y = ny;
                new_vert.normal.z = nz;
                // set the color to normal for display purposes
                new_vert.color = new_vert.normal;
                // push to mesh
                _vertices.push_back(new_vert);
            }
            index_offset += fv;
        }
    }

    return true;
}