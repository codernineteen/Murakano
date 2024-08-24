struct PSInput
{
	[[vk::location(0)]] float2 UV : TEXCOORD0;
};

[[vk::combinedImageSampler]]
Texture2D inTexture : register(t3); // bindding point 0

[[vk::combinedImageSampler]]
SamplerState samplerState : register(s3); // binding point 0

float4 main(PSInput input) : SV_Target
{
	float2 uv = input.UV;
	// gamma correction
	//float gamma = 1.0f / 2.2f;
	//float4 outColor = pow(inTexture.Sample(samplerState, uv).rgba, float4(gamma, gamma, gamma, gamma)); 

	float4 outColor = inTexture.Sample(samplerState, uv);

	return outColor;
}