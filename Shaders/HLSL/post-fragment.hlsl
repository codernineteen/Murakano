struct PSInput
{
    float4 pos : SV_POSITION;
	[[vk::location(0)]] float2 OutUV : TEXCOORD0;
};

[[vk::combinedImageSampler]]
Texture2D inTexture : register(t0); // bindding point 0

[[vk::combinedImageSampler]]
SamplerState samplerState : register(s0); // binding point 0

float4 main(PSInput input) : SV_Target
{
	float2 uv = input.OutUV;
	float gamma = 1.0f / 2.2f;
	float4 outColor = pow(inTexture.Sample(samplerState, uv).rgba, float4(gamma, gamma, gamma, gamma)); // sample color from texture.

	return outColor;
}