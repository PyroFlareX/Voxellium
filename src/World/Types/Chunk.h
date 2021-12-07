#pragma once

//Constants data
constexpr auto CHUNK_SIZE = 16;
constexpr auto CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE;
constexpr auto CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

#include <array>

#include <Types/BaseInheritables.h>
#include "AliasTypes.h"

struct alignas(std::hardware_constructive_interference_size) 
ChunkDrawInfo
{
	pos_xyz chunk_pos;
	struct Face
	{
		u16 textureID;
		u16 faceIndex;
	};

	std::vector<Face> faces;
	u16 numIndices;

	u32 instanceID;
	u32 startOffset;
};	//64 bytes~ aligned, for cache | 56 bytes without align
// constexpr auto drawinfosize = sizeof(ChunkDrawInfo);

struct ChunkInstanceData
{
	pos_xyz position;
	
	//The offset into the storage buffer of where the textureIDs for the faces will be
	//This is equivalent to the index within the Mesh Manager of the chunk
	u32 textureSlotOffset;
};

class Chunk	:	public bs::NonCopyable
{
public:
	Chunk(const pos_xyz pos);

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
	
	//Returns whether the chunk needs to rebuild its mesh
	bool needsMesh() const noexcept;

	//Set the flag so that only one remesh on this is active
	void setRemeshingFlag();

	//Checks whether empty
	bool checkIfEmpty() const noexcept;
	
	//Alias for chunk draw info ptr
	using ChunkMesh = std::shared_ptr<ChunkDrawInfo>;

private:
	/// Members
	//Array of block ids, to store the data for the chunk
	std::array<block_t, CHUNK_VOLUME> m_chunk_layout;
	
	//The local position of the chunk in world (as chunk coordinates)
	const pos_xyz m_pos;

	//Caches whether the chunk is empty
	bool m_empty;
	
	//Set when the chunk needs to be updated too
	//Whether the chunk wants a new mesh built
	bool m_needs_mesh;
};
