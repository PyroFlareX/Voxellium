#include "World.h"

World::World()
{

}

block_t World::getBlockAt(const bs::vec3i& world_pos) const noexcept
{
	const bs::vec3i chunk_coord(world_pos.x / CHUNK_SIZE, world_pos.y / CHUNK_SIZE, world_pos.z / CHUNK_SIZE);
	const bs::vec3i local_coord(world_pos.x % CHUNK_SIZE, world_pos.y % CHUNK_SIZE, world_pos.z % CHUNK_SIZE);

	return getChunkAt(chunk_coord).getBlockAt(local_coord);
}

const Chunk& World::getChunkAt(const bs::vec3i& chunk_coords_pos) const noexcept
{
	const auto searching = m_baseWorld.find(chunk_coords_pos);
	if(searching != m_baseWorld.cend())
	{
		return searching->second;
	}
	else
	{
		//Build chunk
	}

}
