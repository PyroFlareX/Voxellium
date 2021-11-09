#pragma once

//Constants data
constexpr auto CHUNK_SIZE = 16;
constexpr auto CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE;
constexpr auto CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

#include <array>
#include <optional>
#include <Resources/Mesh.h>

#include <Types/BaseInheritables.h>
#include "AliasTypes.h"

class Chunk	:	public bs::NonCopyable
{
public:
	Chunk(pos_xyz pos);

	//Get the block id for the block at the given position
	block_t getBlockAt(const pos_xyz& local_position) const;
	//Get the block id for the block at the given position
	block_t& getBlockAt(const pos_xyz& local_position);
	//Set a block at the relative position
	void setBlockAt(const pos_xyz& local_position, block_t block);
	//Get position
	pos_xyz getChunkPos() const noexcept;
	//Checks if the chunk has any blocks (non air)
	bool isEmpty() const noexcept;
	//Returns whether the chunk needs a mesh built / has been updated
	bool needsMesh() const noexcept;
	//Set the flag so that only one remesh on this is active
	void setRemeshingFlag();

	//to reset the mesh for the chunk
	void setMesh(bs::Mesh&& new_mesh);

	using chunk_mesh_t = std::optional<bs::Mesh>;
	const chunk_mesh_t& getChunkMesh() const;

	//Checks whether empty
	bool checkIfEmpty() const noexcept;

private:
	/// Members
	//Array of block ids, to store the data for the chunk
	std::array<block_t, CHUNK_VOLUME> m_chunk_layout;
	//The local position of the chunk in world (as chunk coordinates)
	const pos_xyz m_pos;
	//Caches whether the chunk is empty
	bool m_empty;
	//Whether the chunk has a mesh
	chunk_mesh_t m_mesh;
	
	//Set when the chunk needs to be updated too (?)
	//Whether the chunk wants a new mesh built
	bool m_needs_mesh;
};
