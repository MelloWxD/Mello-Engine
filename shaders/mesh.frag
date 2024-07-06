#version 450

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inFragPos;

layout (location = 0) out vec4 outFragColor;

// vec4 sDir = vec4(1, 1, -1, 1);
vec3 sClr = vec3(1, 1, 1);

void main() 
{
	vec4 sDir = sceneData.sDir;
	sClr = sceneData.sClr;
	vec3 color = inColor * texture(colorTex,inUV).xyz;
	
	// Directional light
	float sunLightValue = max(dot(inNormal, sDir.xyz), 0.1f) * sDir.w;
	
	vec3 dirtoLight = -sDir.xyz;
	float att = 1.0 / dot(dirtoLight, dirtoLight); // square length
	
	vec3 sunClr = sClr * sDir.w * att;
	//vec3 sunAmbient = sceneData.pointLightAmbientClr.xyz * sceneData.pointLightAmbientClr.w;
	vec3 diffuse = sunClr * max(0, dot(normalize(inNormal), normalize(dirtoLight)));
	
	
	
	
	
	
	// Sun Directional lighting
	outFragColor = vec4(color * (sunClr * sunLightValue) ,1.0f);
	
	
	
	
}