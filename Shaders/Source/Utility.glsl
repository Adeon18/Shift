#ifndef UTILITY_GLSL
#define UTILITY_GLSL

#include "Base.glsl"

void VertexInputsMeshToModelTransform(in out vec3 modelNorm, in out vec3 modelTan, in out vec3 modelBitan, in out vec3 modelPos) {
    modelNorm = normalize(transpose(perObj.meshToModelInv) * vec4(modelNorm, 0.0f)).xyz;
    modelTan = normalize(transpose(perObj.meshToModelInv) * vec4(modelTan, 0.0f)).xyz;
    modelBitan = normalize(transpose(perObj.meshToModelInv) * vec4(modelBitan, 0.0f)).xyz;
    modelPos = normalize(perObj.meshToModel * vec4(modelPos, 1.0f)).xyz;
}

void VertexInputsModelToWorldTransform(in out vec3 worldNorm, in out vec3 worldTan, in out vec3 worldBitan, in out vec3 worldPos) {
    worldPos = (perObj.modelToWorld * vec4(worldPos, 1.0f)).xyz;
    // The world inv is transposed at entering the shader
    worldNorm = normalize(transpose(perObj.modelToWorldInv) * vec4(worldNorm, 0.0f)).xyz;
    worldTan = normalize(transpose(perObj.modelToWorldInv) * vec4(worldTan, 0.0f)).xyz;
    worldBitan = normalize(transpose(perObj.modelToWorldInv) * vec4(worldBitan, 0.0f)).xyz;
}

#endif // UTILITY_GLSL