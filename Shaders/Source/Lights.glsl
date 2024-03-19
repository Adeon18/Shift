#ifndef LIGHTS_GLSL
#define LIGHTS_GLSL

struct DirectionalLight {
    vec4 direction;
    vec4 radiance;
};

struct PointLight {
    vec4 position;
    vec4 radiance;
};

layout (set = 0, binding = 1) uniform LightsPerFrame {
    DirectionalLight directionalLights[2];
    PointLight pointLights[6];
    ivec2 lightCounts; // x - dir, y - point
} lights;


// Prototype has no attenuation
vec3 CalcPointLightDiffuse(PointLight light, vec3 normal, vec3 fragPos)
{
    vec3 lightDir = normalize(light.position.xyz - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    return diff * light.radiance.rgb;
}

// Prototype has no attenuation
vec3 CalcDirectionalLightDiffuse(DirectionalLight light, vec3 normal)
{
    float diff = max(dot(normal, light.direction.xyz), 0.0);
    return diff * light.radiance.rgb;
}

#endif // LIGHTS_GLSL