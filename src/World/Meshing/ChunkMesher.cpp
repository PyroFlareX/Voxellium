#include "ChunkMesher.h"

#include "../World.h"

#include <Resources/Mesh.h>
#include <memory>
#include <array>

// List out the vertices, indicies, UV coords, etc for the models
// for the blocks

// In the future, maybe have this be on the GPU side, and use a compute shader?

// std::vector<float> 

// Some constants
constexpr pos_xyz UP(0, 1, 0);
constexpr pos_xyz DOWN(0, -1, 0);
constexpr pos_xyz LEFT(-1, 0, 0);
constexpr pos_xyz RIGHT(1, 0, 0);
constexpr pos_xyz FRONT(0, 0, -1);
constexpr pos_xyz BACK(0, 0, 1);
constexpr pos_xyz NONE(0, 0, 0);

//Helper functions
constexpr static inline u32 toIndex(u32 x, u32 y, u32 z) noexcept;
constexpr static inline pos_xyz fromIndex(u32 index) noexcept;

static block_t getBlockAt(const pos_xyz& rel_to_chunk, const Chunk& chunk, const World& world);
static pos_xyz OOBChunkOffset(pos_xyz rel_to_chunk);


void generateMeshFor(World& world, const pos_xyz& chunk_coord)
{
	auto& chunk = world.getChunkAt(chunk_coord);

	/**	Algorithm layout:
	 * For each block, check each block adjacent (6 sides) to see if it is transparent.
	 * If transparent (like air, glass), then add the block face to the mesh.
	 * Otherwise, do not add the face to the mesh.
	 * 
	 * Use the prebaked vertex offsets for a chunk and their index arrays
	 * to avoid extra calculations. Then just make sure the blockface has
	 * a corresponding texture.
	 * 
	 * Some possible optimizations:
	 * Find hints for whether there is a lot of exposed faces, so
	 * 	it can run on the transparent blocks instead of opaque ones.
	 * Magic numbers
	 * 
	 * 
	 * For getting these block render properties, just access the block data
	 * map at the corresponding BlockID.
	 * 
	**/

	for(int z = 0; z < CHUNK_SIZE; ++z)
	{
		for(int y = 0; y < CHUNK_SIZE; ++y)
		{
			for(int x = 0; x < CHUNK_SIZE; ++x)
			{
				const pos_xyz coords(x, y, z);
				auto block = chunk.getBlockAt(coords);

				//Check if each face should be rendered
				//Get each adjacent block
				auto blockUP = getBlockAt(coords + UP, chunk, world);
				auto blockDOWN = getBlockAt(coords + DOWN, chunk, world);
				auto blockLEFT = getBlockAt(coords + LEFT, chunk, world);
				auto blockRIGHT = getBlockAt(coords + RIGHT, chunk, world);
				auto blockFRONT = getBlockAt(coords + FRONT, chunk, world);
				auto blockBACK = getBlockAt(coords + BACK, chunk, world);

				//Check the blocks to the registry with the registry
				//BlockDataRegistry
			}
		}
	}
}

constexpr static inline u32 toIndex(u32 x, u32 y, u32 z) noexcept
{
	return x + (y * CHUNK_SIZE) + (z * CHUNK_AREA);
}

constexpr static inline pos_xyz fromIndex(u32 index) noexcept
{
	return pos_xyz(index % CHUNK_SIZE, (index % CHUNK_AREA) / CHUNK_SIZE, index / CHUNK_AREA);
}

static block_t getBlockAt(const pos_xyz& rel_to_chunk, const Chunk& chunk, const World& world)
{
	const auto offset = OOBChunkOffset(rel_to_chunk);
	if((offset.x + offset.y + offset.z) == 0)
	{
		chunk.getBlockAt(rel_to_chunk);
	}

	const auto& otherChunk = world.getChunkAt(chunk.getChunkPos() + offset);
	const auto newRelToChunk = rel_to_chunk - (offset * CHUNK_SIZE);

	return otherChunk.getBlockAt(newRelToChunk);
}

static pos_xyz OOBChunkOffset(pos_xyz rel_to_chunk)
{
	if(rel_to_chunk.x < 0)
	{
		return LEFT;
	}
	else if(rel_to_chunk.x >= 16)
	{
		return RIGHT;
	}
	else if(rel_to_chunk.y < 0)
	{
		return DOWN;
	}
	else if(rel_to_chunk.y >= 16)
	{
		return UP;
	}
	else if(rel_to_chunk.z < 0)
	{
		return FRONT;
	}
	else if(rel_to_chunk.z >= 16)
	{
		return BACK;
	}
	else
	{
		return NONE;
	}
}