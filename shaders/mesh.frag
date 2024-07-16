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

vec3 calcPointLight(vec3 pos,  vec4 Acolr, vec3 FragPos, vec3 FragNormal)
{
	vec3 dirtoLight = pos - FragPos;
	//debugPrintfEXT("dir to light is %f %f %f\n", dirtoLight.x, dirtoLight.y, dirtoLight.z);
	float distance    = length(pos - FragPos);
	float att = 1.0 / (1 + 0.09 * distance + 
    		    0.032 * (distance * distance));    
	//float att = 1.0 / 0.09 * 0.032 * dot(dirtoLight, dirtoLight);
	//debugPrintfEXT("My att is %f\n", att);

	vec3 colour = vec3(1) * att;
	vec3 ambColour = Acolr.xyz * Acolr.w * att;
	
	vec3 diffuse = colour * max(0, dot(normalize(FragNormal), normalize(dirtoLight)));

	return (diffuse + ambColour);
}
vec3 calcSpotLight(vec4 pos, vec4 dir)
{
	vec4 TorchClr = vec4(1,1,1,1);
	vec3 diffuse;
	if (pos.w > 0) // enabled
	{
		vec3 dirtoLight = pos.xyz - inFragPos;
		// do spotlight calc
		vec3 dirN = dir.xyz;
		float theta = dot(dirtoLight, normalize(dirN));
		if (theta > 12.5) // if less than the cutoff range
		{
				float distance    = length(pos.xyz - inFragPos);
				float att = 1.0 / (1 + 0.09 * distance + 
    		    0.032 * (distance * distance));    
				vec3 colour = TorchClr.xyz * TorchClr.w * att;
				diffuse = colour * max(0, dot(normalize(inFragNormal), normalize(dirtoLight)));

				return diffuse + colour;

		}
		else
		{
			return vec3(0);			
		}
	
	}
	else
	{
		return vec3(0);
	}

	return diffuse;

}
void main() 
{
	vec4 sDir = sceneData.sun.Dir;
	vec4 sClr = sceneData.sun.Clr;

	vec4 slPos = sceneData.Torch.Position;
	vec4 slDir = sceneData.Torch.Direction;
	//debugPrintfEXT("PointLight clr - %f %f %f %f \n", plColour.x, plColour.y, plColour.z,plColour.w);
	//debugPrintfEXT("PointLight Aclr - %f %f %f %f \n", plAmbColour.x, plAmbColour.y, plAmbColour.z,plAmbColour.w);

	vec3 color = inColor * texture(colorTex,inUV).xyz;

	vec3 lighting = calcDirectLight(inFragNormal, sDir, sClr.xyz);
	
	for (int x = 0; x < 8; ++x)
	{
		int y = x;
		vec4 plPos = sceneData.light[y].Position;
		vec4 plAmbColour = sceneData.light[y].Ambient_Colour;
		lighting +=  calcPointLight(plPos.xyz, plAmbColour, inFragPos, inFragNormal);
	}
	
	outFragColor = vec4(color * ( lighting ),1.0f); // 

	



	
	
}