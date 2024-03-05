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
    vec4 color;
} perObj;

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inWorldNorm;

layout(location = 0) out vec4 outColor;

void main() {
    float camAngle = max(dot(-perFrame.camDirection.xyz, inWorldNorm), 0.05f);
    outColor = vec4(perObj.color.rgb * camAngle, 1.0f);
    //outColor = vec4(inWorldNorm * 0.5 + 0.5, 1.0f);
}