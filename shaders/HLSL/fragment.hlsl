/// ------------------ FRAGMENT SHADER ------------------
/// [Guides for HLSL to SPIR-V mapping]
/// 
/// 1. Descriptor binding guide
///     [[vk::binding(X, Y)]] , where X is the binding point and Y is the descriptor set index
///     
/// 2. Texture array binding
///     A descriptor type 'VK_DESCRIPTOR_SAMPLED_IMAGE_TYPE' from vulkan is represented as a 'TextureXD' or 'TextureXDArray' in HLSL, where 'XD' means X dimensions.
/// 
/// 3. Sampler binding
///     A descriptor type 'VK_DESCRIPTOR_SAMPLER_TYPE' from vulkan is represented as a 'SamplerState' in HLSL, which is used to sample textures.
/// 
/// 4. How to sample a color from texture array
///     To sample a color from texture array, follow below code statements:
///     <texture_array_variable>.Sample(<sample_state_variable>, <input_texture_coordinates>)
///     - <texture_array_variable> : A variable with TextureXDArray type
///     - <sample_state_variable>   : A variable with SamplerState type
///     - <input_texture_coordinates>: A float3(u,v,w), where 'w' component means the index of the texture in the array
/// 
/// 5. Push constant binding
///     A push constant binding requires a '[[vk::push_constant]]' attribute in HLSL and The push constant struct should be defined in the shader file.
///     Also Note that you can only bind a single push constant per shader program.

#include "common.hlsl"

// ------------------ DEFINITIONS ------------------
struct PSInput
{
    float3 WorldPos    : POSITION0;
    float3 WorldNormal : NORMAL0;
    float3 ViewDir     : VIEW0;
    float2 TexCoord    : TEXCOORD0;
};

[[vk::push_constant]]
PushConstantRaster pc;


// ------------------- Constant -------------------------
static const float PI = 3.14159265f;

// -------------------- PBR ----------------------------- 

static float ginnekenChiClamp(float angle)
{
    return saturate(4.41 * angle / (4.41 * angle + 1)); // clamp Chi function result between 0 and 1
}

// a_variable for parameter in lambda functions
static float avar(float3 n, float3 s, float roughness)
{
    float dotNS = dot(n, s);

    return dotNS / (roughness * sqrt(1 - pow(dotNS, 2)));
}

static float avarSqaure(float3 n, float s, float roughness)
{
    float dotNS = dot(n, s);

    return dotNS * dotNS / (roughness * roughness * (1 - pow(dotNS, 2)));
}

/*
    Lambda functions
*/
static float lambdaApproxBeckmann(float a)
{
    if (a >= 1.6)
    {
        return 0;
    }
    else
    {
        float poweredA = pow(a, 2);
        return (1 - 1.259 * a + 0.396 * poweredA) / (3.535 * a + 2.181 * poweredA);
    }
}

static float lambdaGGX(float aSquare)
{
    return (-1 + sqrt(1 + 1 / aSquare)) / 2;
}

/*
    Smith Shadowing Masking Functions
*/

static float heightCorrelatedSmithG2(float3 l, float3 v, float3 m, float roughness)
{
    float chiMV = ginnekenChiClamp(dot(m, v));
    float chiML = ginnekenChiClamp(dot(m, l));
    float aFromV = avar(m, v, roughness);
    float aFromL = avar(m, l, roughness);
    float lambdaV = lambdaApproxBeckmann(aFromV);
    float lambdaL = lambdaApproxBeckmann(aFromL);

    return chiMV * chiML / (1 + lambdaV + lambdaL);
}

static float heightCorrelatedSmithG2GGX(float3 n, float3 l, float3 v, float alpha)
{
    float alphaSquare = alpha * alpha;

    float muIn = saturate(dot(n, l));
    float muOut = saturate(dot(n, v));

    float denominator = muOut * sqrt(alphaSquare + muIn * (muIn - alphaSquare * muIn)) + muIn * sqrt(alphaSquare + muOut * (muOut - alphaSquare * muOut));

    return 0.5 / denominator;
}

/*
    Normal Distribution Functions
*/

static float beckmannNDF(float dotNM, float roughness)
{
    float dotNMSquare = dotNM * dotNM;
    float roughnessSquare = roughness * roughness;
    float chiDotNM = ginnekenChiClamp(dotNM);

    // For all microfacet normal that point to under macrosurface, clamp to zero.
    return (chiDotNM / (PI * roughnessSquare * dotNMSquare * dotNMSquare)) * exp((dotNMSquare - 1) / (roughnessSquare * dotNMSquare));
}

/*
 GGX-NDF
    ag is a parameter that is equal to r^2, where r is user-selective roughness between 0 and 1
*/
static float GGXNDF(float dotNM, float ag)
{
    float agSquare = ag * ag;
    return ginnekenChiClamp(dotNM) * agSquare / PI * pow((1 + (dotNM * dotNM) * (agSquare - 1)), 2);
}

// use this instead of original form of Fresnel equation
static float schlickApprox(float ior, float3 n, float3 l)
{
    float fZero = (ior - 1) * (ior - 1) / (ior + 1) * (ior + 1);
    float dotNV = saturate(dot(n, l)); // clmap 0 ~ 1

    // interpolation
    return fZero + (1 - fZero) * (1 - pow(dotNV, 5));
}

static float microSpecularBRDF(float3 l, float3 v, float3 n, float roughness)
{
    // compute half vector
    float3 h = normalize(l + v);

    // NDF : GGX
    float ior = 1.4; // skin in air
    float F  = schlickApprox(ior, h, l);
    float G2 = heightCorrelatedSmithG2GGX(h, l, v, roughness);
    float D = GGXNDF(dot(n, h), roughness * roughness);

    return F * G2 * D;
}

static float diffuseBRDF(float3 l, float3 v)
{
    float3 h = normalize(l + v);

    float ior = 1.4;
    float F = schlickApprox(ior, h, l);

    return (1 - F) * 0.5 / PI;
}

static float lambertianDiffuseBRDF()
{
    float ssAlbedo = 0.5;
    return 0.5 / PI;
}

// ------------------ HELPER FUNCTIONS ------------------
static float3 computeDiffuseColor(float4 diffuseColor, float3 viewDir, float3 lightDir, float3 normal)
{
    float dotNL = dot(lightDir, normal);
    //float brdf = lambertianDiffuseBRDF();
    float3 outColor = diffuseColor.rgb * dotNL; // saturate function do clamping the value between 0 and 1
    return outColor;
}

static float3 computeSpecularColor(float4 specularColor, float3 viewDir, float3 lightDir, float3 normal)
{
    float shininess = 2.0f;

    float3 V = normalize(-viewDir);
    float3 R = reflect(-lightDir, normal);
    float dotRV = saturate(dot(V, R)); // clampe the value between 0 and 1
    //float specularTerm = pow(dotRV, shininess);
    float specularTerm = microSpecularBRDF(lightDir, viewDir, normal, 1 / shininess);

    return specularColor.rgb * specularTerm;
}


// ------------------ MAIN FUNCTION ------------------
float4 main(PSInput input) : SV_Target
{
    int diffuseIndex = 0;
    int specularIdx = 1;
    int normalIdx = 2;

    float3 lightDirection = normalize(pc.LightPosition - input.WorldPos);

    // If normal map was given, use accurate normal from it.
    float3 normalCoord = float3(input.TexCoord, (float)normalIdx);
    float3 normal = textures.Sample(samplerState, normalCoord).xyz;

    float3 diffuseCoord = float3(input.TexCoord, (float)diffuseIndex);
    float4 diffuseColor = textures.Sample(samplerState, diffuseCoord);
    float4 outColor = float4(computeDiffuseColor(diffuseColor, input.ViewDir, lightDirection, normal), 1.0f);

    // extract specular color
    float3 specularCoord = float3(input.TexCoord, (float)specularIdx);
    float4 specularColor = textures.Sample(samplerState, specularCoord);

    outColor += float4(computeSpecularColor(specularColor, input.ViewDir, lightDirection, normal), 1.0f);

    return outColor;
}