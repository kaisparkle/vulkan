#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;
layout (location = 3) in vec2 vTexCoord;
layout (location = 4) in vec3 vTangent;
layout (location = 5) in vec3 vBitangent;

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 texCoord;
layout (location = 2) out vec3 worldPos;
layout (location = 3) out vec3 outNormal;

layout (set = 0, binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
    vec3 camPos;
} cameraData;

layout (push_constant) uniform constants {
    vec4 data;
    mat4 matrix;
} pushConstant;

void main() {
    mat4 transformMatrix = (cameraData.viewproj * pushConstant.matrix);
    gl_Position = transformMatrix * vec4(vPosition, 1.0f);
    outColor = vColor;
    texCoord = vTexCoord;

    worldPos = vec3(pushConstant.matrix * vec4(vPosition, 1.0f));
    outNormal = mat3(pushConstant.matrix) * vNormal;
}