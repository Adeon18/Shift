#version 450

layout (set = 0, binding = 0) uniform PerFrame {
    vec4 camPosition;
    vec4 camDirection;
    vec4 camRight;
    vec4 camUp;
    /// Window data, xy - width/height; zw - 1/width\height
    vec4 windowData;
    /// Timer data, x - dt, y - fps, z - seconds dince start
    vec4 timerData;
} perFrame;

layout (set = 1, binding = 0) uniform PerFrameLegacy {
    mat4 view;
    mat4 proj;
    mat4 viewInv;
    mat4 projInv;
} perView;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = perView.proj * perView.view * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}