#pragma once

#include "../Types/Chunk.h"

#include <mutex>

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

	// void updateChunk();

	//Compresses and realigns the space and offsets within the buffer
	void condenseBuffer();

	ChunkDrawInfo createDrawInfoFromChunk(const Chunk& chunk) const;
	static constexpr std::array<u32, 6> getIndicesFromFaceIndex(const u16 faceIndex);

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

	//The Various mutex(es)
	std::mutex m_slot_lock;
	std::mutex m_drop_lock;
	std::mutex m_cache_lock;
	std::mutex m_glob_lock;

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

constexpr std::array<u32, 6> ChunkMeshManager::getIndicesFromFaceIndex(const u16 faceIndex)
{
	// Meshing constants
	constexpr auto NUM_SIDES = 6;
	constexpr auto NUM_FACES_PER_SIDE = CHUNK_VOLUME;
	constexpr auto NUM_VERTS_PER_SIDE = NUM_FACES_PER_SIDE * 4;
	constexpr auto NUM_FACES_IN_FULL_CHUNK = NUM_FACES_PER_SIDE * NUM_SIDES;
	constexpr auto NUM_VERTS_IN_FULL_CHUNK = NUM_FACES_IN_FULL_CHUNK * 4;
	constexpr auto NUM_INDICES_IN_FULL_CHUNK = NUM_FACES_IN_FULL_CHUNK * 6;
	
	//Indices order
	constexpr std::array<u32, 6> indicesList
	{
		0, 1, 2,
		2, 3, 0
	};

	//The offset
	const u32 baseIndex = faceIndex * 4;

	return std::array<u32, 6>
	{{
		baseIndex + indicesList[0], baseIndex + indicesList[1], baseIndex + indicesList[2],
		baseIndex + indicesList[3], baseIndex + indicesList[4], baseIndex + indicesList[5]
	}};
}