#version 460

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 texCoord;

layout (location = 0) out vec4 outFragColor;

layout (set = 1, binding = 0) uniform sampler2D tex1;

void main() {
    // output same color as input
    vec4 texColor = texture(tex1, texCoord);

    outFragColor = texColor;
}