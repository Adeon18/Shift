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

layout (set = 2, binding = 0) uniform PerObj {
    mat4 meshToModel;
    mat4 meshToModelInv;
    mat4 modelToWorld;
    mat4 modelToWorldInv;
} perObj;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0f, 0.0, 0.0, 1.0f);
}