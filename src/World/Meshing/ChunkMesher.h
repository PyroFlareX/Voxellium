#pragma once


#include <Resources/Mesh.h>
#include "../Types/Chunk.h"

class World;

//Generate the mesh for the passed chunk, CPU Single-Threaded
bs::Mesh generateMeshFor(const World& world, Chunk& chunk);

//Get the list of vertices (positions) for a chunk with all faces added
//The fourth component contains a packing of the direction and which corner it is in
const std::vector<bs::vec4>& createFullChunkMesh();

//Get all the faces for this Chunk
std::vector<ChunkDrawInfo::Face> generateFacesForChunk(const World& world, const Chunk& chunk);

constexpr std::array<u32, 6> getIndicesFromFaceIndex(const u16 faceIndex)
{
	//The offset
	const u32 baseIndex = faceIndex * 4;
	//Indices order
	constexpr std::array<u32, 6> indicesList
	{
		0, 1, 2,
		2, 3, 0
	};

	return std::array<u32, 6>
	{{
		baseIndex + indicesList[0], baseIndex + indicesList[1], baseIndex + indicesList[2],
		baseIndex + indicesList[3], baseIndex + indicesList[4], baseIndex + indicesList[5]
	}};
}