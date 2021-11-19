#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require

struct vertexOutputData
{
	vec3 fragPos;
	vec3 normal;
	vec2 textureCoordinates;
};

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vertexOutputData vertdata;

layout (set = 0, binding = 0) uniform MVP
{
	mat4 proj;
	mat4 view;
} CameraData;

layout (set = 0, binding = 1) uniform sampler2D textures[];

/*
layout ( push_constant ) uniform constants
{
	vec4 textureid;
} PushConstants;*/

void main()
{
	const vec3 lightPos = vec3(10.0, 256.0, 10.0);
	const vec3 lightColor = vec3(1.0, 1.0, 1.0);
	const float ambientStrength = 0.1;

    const vec3 ambient = ambientStrength * lightColor;

	const vec3 lightDir = normalize(lightPos - vertdata.fragPos);


	//vec3 normal2 = texture(textures[int(PushConstants.textureid.y)], vertdata.textureCoordinates).xyz;
	vec3 normal = 2 * normalize(vertdata.normal);

	//normal = normalize(normal + normal2);

	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = diff * lightColor;
	
	vec4 result = vec4(ambient + diffuse, 1.0);
	
	FragColor = texture(textures[int(PushConstants.textureid.x)], vertdata.textureCoordinates) * result;
} 