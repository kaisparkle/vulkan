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

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43578.5453123);
}

void main() {
    vec2 st = gl_FragCoord.xy + sceneData.ambientColor.xy;
    float rnd = random(st);
    outFragColor = vec4(vec3(rnd), 1.0f);
}