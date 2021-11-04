#pragma once

//Constants data
constexpr auto CHUNK_SIZE = 16;
constexpr auto CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE;
constexpr auto CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

#include "AliasTypes.h"
#include <array>

class Chunk
{
public:
	Chunk(pos_xyz pos);

	//Get the block id for the block at the given position
	block_t getBlockAt(const pos_xyz& local_position) const;
	//Get the block id for the block at the given position
	block_t& getBlockAt(const pos_xyz& local_position);	

private:
	/// Members
	//Array of block ids, to store the data for the chunk
	std::array<block_t, CHUNK_VOLUME> m_chunk_layout;
	//The local position of the chunk in world (as chunk coordinates)
	const pos_xyz m_pos;
	//Caches whether the chunk is empty
	bool m_empty;
	//Whether the chunk has a mesh
	bool m_has_mesh;
	//Whether the chunk wants a new mesh built, trigger when there is a change
	bool m_needs_mesh;

	/// Functions
	//Checks whether empty
	bool checkIfEmpty() const noexcept;
	//Returns whether the chunk needs a mesh built
	bool needsMesh() const noexcept;
};
