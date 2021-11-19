#pragma once

#include <Types/Types.h>
#include <Resources/Mesh.h>

class World;
class Chunk;

//Generate the mesh for the passed chunk, emplace it into the chunk
bs::Mesh generateMeshFor(const World& world, Chunk& chunk);
//Get the list of vertices (positions) for a chunk with all faces added
//The fourth component contains a packing of the direction and which corner it is in
const std::vector<bs::vec4>& createFullChunkMesh();
