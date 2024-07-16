#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "input_structures.glsl"

struct Vertex {

	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
}; 

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outFragWorldPos;
layout (location = 4) out vec3 outFragNormal;
layout (location = 5) out float affine;
layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

//push constants block
layout( push_constant ) uniform constants
{
	mat4 render_matrix;
	VertexBuffer vertexBuffer;
} PushConstants;

vec3 snap(vec4 vert_pos)
{
    vec4 snappedPos = vec4((vert_pos.xyz / vert_pos.w), 1);
	
    snappedPos.y = floor(snappedPos.y); // snap the vertex to the lower-resolution grid
	snappedPos.x = floor(snappedPos.x); // snap the vertex to the lower-resolution grid
    snappedPos.xyz *= vert_pos.w; // convert back to projection-space
    return snappedPos.xyz;
}

void main() 
{
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
	mat4 normalMatrix = transpose(inverse(PushConstants.render_matrix*sceneData.view));
	vec4 position = vec4(v.position, 1.0f);
	outFragWorldPos = (position * PushConstants.render_matrix).xyz;
	

	//position.xyz = snap(position);
	gl_Position =  sceneData.viewproj * PushConstants.render_matrix * position;
	


	outNormal = vec4(v.normal, 0.f).xyz;
	outNormal = (normalMatrix * vec4(v.normal, 0.f)).xyz;

	outColor = v.color.xyz * materialData.colorFactors.xyz;	
	outUV.x = v.uv_x;
	outUV.y = v.uv_y;
}