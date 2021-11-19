#include "Chunk.h"

constexpr static inline bool isValid(const pos_xyz& pos) noexcept
{
	return !((pos.x < 0) || (pos.x > 15) || (pos.y < 0) || (pos.y > 15) || (pos.z < 0) || (pos.z > 15));
}
constexpr static inline bool isNotValid(const pos_xyz& pos) noexcept
{
	return (pos.x < 0) || (pos.x > 15) || (pos.y < 0) || (pos.y > 15) || (pos.z < 0) || (pos.z > 15);
}
constexpr static inline auto toIndex(const pos_xyz& pos) noexcept
{
	// (0, 0, 0) is left, bottom, front
	// (15, 15, 15) is right, top, back
	return (pos.x) + (pos.y * CHUNK_SIZE) + (pos.z * CHUNK_AREA);
}
constexpr static inline auto toIndex(u32 x, u32 y, u32 z) noexcept
{
	// (0, 0, 0) is left, bottom, front
	// (15, 15, 15) is right, top, back
	return x + (y * CHUNK_SIZE) + (z * CHUNK_AREA);
}

constexpr block_t air_id = 0;

Chunk::Chunk(pos_xyz pos)	:	m_pos(pos), m_needs_mesh(true), m_empty(true)
{
	m_chunk_layout.fill(air_id);
	m_empty = checkIfEmpty();
}

block_t Chunk::getBlockAt(const pos_xyz& local_position) const
{
	assert(isValid(local_position));
	return m_chunk_layout.at(toIndex(local_position));
}

block_t& Chunk::getBlockAt(const pos_xyz& local_position)
{
	assert(isValid(local_position));
	return m_chunk_layout.at(toIndex(local_position));
}

void Chunk::setBlockAt(const pos_xyz& local_position, block_t block)
{
	assert(isValid(local_position));
	m_chunk_layout[toIndex(local_position)] = block;
	m_needs_mesh = true;
}

pos_xyz Chunk::getChunkPos() const noexcept
{
	return m_pos;
}

bool Chunk::checkIfEmpty() const noexcept
{
	for(const auto block_id : m_chunk_layout)
	{
		if(block_id != air_id)
		{
			return false;
		}
	}
	return true;
}

bool Chunk::isEmpty() const noexcept
{
	return m_empty;
}

//Returns whether the chunk needs a mesh built
bool Chunk::needsMesh() const noexcept
{
	return m_needs_mesh;
}

void Chunk::setRemeshingFlag()
{
	m_needs_mesh = false;
	m_empty = checkIfEmpty();
}

void Chunk::setMesh(const ChunkMesh managed_mesh)
{
	m_mesh_handle = managed_mesh;
}

bool Chunk::hasMesh() const
{
	return m_mesh_handle.expired();
}
	
Chunk::ChunkMesh Chunk::getChunkMesh() const
{
	return m_mesh_handle.lock();
}