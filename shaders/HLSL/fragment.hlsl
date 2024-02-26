// pixel shader input
struct PSInput
{
    float3 Color : COLOR;
    float2 TexCoord : TEXCOORD;
};

// texture sampler binding
[[vk::combinedImageSampler]]
Texture2D texSampler : register(t1);
[[vk::combinedImageSampler]]
SamplerState samplerState : register(s1);


float4 main(PSInput input) : SV_Target
{

    float4 outColor = texSampler.Sample(samplerState, input.TexCoord);
    return outColor;
}