#version 450

#extension GL_GOOGLE_include_directive : require

#include "../Base.glsl"
#include "../VertexInputs.glsl"
#include "../Utility.glsl"

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outWorldNorm;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out mat3 TBN;

void main() {
    vec3 meshNorm = inNorm;
    vec3 meshTan = inTan;
    vec3 meshPos = inPosition;
    vec3 meshBitan = inBitan;

    // Transform coords to world
//    VertexInputsMeshToModelTransform(meshNorm, meshTan, meshBitan, meshPos);
    VertexInputsModelToWorldTransform(meshNorm, meshTan, meshBitan, meshPos);

    outWorldPos = meshPos;
    outWorldNorm = meshNorm;
    fragTexCoord = inTexCoord;

    vec3 T = normalize(meshTan);
    vec3 N = normalize(meshNorm);
    vec3 B = normalize(cross(T, N));
    TBN = mat3(T, B, N);

    gl_Position = perView.proj * perView.view * vec4(meshPos, 1.0f);
}