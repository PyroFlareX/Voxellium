#include "World.h"

//Filler chunk, TEMP, REMOVE LATER
const Chunk basic_chunk({0, 0, 0});

World::World()
{

}

block_t World::getBlockAt(const pos_xyz& world_pos) const noexcept
{
	const pos_xyz chunk_coord(world_pos.x / CHUNK_SIZE, world_pos.y / CHUNK_SIZE, world_pos.z / CHUNK_SIZE);
	const pos_xyz local_coord(world_pos.x % CHUNK_SIZE, world_pos.y % CHUNK_SIZE, world_pos.z % CHUNK_SIZE);

	return getChunkAt(chunk_coord).getBlockAt(local_coord);
}

const Chunk& World::getChunkAt(const pos_xyz& chunk_coords_pos) const noexcept
{
	const auto searching = m_baseWorld.find(chunk_coords_pos);
	if(searching != m_baseWorld.cend())
	{
		return searching->second;
	}
	else
	{
		//Build chunk (queue as a job)

		//for now returning a empty default chunk
		return basic_chunk;
	}
}
