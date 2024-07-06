

struct DirLight {
    vec3 Dir;
    vec4 Clr;
  
    
};  

layout(set = 0, binding = 0) uniform  SceneData{   

	mat4 view;
	mat4 proj;
	mat4 viewproj;
	//vec4 ambientColor;
	
	vec4 sDir; //w for sun power
	vec3 sClr;
	
	DirLight sun;	// Struct for above

	//vec3 pointLightPos;
	//vec4 pointLightAmbientClr;
	//vec4 pointLightClr;
	
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

