#version 450

#extension GL_GOOGLE_include_directive : require

#include "../Base.glsl"

layout(location = 0) in vec3 outWorldPos;
layout(location = 1) in vec3 outWorldNorm;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 1) uniform sampler2D texSampler;

void main() {
    vec4 color = texture(texSampler, fragTexCoord);

    outColor = vec4(color.rgb, 1.0f);
}