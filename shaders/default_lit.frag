#version 460

// color input
layout (location = 0) in vec3 inColor;

// output write
layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 1) uniform SceneData {
    vec4 fogColor; // w for exponent
    vec4 fogDistances; // x for min, y for max, zw unused
    vec4 ambientColor;
    vec4 sunlightDirection; // w for sun power
    vec4 sunlightColor;
} sceneData;

void main() {
    // output same color as input
    outFragColor = vec4(inColor + sceneData.ambientColor.xyz, 1.0f);
}