struct VSInput
{
    [[vk::location(0)]] float3 Position : POSITIONT0;
    [[vk::location(1)]] float3 Color : COLOR0;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]]   float3 Color : COLOR0;
};

// hard-coded positions

VSOutput main(VSInput input, uint VertexIndex : SV_VertexID)
{
    VSOutput output = (VSOutput)0;

    output.Pos = float4(input.Position, 1.0);
    output.Color = input.Color;

    return output;
}