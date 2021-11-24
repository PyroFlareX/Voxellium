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