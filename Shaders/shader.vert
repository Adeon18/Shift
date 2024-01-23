#version 450

layout (binding = 0) uniform PerFrame {
    mat4 model;
    mat4 view;
    mat4 proj;
} perFrame;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = perFrame.proj * perFrame.view * perFrame.model * vec4(inPosition, 0.5, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}