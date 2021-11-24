#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require

struct vertexOutputData
{
	vec3 fragPos;
	vec3 normal;
	vec2 textureCoordinates;
	vec4 textureIDFiller;
};

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vertexOutputData vertexData;

layout (set = 0, binding = 0) uniform MVP
{
	mat4 proj;
	mat4 view;
} CameraData;

layout (set = 0, binding = 1) uniform sampler2D textures[];

void main()
{
	const vec3 lightPos = vec3(10.0, 256.0, 10.0);
	const vec3 lightColor = vec3(1.0, 1.0, 1.0);
	const float ambientStrength = 0.1;

	//Ambient Lighting Value
    const vec3 ambient = ambientStrength * lightColor;

	//Direction from light for diffuse
	const vec3 lightDir = normalize(lightPos - vertexData.fragPos);
	vec3 normal = 2 * normalize(vertexData.normal);
	float diff = max(dot(normal, lightDir), 0.0);

	//Actual diffuse
	vec3 diffuse = diff * lightColor;
	
	//No specular

	//Here's the result
	vec4 result = vec4(ambient + diffuse, 1.0);

	//Texture Index
	const int t_index = int(vertexData.textureIDFiller.x);
	
	FragColor = texture(textures[t_index], vertexData.textureCoordinates) * result;
} 