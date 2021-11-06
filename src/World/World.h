#pragma once

#include <unordered_map>
#include <memory>

#include "Types/Chunk.h"
#include "Meshing/ChunkMesher.h"

class World
{
	using ChunkMap = std::unordered_map<pos_xyz, Chunk>;
public:
	World();
	~World();

	block_t getBlockAt(const pos_xyz& world_pos) const noexcept;
	void setBlockAt(const pos_xyz& world_pos, block_t block) noexcept;

	const Chunk& getChunkAt(const pos_xyz& chunk_coords_pos) const noexcept;
	Chunk& getChunkAt(const pos_xyz& chunk_coords_pos);

	std::shared_ptr<ChunkMap> getWorldMap();
private:
	std::shared_ptr<ChunkMap> m_baseWorld;
};