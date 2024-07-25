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



// ------------------ DEFINITIONS ------------------
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

[[vk::binding(1, 0)]]                   // sampled image descriptor binding
Texture2DArray textures : register(t0); // binding array to register t0

[[vk::binding(0, 1)]]                     // sampler descriptor binding
SamplerState samplerState : register(s0); // binding sampler to register s0

// ------------------ HELPER FUNCTIONS ------------------
static float3 computeDiffuseColor(float4 diffuseColor, float3 viewDir, float3 lightDir, float3 normal)
{
    float dotNL = dot(lightDir, normal);
    float3 outColor = diffuseColor.rgb * saturate(dotNL); // saturate function do clamping the value between 0 and 1
    return outColor;
}

static float3 computeSpecularColor(float4 specularColor, float3 viewDir, float3 lightDir, float3 normal)
{
    float shininess = 2.0f;

    float3 V = normalize(-viewDir);
    float3 R = reflect(-lightDir, normal);
    float dotRV = saturate(dot(V, R)); // clampe the value between 0 and 1
    float specularTerm = pow(dotRV, shininess);

    return specularColor.rgb * specularTerm;
}

static float3 createTestColorBasedNormal(float3 normal)
{
    return dot(float3(1.0f, 1.0f, 1.0f), normal);
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