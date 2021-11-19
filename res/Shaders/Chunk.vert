#version 450

#extension GL_ARB_separate_shader_objects : enable

struct vertexOutputData
{
	vec3 fragPos;
	vec3 normal;
	vec2 textureCoordinates;
};

layout(location = 0) in vec4 packedData;
layout(location = 0, binding = 1) in vec3 chunkPos;

layout (location = 0) out vertexOutputData vertdata;

layout (set = 0, binding = 0) uniform MVP
{
	mat4 proj;
	mat4 view;
} CameraData;

/*layout ( push_constant ) uniform constants
{
	vec4 textureid;
} PushConstants;*/

vec3 getNormal(float packedw)
{
	const int direction = floor(packedw / 10.0f) - 1;

	const vec3 directions[6] = 
	{
		vec3(0.0f, 0.0f, -1.0f),	//F
		vec3(0.0f, 0.0f, 1.0f),		//B
		vec3(0.0f, 1.0f, 0.0f),		//U
		vec3(0.0f, -1.0f, 0.0f),	//D
		vec3(-1.0f, 0.0f, 0.0f),	//L
		vec3(1.0f, 0.0f, 0.0f),		//R
	}

	return directions[direction];
}

int getCorner(float packedw)
{
	return (int)mod(packedw, 10.0f);
}

vec2 getTextureCoordinates(int corner)
{
	if(index == 1)
	{	//BL
		return vec2(0.0f, 0.0f);
	}
	else if(index == 2) 
	{	//BR
		return vec2(1.0f, 0.0f);
	}
	else if(index == 3) 
	{	//TR
		return vec2(1.0f, 1.0f);
	}
	else if(index == 4) 
	{	//TL
		return vec2(0.0f, 1.0f);
	}
	else {
		//THIS SHOULDN'T HAPPEN UUHHHH
		return vec2(0, 0);
	}
}

void main()
{
	const float directionCornerInfo = packedData.w;

	vertdata.textureCoordinates = getTextureCoordinates(getCorner(directionCornerInfo));
	vertdata.normal = getNormal(directionCornerInfo);

	//Calculate vertex positions
	const vec3 vertexPos = packedData.xyz + chunkPos.xyz;

	vertdata.fragPos = vertexPos;
	gl_Position = CameraData.proj * CameraData.view * vec4(vertexPos, 1.0);
	//Flipping y axis bc vulkan
	gl_Position.y = -gl_Position.y;	//HACK
}