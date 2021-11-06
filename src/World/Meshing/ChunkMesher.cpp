#include "ChunkMesher.h"

#include "../World.h"

#include <Resources/Mesh.h>
#include <memory>
#include <array>
#include <iostream>

//Helper functions
constexpr static inline u32 toIndex(u32 x, u32 y, u32 z) noexcept;
constexpr static inline pos_xyz fromIndex(u32 index) noexcept;

static block_t getBlockAt(const pos_xyz& rel_to_chunk, const Chunk& chunk, const World& world);
static pos_xyz OOBChunkOffset(pos_xyz rel_to_chunk);

static void makeFace(bs::Mesh& chunkmesh, const pos_xyz& block_pos, const pos_xyz& direction, const std::vector<u32>& face);

static inline bool tempisTransparent(block_t b)
{
	return true; //(b == 0);
}

// List out the vertices, indicies, UV coords, etc for the models
// for the blocks
// In the future, maybe have this be on the GPU side, and use a compute shader?

//Cube coords
const std::vector<bs::vec3> vertices =
{
	//8 vertices, one per corner
	//Starting with 0,0,0 or left bottom front
	{0.0f, 0.0f, 0.0f},	//0	//Left,		Bottom,	Front
	{1.0f, 0.0f, 0.0f},	//1	//Right,	Bottom,	Front
	{1.0f, 1.0f, 0.0f},	//2	//Right,	Top,	Front
	{0.0f, 1.0f, 0.0f},	//3	//Left,		Top,	Front
	{0.0f, 0.0f, 1.0f},	//4	//Left,		Bottom,	Back
	{1.0f, 0.0f, 1.0f},	//5	//Right,	Bottom,	Back
	{1.0f, 1.0f, 1.0f},	//6	//Right,	Top,	Back
	{0.0f, 1.0f, 1.0f}	//7	//Left,		Top,	Back
	//8 total
};
//Cube faces
//Outside is counter-clockwise
const std::vector<u32> front
{
	0, 1, 2,
	2, 3, 0
};
const std::vector<u32> back
{
	5, 4, 7,
	7, 6, 5
};
const std::vector<u32> top
{
	7, 3, 2,
	2, 6, 7
};
const std::vector<u32> bottom
{
	1, 0, 4,
	4, 5, 1
};
const std::vector<u32> left
{
	0, 3, 7,
	7, 4, 0
};
const std::vector<u32> right
{
	2, 1, 5,
	5, 6, 2
};

// Some constants
constexpr pos_xyz UP(0, 1, 0);
constexpr pos_xyz DOWN(0, -1, 0);
constexpr pos_xyz LEFT(-1, 0, 0);
constexpr pos_xyz RIGHT(1, 0, 0);
constexpr pos_xyz FRONT(0, 0, -1);
constexpr pos_xyz BACK(0, 0, 1);
constexpr pos_xyz NONE(0, 0, 0);

void generateMeshFor(const World& world, Chunk& chunk)
{
	if(chunk.needsMesh())
	{
		//Locks this chunk mesh building to this caller
		chunk.setRemeshingFlag();
		/*if(chunk.isEmpty())
		{
			return;
		}*/
	}
	else
	{
		return;
	}

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

	bs::Mesh chunkMesh;

	for(int z = 0; z < CHUNK_SIZE; ++z)
	{
		for(int y = 0; y < CHUNK_SIZE; ++y)
		{
			for(int x = 0; x < CHUNK_SIZE; ++x)
			{
				const pos_xyz coords(x, y, z);
				const auto block = chunk.getBlockAt(coords);

				//Check if transparent
				//If the current block is transparent, skip it
				//if(tempisTransparent(block))
				{
					//continue;
				}

				//Check if each face should be rendered
				//Get each adjacent block
				const auto blockUP =	getBlockAt(coords + UP, chunk, world);
				const auto blockDOWN =	getBlockAt(coords + DOWN, chunk, world);
				const auto blockLEFT =	getBlockAt(coords + LEFT, chunk, world);
				const auto blockRIGHT =	getBlockAt(coords + RIGHT, chunk, world);
				const auto blockFRONT =	getBlockAt(coords + FRONT, chunk, world);
				const auto blockBACK =	getBlockAt(coords + BACK, chunk, world);

				//Check the blocks to the registry with the registry
				//BlockDataRegistry
				//Using this temp function for now to do it
				//If the adjacent block is transparent, then add the face to the mesh
				if(tempisTransparent(blockUP))
				{
					//Add upper face
					makeFace(chunkMesh, coords, UP, top);
				}
				if(tempisTransparent(blockDOWN))
				{
					//Add lower face
					makeFace(chunkMesh, coords, DOWN, bottom);
				}
				if(tempisTransparent(blockFRONT))
				{
					//Add front face
					makeFace(chunkMesh, coords, FRONT, front);
				}
				if(tempisTransparent(blockBACK))
				{
					//Add back face
					makeFace(chunkMesh, coords, BACK, back);
				}
				if(tempisTransparent(blockLEFT))
				{
					//Add left face
					makeFace(chunkMesh, coords, LEFT, left);
				}
				if(tempisTransparent(blockRIGHT))
				{
					//Add right face
					makeFace(chunkMesh, coords, RIGHT, right);
				}
			}	//x
		}	//y
	}	//z

	//Now for the mesh to go to the owning chunk
	chunk.setMesh(std::move(chunkMesh));
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

static void makeFace(bs::Mesh& chunkmesh, const pos_xyz& block_pos, const pos_xyz& direction, const std::vector<u32>& baked_face)
{
	constexpr bs::Vertex basicVert = 
	{
		.position = { 0.0f, 0.0f, 0.0f },
		.normal = { 0.0f, 0.0f, 0.0f},
		.uv = { 0.0f, 0.0f }
	};

	const bs::vec3 offset(block_pos.x, block_pos.y, block_pos.z);

	//Verts are in counter clockwise orientation
	auto v1 = basicVert;
	v1.normal = direction;
	auto v2 = v1;
	v2.uv = { 1.0f, 0.0f };
	auto v3 = v1;
	v3.uv = { 1.0f, 1.0f };
	auto v4 = v1;
	v4.uv = { 0.0f, 1.0f };

	//Each pre-made index array magic nums: 0, 1, 2, 4
	v1.position = vertices[baked_face[0]] + offset;	//Bottom Left
	v2.position = vertices[baked_face[1]] + offset;	//Bottom Right
	v3.position = vertices[baked_face[2]] + offset;	//Top Right
	v4.position = vertices[baked_face[4]] + offset;	//Top Left
	//Indexing
	const u32 currentIndex = chunkmesh.vertices.size();
	chunkmesh.vertices.emplace_back(v1);
	chunkmesh.vertices.emplace_back(v2);
	chunkmesh.vertices.emplace_back(v3);
	chunkmesh.vertices.emplace_back(v4);

	for(const auto& offsetBase : baked_face)
	{
		chunkmesh.indicies.emplace_back(offsetBase + currentIndex);
	}
}