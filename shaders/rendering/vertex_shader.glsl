#version 430 core

layout(location = 0) in vec2 position;  // Quad vertices

out vec2 uv;  // Pass UV coordinates to the fragment shader

void main() {
    // Pass through position and map to normalized device coordinates
    gl_Position = vec4(position, 0.0, 1.0);
    uv = position * 0.5 + 0.5;  // Convert [-1, 1] to [0, 1] range for UV
}
