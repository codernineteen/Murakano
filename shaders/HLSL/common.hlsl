struct UBO
{
    /* transform matrices */
    float4x4 viewMat;
    float4x4 projMat;
    float4x4 mvpMat; // (model x view x projection) matrix
    float4x4 viewInverseMat;
    
    /* scene environment parameter */
    float4 ambientColor;
    float4 sunlightDir;
    float4 sunlightColor; // (r, g, b, intensity)
};

struct PushConstantRaster
{
    float4x4 ModelMat;
    float3   LightPosition;
    float    LightIntensity;
    int      LightType;
};

struct GLTFMaterialData
{
    float4 colorFactors;
    float4 metallicRoughnessFactors;
};

/**
* ----- Global Descriptor ------
*/
[[vk::binding(0/* binding 0 */, 0 /* set 0 */)]] // uniform buffer binding
cbuffer ubo : register(b0, space0)
{
	UBO ubo;
}

/**
* ----- Local Descriptor ------
*/
[[vk::binding(0/* binding 0 */, 1 /* set 1 */)]] // uniform buffer binding
cbuffer materialData : register(b0, space1)
{
	GLTFMaterialData materialData;
}

[[vk::binding(1/* binding 1 */, 1 /* set 1 */)]] // sampled image descriptor binding
Texture2DArray textures : register(t1);          // binding array to register t1

[[vk::binding(2/* binding 2 */, 1 /* set 1 */)]] // sampler descriptor binding
SamplerState samplerState : register(s2);        // binding sampler to register s0