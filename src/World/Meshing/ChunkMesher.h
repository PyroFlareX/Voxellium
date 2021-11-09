#pragma once

#include <Types/Types.h>

class World;
class Chunk;

//Generate the mesh for the passed chunk, emplace it into the chunk
void generateMeshFor(const World& world, Chunk& chunk);
//Get the list of vertices (positions) for a chunk with all faces added
const std::vector<bs::vec3>& createFullChunkMesh();