//! The BRDF: each term is swappable

static const float PI = 3.14159265358979323846f;

//! From filament
static const float MIN_ROUGHNESS = 0.045f;

static const float MIN_NoV = 1e-4f;

//! Defailt F0.
static const float DIELECTRIC_F0 = 0.04f;

//! Surface params that get passe through the functions
struct SurfaceParams
{
    float3 normal;
    float3 viewDir;
    //! Base colour with the metal share of the energy already removed
    float3 diffuseColor;
    float3 f0;
    float perceptualRoughness;
    //! perceptualRoughness^2
    float alphaRoughness;
    //! Does not vary per light so it is here
    float NoV;
};

struct BRDFResult
{
    float3 diffuse;
    float3 specular;
};

//! Each term is designed to be hot-swapped in the future

//! D
interface IDistribution { float Eval(float NoH, float alphaRoughness); }

//! Visibility, G / (4 NoV NoL)
interface IVisibility { float Eval(float NoV, float NoL, float alphaRoughness); }

//! F
interface IFresnel {
    float3 Eval(float3 f0, float f90, float VoH);
    float Eval(float f0, float f90, float VoH);
}

//! Diffuse part
interface IDiffuse { float3 Eval(float3 diffuseColor, float NoV, float NoL, float LoH, float perceptualRoughness); }

interface IBRDF { BRDFResult Evaluate(SurfaceParams s, float3 lightDir); }

SurfaceParams MakeSurfaceParams(float3 normal, float3 viewDir, float3 baseColor, float metallic, float perceptualRoughness) {
    SurfaceParams s;
    s.normal = normal;
    s.viewDir = viewDir;
    s.diffuseColor = baseColor * (1.0f - metallic);
    s.f0 = lerp(float3(DIELECTRIC_F0), baseColor, metallic);
    s.perceptualRoughness = clamp(perceptualRoughness, MIN_ROUGHNESS, 1.0f);
    s.alphaRoughness = s.perceptualRoughness * s.perceptualRoughness;
    s.NoV = abs(dot(normal, viewDir)) + MIN_NoV;
    return s;
}

struct D_GGX : IDistribution
{
    //! From filament, TODO [OPTIMIZATION]
    float Eval(float NoH, float alphaRoughness)
    {
    	float a = NoH * alphaRoughness;
        float k = alphaRoughness / (1.0 - NoH * NoH + a * a);
        return k * k * (1.0 / PI);
    }
}


//! I do not name this G as it is G / (4 NoV NoL) which is visibility, from filament
// TODO: [OPTIMIZATION] for 2 sqrts
struct V_SmithGGXCorrelated : IVisibility
{
    float Eval(float NoV, float NoL, float alphaRoughness)
    {
        float a2 = alphaRoughness * alphaRoughness;
        float GGXV = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
        float GGXL = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
        return 0.5 / (GGXV + GGXL);
    }
}


//! In regular logl fresnel we would have 1.0 instead of f90
//! f90 is how much light gets reflected when we look at a surface at perfect 90 deg - it approaches 1.0
struct F_Schlick : IFresnel
{
    float3 Eval(float3 f0, float f90, float VoH)
    {
        return f0 + (float3(f90) - f0) * pow(1.0 - VoH, 5.0); // max???
    }

    float Eval(float f0, float f90, float VoH)
    {
        return f0 + (f90 - f0) * pow(1.0 - VoH, 5.0); // max???
    }
}


//! Diffuse parts

struct Fd_Lambert : IDiffuse
{
    float3 Eval(float3 diffuseColor, float NoV, float NoL, float LoH, float perceptualRoughness)
    {
        return diffuseColor / PI;
    }
}


//! TODO: parametrization here is wrong!!!
struct Fd_Burley<TF: IFresnel> : IDiffuse
{
    TF fresnel;
    float3 Eval(float3 diffuseColor, float NoV, float NoL, float LoH, float perceptualRoughness)
    {
        float f90 = 0.5 + 2.0 * perceptualRoughness * LoH * LoH;
        float lightScatter = fresnel.Eval(1.0, f90, NoL);
        float viewScatter = fresnel.Eval(1.0, f90, NoV);
        return diffuseColor * lightScatter * viewScatter * (1.0 / PI);
    }
}


struct StandardModel<TD : IDistribution, TV : IVisibility, TF : IFresnel, TFd : IDiffuse> : IBRDF
{
    TD distribution;
    TV visibility;
    TF fresnel;
    TFd diffuseTerm;

    BRDFResult Evaluate(SurfaceParams s, float3 lightDir)
    {
        BRDFResult result;
        float3 H = normalize(s.viewDir + lightDir);
        float NoH = max(0.0f, dot(s.normal, H));
        float NoL = max(0.0f, dot(s.normal, lightDir));
        float VoH = max(0.0f, dot(s.viewDir, H));

        float D = distribution.Eval(NoH, s.alphaRoughness);
        float V = visibility.Eval(s.NoV, NoL, s.alphaRoughness);
        float3 F = fresnel.Eval(s.f0, 1.0, VoH);

        result.diffuse = diffuseTerm.Eval(s.diffuseColor, s.NoV, NoL, VoH, s.perceptualRoughness);
        result.specular = D * V * F;
        return result;
    }
}

//! Current shading model which can be quickly chnaged
typealias ActiveBRDF = StandardModel<D_GGX, V_SmithGGXCorrelated, F_Schlick, Fd_Lambert>;
