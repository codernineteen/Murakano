struct VSOutput
{
    float4 pos : SV_POSITION;
    [[vk::location(0)]] float2 OutUV : TEXCOORD0;
};

/*
* ----- Main ------
*/


VSOutput main(uint VertexIndex : SV_VertexID)
{
    VSOutput output = (VSOutput)0;

    output.OutUV = float2((VertexIndex << 1) & 2, VertexIndex & 2);
    output.pos = float4(output.OutUV * 2.0f - 1.0f, 1.0f, 1.0f);

    return output;
}