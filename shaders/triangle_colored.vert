#version 450

layout (location = 0) out vec3 outColor;

void main() {
    // array of positions for triangle
    const vec3 positions[3] = vec3[3](
        vec3(1.0f, 1.0f, 0.0f),
        vec3(-1.0f, 1.0f, 0.0f),
        vec3(0.0f, -1.0f, 0.0f)
    );

    // array of colors for triangle
    const vec3 colors[3] = vec3[3](
        vec3(1.0f, 0.0f, 0.0f), // red
        vec3(0.0f, 1.0f, 0.0f), // green
        vec3(0.0f, 0.0f, 2.0f) // blue
    );

    // output position of each vertex
    gl_Position = vec4(positions[gl_VertexIndex], 1.0f);
    // output color based on vertex index
    outColor = colors[gl_VertexIndex];
}