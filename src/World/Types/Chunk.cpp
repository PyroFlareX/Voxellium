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

Chunk::Chunk(const pos_xyz& pos)	:	m_pos(pos), m_has_mesh(false), m_needs_mesh(true), m_empty(false)
{
	constexpr block_t air_id = 0;
	m_chunk_layout.fill(air_id);

	//Above is same as this:
	/*for(auto z = 0; z < 16; ++z)
	{
		for(auto y = 0; z < 16; ++z)
		{
			for(auto x = 0; z < 16; ++z)
			{
				// const auto index = x + (y * CHUNK_SIZE) + (z * CHUNK_AREA);	// OR
				// const index = toIndex(toIndex({x, y, z}));	// OR
				const auto index = toIndex(x, y, z);
				m_chunk_layout[index] = air_id;
			}
		}	
	}*/

	checkIfEmpty();
}

block_t Chunk::getBlockAt(const pos_xyz& local_position) const
{
	assert(this->isValid(local_position));
	return m_chunk_layout.at(toIndex(local_position));
}

block_t& Chunk::getBlockAt(const pos_xyz& local_position)
{
	assert(this->isValid(local_position));
	return m_chunk_layout.at(toIndex(local_position));
}

bool Chunk::checkIfEmpty() noexcept
{
	constexpr block_t air_id = 0;
	for(const auto block_id : m_chunk_layout)
	{
		if(block_id != air_id)
		{
			m_empty = false;
			return false;
		}
	}
	m_empty = true;
	return true;
}

bool Chunk::needsMesh() const noexcept
{
	return m_needs_mesh;
}