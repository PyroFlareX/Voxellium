#pragma once

#include <Engine.h>
#include "../Types/Chunk.h"

//Chunk Mesh Manager
//This keeps track of the data for the built chunk meshes
//Makes sure the chunk slots are valid and properly managed
class ChunkMeshManager
{
public:
	ChunkMeshManager();
	~ChunkMeshManager();

	//Set the render distance in number of chunks, as a radius around the player
	void setRenderDistance(const u32 renderDistance);

	//Tries to cache the chunk passed
	//Returns true if successful, false otherwise (like if there is no more room in the buffer)
	bool cacheChunk(const Chunk& chunk);

	//Marks the passed chunk as removable from the list
	// The time of removal is arbitrary after this is called
	void canDrop(const pos_xyz chunkPosition);
	//Marks the passed chunk as removable from the list
	// The time of removal is arbitrary after this is called
	void canDrop(const Chunk& chunk);

	//Checks whether the chunk is in the drawlist
	// Returns false if it is queued to be removed
	bool isChunkCached(const pos_xyz chunkPosition);
	//Checks whether the chunk is in the drawlist
	// Returns false if it is queued to be removed
	bool isChunkCached(const Chunk& chunk);
	

private:
	///Private Member Functions

	void addChunkToBuffer(const Chunk& chunk);
	//Compresses and realigns the space and offsets within the buffer
	void condenseBuffer();

	ChunkDrawInfo createDrawInfoFromChunk(const Chunk& chunk) const;
	static const std::array<u32, 6> getIndicesFromFaceIndex(const u16 faceIndex);

	struct IndexMesh
	{
		std::vector<u32> meshindicies;
	};

	IndexMesh buildIndexMesh(const ChunkDrawInfo& drawInfo) const;


	///Member variables

	u32 m_renderDistance;

	std::vector<Chunk::ChunkMesh> m_chunk_draw_data;

	std::shared_ptr<bs::vk::Buffer> m_data_buffer;	

	//The actively drawn chunks
	std::vector<pos_xyz> m_activeChunks;
	
	//Use like a GC
	std::vector<pos_xyz> m_droppableChunks;
};