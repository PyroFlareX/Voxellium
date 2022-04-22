#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require

// #extension GL_EXT_shader_16bit_storage : require

const int textureBindingSlot = 7;

struct vertexOutputData
{
	vec3 fragPos;
	vec3 normal;
	vec2 textureCoordinates;
	vec4 textureIDFiller;
};

//Normal Attributes
layout( location = 0 ) in vec4 packedData;

//Instanced Attributes
layout( location = 1 ) in ivec3 chunkPos;
layout( location = 2 ) in uint textureBufferOffset;	//This is derivable from the InstanceID I think?

//The output data to the fragment shader
layout ( location = 0 ) out vertexOutputData vertexData;

//Descriptor Set Buffers
layout ( set = 0, binding = 0 ) uniform MVP
{
	mat4 proj;
	mat4 view;
	mat4 model;
} CameraData;

const int CHUNK_SIZE = 16;
const int CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE;
const int CHUNK_VOLUME = CHUNK_AREA * CHUNK_SIZE;
const int NUM_SIDES = 6;

const int NUM_FACES_IN_CHUNK = CHUNK_VOLUME * NUM_SIDES;
layout ( set = 0, binding = 1, std430 ) readonly buffer TextureLayoutData
{
	uint /*uint16_t*/ faceTexture[NUM_FACES_IN_CHUNK];
} faceTextures[];

//FUNCTIONS FOR THE SHADER

//Unpack the direction from the packed data
int getDirection(float packedw)
{
	return int(round(mod(packedw, 100.0f) / 10.0f) - 1);
}

//Get the vertex normal from the packed data
vec3 getNormal(float packedw)
{
	const vec3 directions[6] = 
	{
		vec3(0.0f, 0.0f, -1.0f),	//F
		vec3(0.0f, 0.0f, 1.0f),		//B
		vec3(0.0f, 1.0f, 0.0f),		//U
		vec3(0.0f, -1.0f, 0.0f),	//D
		vec3(-1.0f, 0.0f, 0.0f),	//L
		vec3(1.0f, 0.0f, 0.0f),		//R
	};
	return directions[getDirection(packedw)];
}

//Get which corner, 1, 2, 3, or 4 them vertex is from the packed data
int getCorner(float packedw)
{
	return int(mod(packedw, 10.0f));
}

//Get the texture coordinates of the vertex from the corner
vec2 getTextureCoordinates(int corner)
{
	if(corner == 1)
	{	//BL
		return vec2(0.0f, 0.0f);
	}
	else if(corner == 2) 
	{	//BR
		return vec2(1.0f, 0.0f);
	}
	else if(corner == 3) 
	{	//TR
		return vec2(1.0f, 1.0f);
	}
	else// if(corner == 4) 
	{	//TL
		return vec2(0.0f, 1.0f);
	}	
}

//Get which block this is from the packed data
int getBlockIndex(float packedw)
{
	return int(round(packedw / 100.0f));
}

//Get which face this is from the packed data
int getFaceIndex(float packedw)
{
	return getBlockIndex(packedw) + CHUNK_VOLUME * getDirection(packedw);
}

void main()
{
	const float directionCornerInfo = packedData.w;
	//Unpacking the packed vertex data
	vertexData.textureCoordinates = getTextureCoordinates(getCorner(directionCornerInfo));
	vertexData.normal = getNormal(directionCornerInfo);
	vertexData.textureIDFiller = vec4(0.0, 0.0, 0.0, 0.0);

	//Calculate via offsets the textureID for this face
	int	faceID = getFaceIndex(directionCornerInfo);
	int textureID = (faceID % 2) + 1; //int(faceTextures[textureBufferOffset].faceTexture[faceID]); // = 1;
	vertexData.textureIDFiller.x = float(textureID);

	//Calculate vertex positions
	const vec3 vertexPos = packedData.xyz + (16.0f * chunkPos.xyz);
	vertexData.fragPos = vertexPos;
	gl_Position = CameraData.proj * CameraData.view * vec4(vertexPos, 1.0);
	
	//Flipping y axis bc vulkan
	gl_Position.y = -gl_Position.y;	//HACK
}