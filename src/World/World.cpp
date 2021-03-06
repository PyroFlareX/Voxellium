#include "World.h"

#include <Engine.h>

#include <mutex>

World::World()	:	m_mesh_manager(*this, 2)
{
	m_baseWorld = std::make_shared<ChunkMap>();
	m_baseWorld->reserve(CHUNK_VOLUME);

	//Generate some chunks
	constexpr auto min = -4;
	constexpr auto max = 4;

	for(auto chunk_x = min; chunk_x < max; ++chunk_x)
	{
		for(auto chunk_y = 0; chunk_y < max; ++chunk_y)
		{
			for(auto chunk_z = min; chunk_z < max; ++chunk_z)
			{
				const pos_xyz chunk_pos(chunk_x, chunk_y, chunk_z);
				m_baseWorld->emplace(chunk_pos, chunk_pos);
			}
		}
	}
}

World::~World()
{
	bs::getJobSystem().waitBackground(0);
}

block_t World::getBlockAt(const pos_xyz& world_pos) const
{
	const pos_xyz chunk_coord(world_pos.x / CHUNK_SIZE, world_pos.y / CHUNK_SIZE, world_pos.z / CHUNK_SIZE);
	const pos_xyz local_coord(world_pos.x % CHUNK_SIZE, world_pos.y % CHUNK_SIZE, world_pos.z % CHUNK_SIZE);

	//Const modding
	const auto& c = const_cast<World*>(this)->getChunkAt(chunk_coord);
	
	return c.getBlockAt(local_coord);
}

bool World::setBlockAt(const pos_xyz& world_pos, block_t block)
{
	const pos_xyz chunk_coord(world_pos.x / CHUNK_SIZE, world_pos.y / CHUNK_SIZE, world_pos.z / CHUNK_SIZE);
	const pos_xyz local_coord(world_pos.x % CHUNK_SIZE, world_pos.y % CHUNK_SIZE, world_pos.z % CHUNK_SIZE);

	auto& c = getChunkAt(chunk_coord);

	c.setBlockAt(local_coord, block);
	return true;
}
/*
const std::optional<Chunk> World::getChunkAt(const pos_xyz& chunk_coords_pos) const noexcept
{
	const auto searching = m_baseWorld->find(chunk_coords_pos);
	if(searching != m_baseWorld->cend())
	{
		return std::make_optional<Chunk>(searching->second);
	}
	else
	{
		//Build chunk (queue as a job)

		// std::cout << "RETURNED BASIC CHUNK!!!\n";

		//for now returning a empty default chunk
		return {};
	}
}*/

std::mutex newChunkMutex;
Chunk& World::getChunkAt(const pos_xyz& chunk_coords_pos)
{
	std::lock_guard<std::mutex> g_mapGuard(newChunkMutex);

	ChunkMap::iterator searching = m_baseWorld->find(chunk_coords_pos);
	if(searching != m_baseWorld->end())
	{
		return searching->second;
	}
	else
	{
		//Build chunk (queue as a job)
		//Change this to actual generation

		//Return the newly generated chunk
		auto newChunk = m_baseWorld->emplace(chunk_coords_pos, chunk_coords_pos);
		return m_baseWorld->at(chunk_coords_pos);
		// return newChunk.first->second;
	}
}

std::shared_ptr<World::ChunkMap> World::getWorldMap()
{
	return m_baseWorld;
}

ChunkMeshManager& World::getMeshManager()
{
	return m_mesh_manager;
}