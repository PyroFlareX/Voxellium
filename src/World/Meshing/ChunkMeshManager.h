#pragma once

#include "../Types/Chunk.h"

#include <shared_mutex>

class World;

/** Other specifications and info
 * 
 * ALL const functions are guarunteed to be thread safe
 * 
 * Unless stated, all other functions are NOT thread safe
 * 
 * This is SOLELY a rendering and " state 'copy' " system
 * 	Meaning that:
 * 		- It may NEVER modify the world or chunk data [possible exception for rendering exclusive flags for a chunk]
 * 		- Its state is independent from the world, changes to the world do not modify mesh data until it is told to rebuild
 * 		- Game ticks are FULLY INDEPENDENT from rendering and frame updates
 * 		- It can render chunks that may no longer be active in the game state, and continues to draw them until a "cleaning"
 * 			is needed
 * 
 * Other weirdness I am predicting:
 * 		> If a chunk needs to be updated, and the size is bigger, it likely will have to be moved to a different location
 * 			and have to modify the open slots for it's old spot
 * 			 If size is equal, take over the slot, and overwrite the old data
 * 			 If size is smaller, then overwrite old data, and free the leftover allocation space
 * 		> There is a high chance with the implementation idea I have right now that like 2-4 mutexes will be used
 * 
**/

//Chunk Mesh Manager
//This keeps track of the data for the built chunk meshes
//Makes sure the chunk slots are valid and properly managed
class ChunkMeshManager
{
public:
	//The world is used for mesh building, renderDistance is for the buffer allocation size
	ChunkMeshManager(const World& world, const u32 renderDistance);
	~ChunkMeshManager();

	//Set the render distance in number of chunks, as a radius around the player
	void setRenderDistance(const u32 renderDistance);

	//Tries to cache the chunk passed
	//Returns true if successful, false otherwise (like if there is no more room in the buffer)
	// 		@TODO: TRY TO MAKE THIS BE THREAD SAFE
	bool cacheChunk(const Chunk& chunk);

	//Marks the passed chunk as removable from the list
	// The time of removal is arbitrary after this is called
	// Thread safe
	void canDrop(const pos_xyz chunkPosition);
	//Marks the passed chunk as removable from the list
	// The time of removal is arbitrary after this is called
	// Thread safe
	void canDrop(const Chunk& chunk);

	//Checks whether the chunk is in the drawlist
	// Returns false if it is queued to be removed
	bool isChunkCached(const pos_xyz chunkPosition) const;
	//Checks whether the chunk is in the drawlist
	// Returns false if it is queued to be removed
	bool isChunkCached(const Chunk& chunk) const;
	
	//Gets the number of chunks in the buffer
	u32 getNumChunks() const;

	//Get the draw datas
	const std::vector<Chunk::ChunkMesh>& getChunkDrawData() const;	

private:
	///Private Member Functions

	//Add the given chunk to the buffer (might add a location offset argument)
	void addChunkToBuffer(const Chunk::ChunkMesh chunk);

	//Compresses and realigns the space and offsets within the buffer
	void condenseBuffer();

	void reallocateBuffers();

	ChunkDrawInfo createDrawInfoFromChunk(const Chunk& chunk) const;

	struct IndexMesh
	{
		std::vector<u32> meshindicies;
	};
	const IndexMesh buildIndexMesh(const ChunkDrawInfo& drawInfo) const;

	i64 findOpenSlot(const u32 data_length) const;
	bool reserveSlot(const u32 start, const u32 data_length);
	u32 reserveOpenSlot(const u32 data_length);

	///Member variables

	//World Ref
	const World& m_world;

	//Render Distance
	u32 m_renderDistance;

	// @TODO: Fix the const functions or implement this in a slightly different way somehow so that
	//		the const member functions can shared_lock these too maybe?
	//The Various mutex(es)
	std::shared_mutex m_slot_lock;
	std::shared_mutex m_drop_lock;
	std::shared_mutex m_cache_lock;

	//Stores the open areas of the buffer
	struct span
	{
		u32 start;
		u32 length;
	};
	std::vector<span> m_open_spans;

	//The actively drawn chunks
	std::vector<pos_xyz> m_activeChunks;
	//Use like a GC
	std::vector<pos_xyz> m_droppableChunks;

	//Chunk Draw Info
	std::vector<Chunk::ChunkMesh> m_chunk_draw_data;
};
