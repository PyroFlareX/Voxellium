#version 450

#extension GL_ARB_separate_shader_objects : enable

struct outVert
{
	vec3 fragPos;
	vec3 normal;
	vec2 textureCoordinates;
};

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

layout (location = 0) out outVert outVertShader;

layout (set = 0, binding = 0) uniform MVP
{
	mat4 proj;
	mat4 view;
	mat4 model;
} CameraData;

layout ( push_constant ) uniform constants
{
	vec4 textureid;
} PushConstants;

vec3 colors[3] = vec3[]
(
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0)
);

void main()
{
	outVertShader.fragPos = vec3(CameraData.model * vec4(aPos, 1.0));
	outVertShader.textureCoordinates = aTexCoord;
	outVertShader.normal = vec3(CameraData.model * vec4(aNormal, 0.0));
	
	gl_Position = CameraData.proj * CameraData.view * CameraData.model * vec4(aPos, 1.0);

	//TBN Matrix Starting BS
	
	gl_Position.y = -gl_Position.y;	//HACK
}