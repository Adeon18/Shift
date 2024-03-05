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

layout (set = 1, binding = 0) uniform PerView {
    mat4 view;
    mat4 proj;
    mat4 viewInv;
    mat4 projInv;
} perView;

layout (set = 2, binding = 0) uniform PerObj {
    mat4 meshToModel;
    mat4 meshToModelInv;
    mat4 modelToWorld;
    mat4 modelToWorldInv;
    vec4 color;
} perObj;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNorm;
layout(location = 4) in vec3 inTan;
layout(location = 5) in vec3 inBitan;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outWorldNorm;

void main() {
    vec3 modelNorm = normalize(transpose(perObj.meshToModelInv) * vec4(inNorm, 0.0f)).xyz;
    vec3 modelTan = normalize(transpose(perObj.meshToModelInv) * vec4(inTan, 0.0f)).xyz;
    vec3 modelBitan = normalize(transpose(perObj.meshToModelInv) * vec4(inBitan, 0.0f)).xyz;
    vec4 modelPos = normalize(perObj.meshToModel * vec4(inPosition, 1.0f));

    vec4 worldPos = perObj.modelToWorld * modelPos;
    outWorldPos = worldPos.xyz;
    // The world inv is transposed at entering the shader
    outWorldNorm = normalize(transpose(perObj.modelToWorldInv) * vec4(modelNorm, 0.0f)).xyz;
    vec3 worldTan = normalize(transpose(perObj.modelToWorldInv) * vec4(modelTan, 0.0f)).xyz;
    vec3 worldBiTan = normalize(transpose(perObj.modelToWorldInv) * vec4(modelBitan, 0.0f)).xyz;

    gl_Position = perView.proj * perView.view * worldPos;
}