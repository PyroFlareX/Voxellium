#pragma once

#include "../Types/Chunk.h"

class World;

//Chunk Mesh Manager
//This keeps track of the data for the built chunk meshes
//Makes sure the chunk slots are valid and properly managed
class ChunkMeshManager
{
public:
	ChunkMeshManager(const World& world);
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
	//Add the given chunk to the buffer (might add a location offset argument)
	void addChunkToBuffer(const Chunk& chunk);

	//Compresses and realigns the space and offsets within the buffer
	void condenseBuffer();

	ChunkDrawInfo createDrawInfoFromChunk(const Chunk& chunk) const;
	static constexpr std::array<u32, 6> getIndicesFromFaceIndex(const u16 faceIndex);

	struct IndexMesh
	{
		std::vector<u32> meshindicies;
	};
	const IndexMesh buildIndexMesh(const ChunkDrawInfo& drawInfo) const;

	i64 findOpenSlot(const u32 data_length);


	///Member variables
	//World Ref
	const World& m_world;

	//Render Distance
	u32 m_renderDistance;

	//Stores the open areas of the buffer
	struct span
	{
		u32 start;
		u32 length;
	};
	std::vector<span> m_open_spans;

	//Draw Data Holder
	std::vector<Chunk::ChunkMesh> m_chunk_draw_data;

	//The actively drawn chunks
	std::vector<pos_xyz> m_activeChunks;
	//Use like a GC
	std::vector<pos_xyz> m_droppableChunks;
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
	constexpr std::array<u32, 6> front
	{
		0, 1, 2,
		2, 3, 0
	};

	//The offset
	const u32 baseIndex = faceIndex * 4;

	return std::array<u32, 6>
	{{
		baseIndex + front[0], baseIndex + front[1], baseIndex + front[2],
		baseIndex + front[3], baseIndex + front[4], baseIndex + front[5]
	}};
}