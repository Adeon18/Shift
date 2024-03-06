#ifndef BASE_GLSL
#define BASE_GLSL

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

#endif