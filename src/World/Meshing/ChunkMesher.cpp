#include "ChunkMesher.h"

#include "../World.h"



std::shared_ptr<bs::Mesh> generateMeshFor(World& world, const pos_xyz& chunk_coord)
{
	const auto& chunk = world.getChunkAt(chunk_coord);

	return std::shared_ptr<bs::Mesh>(nullptr);
}