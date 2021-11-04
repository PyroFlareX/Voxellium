#pragma once

#include <unordered_map>
#include <memory>

#include "Types/Chunk.h"

class World
{
public:
	World();
	~World();

	block_t getBlockAt(const pos_xyz& world_pos) const noexcept;
	const Chunk& getChunkAt(const pos_xyz& chunk_coords_pos) const noexcept;

private:
	using ChunkMap = std::unordered_map<pos_xyz, Chunk>;

	ChunkMap& m_baseWorld;
};