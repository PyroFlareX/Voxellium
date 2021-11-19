#pragma once

#include <array>

#include "../World.h"


// List out the vertices, indicies, UV coords, etc for the models
// for the blocks
// In the future, maybe have this be on the GPU side, and use a compute shader?

//Cube coords
constexpr std::array<bs::vec3, 8> vertices
{
	//8 vertices, one per corner
	//Starting with 0,0,0 or left bottom front
	bs::vec3{ 0.0f, 0.0f, 0.0f },	//0	//Left,		Bottom,	Front
	{ 1.0f, 0.0f, 0.0f },	//1	//Right,	Bottom,	Front
	{ 1.0f, 1.0f, 0.0f },	//2	//Right,	Top,	Front
	{ 0.0f, 1.0f, 0.0f },	//3	//Left,		Top,	Front
	{ 0.0f, 0.0f, 1.0f },	//4	//Left,		Bottom,	Back
	{ 1.0f, 0.0f, 1.0f },	//5	//Right,	Bottom,	Back
	{ 1.0f, 1.0f, 1.0f },	//6	//Right,	Top,	Back
	{ 0.0f, 1.0f, 1.0f }	//7	//Left,		Top,	Back
	//8 total
};
//Cube faces
//Outside is counter-clockwise
constexpr std::array<u32, 6> front
{
	0, 1, 2,
	2, 3, 0
};
constexpr std::array<u32, 6> back
{
	5, 4, 7,
	7, 6, 5
};
constexpr std::array<u32, 6> top
{
	3, 2, 6,
	6, 7, 3
};
constexpr std::array<u32, 6> bottom
{
	1, 0, 4,
	4, 5, 1
};
constexpr std::array<u32, 6> left
{
	0, 3, 7,
	7, 4, 0
};
constexpr std::array<u32, 6> right
{
	2, 1, 5,
	5, 6, 2
};

//Magic numbers for the above indices
constexpr auto BOTTOM_LEFT = 0;
constexpr auto BOTTOM_RIGHT = 1;
constexpr auto TOP_RIGHT = 2;
constexpr auto TOP_LEFT = 4;

// Some constants
constexpr pos_xyz UP(0, 1, 0);
constexpr pos_xyz DOWN(0, -1, 0);
constexpr pos_xyz LEFT(-1, 0, 0);
constexpr pos_xyz RIGHT(1, 0, 0);
constexpr pos_xyz FRONT(0, 0, -1);
constexpr pos_xyz BACK(0, 0, 1);
constexpr pos_xyz NONE(0, 0, 0);

// Meshing constants
constexpr auto NUM_SIDES = 6;
constexpr auto NUM_FACES_PER_SIDE = CHUNK_VOLUME;
constexpr auto NUM_VERTS_PER_SIDE = NUM_FACES_PER_SIDE * 4;
constexpr auto NUM_FACES_IN_FULL_CHUNK = NUM_FACES_PER_SIDE * NUM_SIDES;
constexpr auto NUM_VERTS_IN_FULL_CHUNK = NUM_FACES_IN_FULL_CHUNK * 4;

enum class dir
{
	FRONT = 0,
	BACK,
	TOP,
	BOTTOM,
	LEFT,
	RIGHT,

	NUM_SIDES
};

///Listing a bunch of various struct layouts that could be possibly used for storing the data
/*
//AbsFace Description
struct AbsBlockFace
{
	u16 textureID;
	std::array<u16, 6> faceIndices;
};

struct FaceDescription
{
	//The Texture ID of the block face
	u16 textureID;
	//From 0 to 24576 (log2(24576) = 14.5849...), so 16 bits works
	u16 faceID;

	//Unsure about including this or not, but have the AO lighting here maybe?
};

//Chunk Description
struct ChunkDescription
{
	//For position of chunk, probably could change to a vec3
	bs::mat4 chunkTransform;
	
	std::vector<FaceDescription> facesList;
};

struct ChunkMesh
{
	bs::vec3 chunkTransform;

	std::vector<FaceDescription> chunkFaces;
};
*/