struct VSInput
{
    [[vk::location(0)]] float2 Position : POSITION0;
    [[vk::location(1)]] float3 Color : COLOR0;
    [[vk::location(2)]] float2 TexCoord : TEXCOORD0;
};

struct UBO
{
    float4x4 mvpMat;
};

cbuffer ubo : register(b0, space0)
{
    UBO ubo;
}

struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float3 Color : COLOR0;
    [[vk::location(1)]] float2 TexCoord : TEXCOORD0;
};

VSOutput main(VSInput input, uint VertexIndex : SV_VertexID)
{
    VSOutput output = (VSOutput) 0;
    output.Color = input.Color;
    
    float4 pos = float4(input.Position, 0.0f, 1.0f);
    output.Pos = mul(pos, ubo.mvpMat);
    output.TexCoord = input.TexCoord;
   
    return output;
}