/*
* ----- Definition ------
*/
struct VSInput
{
    [[vk::location(0)]] float3 Position : POSITION0;
    [[vk::location(1)]] float3 Normal   : NORMAL0;
    [[vk::location(2)]] float2 TexCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    [[vk::location(0)]] float3 WorldPos    : POSITION0;
    [[vk::location(1)]] float3 WorldNormal : NORMAL0;
    [[vk::location(2)]] float3 ViewDir     : VIEW0;
    [[vk::location(3)]] float2 TexCoord    : TEXCOORD0;
};

struct UBO
{
    float4x4 mvpMat;
    float4x4 viewInverseMat;
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

cbuffer ubo : register(b0, space0)
{
    UBO ubo;
}

/*
* ----- Main ------
*/


VSOutput main(VSInput input, uint VertexIndex : SV_VertexID)
{
    VSOutput output = (VSOutput) 0;
    
    // vec3 to vec4
    float4 pos      = float4(input.Position, 1.0f);

    // calculate the view direction
    float4 cameraOrigin = normalize(mul(float4(4.0f, 0.0f, 0.0f, 1.0f), ubo.viewInverseMat));
    
    output.ViewDir     = normalize(input.Position.xyz - cameraOrigin.xyz);
    output.pos         = mul(pos, ubo.mvpMat); // transform model to clip space
    output.WorldPos    = input.Position;
    output.WorldNormal = normalize(mul(input.Normal, (float3x3)pc.ModelMat)); // downcast model matrix to 3x3 matrix first. Then multiply with normal
    output.TexCoord    = input.TexCoord;
   
    return output;
}