#version 450

#extension GL_EXT_debug_printf : enable

#extension GL_GOOGLE_include_directive : require


#include "input_structures.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inFragPos;
layout (location = 4) in vec3 inFragNormal;
layout (location = 0) out vec4 outFragColor;

// vec4 sDir = vec4(1, 1, -1, 1);


//vec3 plPos = vec3(0,0,0);
//vec4 plClr = vec4(0,0,1, 1);
//vec4 plAClr = vec4(0,0,1, 1);

vec3 calcDirectLight(vec3 Normal, vec4 Direction, vec3 Colour)
{
	float sunLightValue = max(dot(Normal, Direction.xyz), 0.1f) * Direction.w;
	vec3 dirtoLight = -Direction.xyz;
	float att = 1.0 / dot(dirtoLight, dirtoLight); // square length
	vec3 sunClr = Colour * Direction.w * att;
	return (sunLightValue + sunClr);
}

vec3 calcPointLight(vec3 pos, vec4 colr, vec4 Acolr, vec3 FragPos, vec3 FragNormal)
{
	vec3 dirtoLight = pos - FragPos;
	//debugPrintfEXT("dir to light is %f %f %f\n", dirtoLight.x, dirtoLight.y, dirtoLight.z);

	float att = 1.0 / dot(dirtoLight, dirtoLight);
	//debugPrintfEXT("My att is %f\n", att);

	vec3 colour = colr.xyz * colr.w;
	vec3 ambColour = Acolr.xyz * Acolr.w * att;
	
	vec3 diffuse = colour * max(0, dot(normalize(FragNormal), normalize(dirtoLight)));

	return (diffuse + ambColour);
}
void main() 
{
	vec4 sDir = sceneData.sun.Dir;
	vec4 sClr = sceneData.sun.Clr;
	vec4 plPos = sceneData.light.Position;
	vec4 plColour = sceneData.light.Colour;
	vec4 plAmbColour = sceneData.light.Ambient_Colour;
	//debugPrintfEXT("PointLight clr - %f %f %f %f \n", plColour.x, plColour.y, plColour.z,plColour.w);
	//debugPrintfEXT("PointLight Aclr - %f %f %f %f \n", plAmbColour.x, plAmbColour.y, plAmbColour.z,plAmbColour.w);

	vec3 color = inColor * texture(colorTex,inUV).xyz;
	outFragColor = vec4(color  * ( calcDirectLight(inFragNormal, sDir, sClr.xyz) + calcPointLight(plPos.xyz, plColour, plAmbColour, inFragPos, inFragNormal) ),1.0f); // 

	



	
	
}