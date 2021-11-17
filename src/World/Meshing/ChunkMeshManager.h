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

	//Set the render distance in number of chunks, the radius from the player
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
	u32 m_renderDistance;

	

	std::vector<pos_xyz> m_activeChunks;
	std::vector<pos_xyz> m_droppableChunks;
};