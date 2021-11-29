#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require

const int textureBindingSlot = 7;

struct vertexOutputData
{
	vec3 fragPos;
	vec3 normal;
	vec2 textureCoordinates;
	vec4 textureIDFiller;
};

//Fragment output
layout (location = 0) out vec4 FragColor;

//Input from vertex shader
layout (location = 0) in vertexOutputData vertexData;

//Descriptor set buffers
layout ( set = 0, binding = textureBindingSlot ) uniform sampler2D textures[];

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
	const int t_index = 0;//int(vertexData.textureIDFiller.x);
	
	//Output
	FragColor = texture(textures[t_index], vertexData.textureCoordinates) * result;
} 