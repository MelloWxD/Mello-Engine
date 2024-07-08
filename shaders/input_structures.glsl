

struct DirLight {
    vec4 Dir;
    vec4 Clr;
  
    
};  
struct PointLight
{
	vec4 Position;
    vec4 Colour;
	vec4 Ambient_Colour;
};

layout(set = 0, binding = 0) uniform  SceneData{   

	mat4 view;
	mat4 proj;
	mat4 viewproj;

	
	DirLight sun;

	PointLight light;
	
} sceneData;

#ifdef USE_BINDLESS
layout(set = 0, binding = 1) uniform sampler2D allTextures[];
#else
layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;
#endif

layout(set = 1, binding = 0) uniform GLTFMaterialData{   

	vec4 colorFactors;
	vec4 metal_rough_factors;
	int colorTexID;
	int metalRoughTexID;
} materialData;

