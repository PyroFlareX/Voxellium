#include "World.h"

#include <Engine.h>

//Filler chunk, TEMP, REMOVE LATER
const Chunk basic_chunk({0, 0, 0});

World::World()	:	m_baseWorld(*(new ChunkMap()))
{
	auto* world = &m_baseWorld;

	//Generate some chunks
	for(int chunk_x = -8; chunk_x < 8; ++chunk_x)
	{
		for(int chunk_y = -8; chunk_y < 8; ++chunk_y)
		{
			for(int chunk_z = -8; chunk_z < 8; ++chunk_z)
			{
				const auto generateChunk = jobSystem.createJob([world, chunk_x, chunk_y, chunk_z](Job j)
				{
					const pos_xyz chunk_pos(chunk_x, chunk_y, chunk_z);
					Chunk chunk(chunk_pos);
					for(int x = 0; x < CHUNK_SIZE; ++x)
					{
						for(int y = 0; y < CHUNK_SIZE; ++y)
						{
							for(int z = 0; z < CHUNK_SIZE; ++z)
							{
								const pos_xyz worldpos(chunk_x * CHUNK_SIZE + x, 
														chunk_y * CHUNK_SIZE + y, 
														chunk_z * CHUNK_SIZE + z);
								//Generation coolio!!!

								chunk.setBlockAt({x, y, z}, y & 2);
							}
						}
					}
					world->emplace(std::pair(chunk_pos, std::move(chunk)));
				});
				jobSystem.schedule(generateChunk, false);
			}
		}
	}
}

World::~World()
{
	ChunkMap* map_ptr = &m_baseWorld;
	delete map_ptr;
}

block_t World::getBlockAt(const pos_xyz& world_pos) const noexcept
{
	const pos_xyz chunk_coord(world_pos.x / CHUNK_SIZE, world_pos.y / CHUNK_SIZE, world_pos.z / CHUNK_SIZE);
	const pos_xyz local_coord(world_pos.x % CHUNK_SIZE, world_pos.y % CHUNK_SIZE, world_pos.z % CHUNK_SIZE);

	return getChunkAt(chunk_coord).getBlockAt(local_coord);
}

void World::setBlockAt(const pos_xyz& world_pos, block_t block) noexcept
{
	const pos_xyz chunk_coord(world_pos.x / CHUNK_SIZE, world_pos.y / CHUNK_SIZE, world_pos.z / CHUNK_SIZE);
	const pos_xyz local_coord(world_pos.x % CHUNK_SIZE, world_pos.y % CHUNK_SIZE, world_pos.z % CHUNK_SIZE);

	getChunkAt(chunk_coord).setBlockAt(local_coord, block);
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

Chunk& World::getChunkAt(const pos_xyz& chunk_coords_pos) noexcept
{
	const auto searching = m_baseWorld.find(chunk_coords_pos);
	if(searching != m_baseWorld.cend())
	{
		return searching->second;
	}
	else
	{
		//Build chunk (queue as a job)
		Chunk chunk(chunk_coords_pos);
		//Change this to actual generation

		//Return the newly generated chunk
		auto newChunk = m_baseWorld.emplace(std::pair(chunk_coords_pos, std::move(chunk)));
		return newChunk.first->second;
	}
}