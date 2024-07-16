// pixel shader input
struct PSInput
{
    float3 WorldPos    : POSITION0;
    float3 WorldNormal : NORMAL0;
    float3 ViewDir     : VIEW0;
    float2 TexCoord    : TEXCOORD0;
};

struct PushConstantRaster 
{
    float4x4 ModelMat;
    float3   LightPosition;
    float    LightIntensity;
    int      LightType;
};

[[vk::push_constant]]
PushConstantRaster pc;

// texture sampler binding
[[vk::combinedImageSampler]]
Texture2D texSampler : register(t1);
[[vk::combinedImageSampler]]
SamplerState samplerState : register(s1);

static float3 computeDiffuseColor(float4 diffuseColor, float3 viewDir, float3 lightDir, float3 normal)
{
    float dotNL = dot(lightDir, normal);
    float3 outColor = diffuseColor.rgb * saturate(dotNL); // clamp the value between 0 and 1
    return outColor;
}


float4 main(PSInput input) : SV_Target
{

    float4 texColor = texSampler.Sample(samplerState, input.TexCoord);
    float4 outColor = float4(computeDiffuseColor(texColor, input.ViewDir, pc.LightPosition, input.WorldNormal), 1.0f);
    return outColor;
}