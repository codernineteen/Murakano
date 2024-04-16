#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require


#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT HitPayload prd;

layout(push_constant) uniform PushConstantRay
{
	vec4  clearColor;
	vec3  lightPos;
	float lightIntensity;
	int   lightType;
} pcr;


void main() {
	//prd.hitValue = pcr.pcRay.clearColor.xyz * 0.8;
}


