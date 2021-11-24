#include "ChunkMesher.h"
#include "MeshingData.h"


//Helper functions
constexpr static inline u16 toIndex(u16 x, u16 y, u16 z) noexcept;
constexpr static inline u32 toIndex(pos_xyz block_pos) noexcept
{
	return block_pos.x + (block_pos.y * CHUNK_SIZE) + (block_pos.z * CHUNK_AREA);
}
constexpr static inline pos_xyz fromIndex(u32 index) noexcept;
static block_t getBlockAt(const pos_xyz& rel_to_chunk, const Chunk& chunk, const World& world);
static pos_xyz OOBChunkOffset(pos_xyz rel_to_chunk);
static void makeFace(bs::Mesh& chunkmesh, const pos_xyz& block_pos, const pos_xyz& direction, const std::array<u32, 6>& baked_face);
static inline bool tempisTransparent(block_t b)
{
	return (b == 0);
}

//IMPLEMENTATIONS FOR THE HEADER

bs::Mesh generateMeshFor(const World& world, Chunk& chunk)
{
	if(chunk.needsMesh())
	{
		//Locks this chunk mesh building to this caller
		chunk.setRemeshingFlag();
		if(chunk.isEmpty())
		{
			return bs::Mesh();
		}
	}
	else
	{
		return bs::Mesh();
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

	for(auto z = 0; z < CHUNK_SIZE; ++z)
	{
		for(auto y = 0; y < CHUNK_SIZE; ++y)
		{
			for(auto x = 0; x < CHUNK_SIZE; ++x)
			{
				const pos_xyz coords(x, y, z);
				const auto block = chunk.getBlockAt(coords);

				//Check if transparent
				//If the current block is transparent, skip it
				if(tempisTransparent(block))
				{
					continue;
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

	return chunkMesh;
}

//Meshing order should be like this:
/**
 * All front, back, top, bottom, left, then right
 * For the x, then y, then z
 * So:
 * 0-4096 is the front faces
 * 
**/
//Get the indices of the block face for block `blockCoord` for the `side`
template<pos_xyz blockCoord, dir Side>
constexpr std::array<u16, 6> getIndicesFor()
{
	constexpr u16 vertOffsetPerSides = 4 * (1 + toIndex({15, 15, 15}));	//4096 (num blocks) * 4 (numVerts on a side)
	u16 indexOffsetThing = 0;
	switch (Side)
	{
	case dir::FRONT:
		indexOffsetThing = 0 * vertOffsetPerSides;
		break;
	case dir::BACK:
		indexOffsetThing = 1 * vertOffsetPerSides;
		break;
	case dir::TOP:
		indexOffsetThing = 2 * vertOffsetPerSides;
		break;
	case dir::BOTTOM:
		indexOffsetThing = 3 * vertOffsetPerSides;
		break;
	case dir::LEFT:
		indexOffsetThing = 4 * vertOffsetPerSides;
		break;
	case dir::RIGHT:
		indexOffsetThing = 5 * vertOffsetPerSides;
		break;
	default:
		//THERE WAS AN ERROR
		//RETURN THE DEFAULT?
		break;
	}

	//Get the four vertex offsets for the face at `blockCoord` and direction `Side`
	//Pretty sure its=>: index(blockCoord) * 4
	constexpr u16 baseIndex = (toIndex(blockCoord) * 4) + indexOffsetThing;

	return std::array<u16, 6> (baseIndex + front[0], baseIndex + front[1], baseIndex + front[2],
						baseIndex + front[3], baseIndex + front[4], baseIndex + front[5]);
}

const std::vector<bs::vec4>& createFullChunkMesh()
{
	static std::vector<bs::vec4> meshVerts;
	if(!meshVerts.empty())
	{
		return meshVerts;	//Return it if already generated
	}

	for(auto side = 0; side < 6; side+=1)
	{
		std::array<u32, 6> currentFace;
		float direction = 0.0f; //F1, B2, T3, B4, L5, R6
		switch (side)
		{
		case 0:
			currentFace = front;
			direction = 10.0f;
			break;
		case 1:
			currentFace = back;
			direction = 20.0f;
			break;
		case 2:
			currentFace = top;
			direction = 30.0f;
			break;
		case 3:
			currentFace = bottom;
			direction = 40.0f;
			break;
		case 4:
			currentFace = left;
			direction = 50.0f;
			break;
		case 5:
			currentFace = right;
			direction = 60.0f;
			break;
		
		default:
			throw "SIDE CALCULATION BROKE!!!";
			break;
		}
		//Iterate through each block
		for(auto z = 0; z < CHUNK_SIZE; z+=1)
		{
			for(auto y = 0; y < CHUNK_SIZE; y+=1)
			{
				for(auto x = 0; x < CHUNK_SIZE; x+=1)
				{
					const bs::vec3 coords(x, y, z);

					//Order for vertices is starting bottom left, then counter clockwise
					const auto& v1 = vertices[currentFace[BOTTOM_LEFT]];
					const auto& v2 = vertices[currentFace[BOTTOM_RIGHT]];
					const auto& v3 = vertices[currentFace[TOP_RIGHT]];
					const auto& v4 = vertices[currentFace[TOP_LEFT]];

					const float component4 = toIndex(x, y, z) * 100.0f;

					//Add the verticies to the vector
					meshVerts.emplace_back(bs::vec4(v1 + coords, component4 + direction + 1));
					meshVerts.emplace_back(bs::vec4(v2 + coords, component4 + direction + 2));
					meshVerts.emplace_back(bs::vec4(v3 + coords, component4 + direction + 3));
					meshVerts.emplace_back(bs::vec4(v4 + coords, component4 + direction + 4));
				}//x
			}//y
		}//z
	}//Faces

	return meshVerts;
}

std::vector<ChunkDrawInfo::Face> generateFacesForChunk(const World& world, const Chunk& chunk)
{
	//@TODO: Make a more parallelized version with the job system
	//That can be optionally enabled

	std::vector<ChunkDrawInfo::Face> faces;

	for(auto side = 0; side < NUM_SIDES; side += 1)
	{
		//Doing the sides
		const u16 offset = NUM_FACES_PER_SIDE * side;
		auto direction = FRONT;

		switch (side)
		{
		case 1:
			direction = BACK;
			break;
		case 2:
			direction = UP;
			break;
		case 3:
			direction = DOWN;
			break;
		case 4:
			direction = LEFT;
			break;
		case 5:
			direction = RIGHT;
			break;
		default:
			break;
		}

		for(u16 z = 0; z < CHUNK_SIZE; z+=1)
		{
			for(u16 y = 0; y < CHUNK_SIZE; y+=1)
			{
				for(u16 x = 0; x < CHUNK_SIZE; x+=1)
				{
					const pos_xyz coords(x, y, z);
					const auto block = chunk.getBlockAt(coords);

					//Check if transparent
					//If the current block is transparent, skip it
					if(tempisTransparent(block))
					{
						continue;
					}

					//Check if the adj block is transparent
					const auto adjBlock = getBlockAt(coords + direction, chunk, world);

					//Check the blocks to the registry with the registry
					//BlockDataRegistry
					//Using this temp function for now to do it
					//If the adjacent block is transparent, then add the face to the mesh
					if(tempisTransparent(adjBlock))
					{
						u16 FaceIndex = toIndex(x, y, z) + offset;
						faces.emplace_back(ChunkDrawInfo::Face
						{
							.textureID = (u16)0,
							.faceIndex = FaceIndex,
						});
					}
				}//x
			}//y
		}//z
	}//Sides

	return faces;
}

constexpr static inline u16 toIndex(u16 x, u16 y, u16 z) noexcept
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
		return chunk.getBlockAt(rel_to_chunk);
	}
	else
	{
		return 0;
	}
	//World coords
	return world.getBlockAt((chunk.getChunkPos() * CHUNK_SIZE) + rel_to_chunk);
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

static void makeFace(bs::Mesh& chunkmesh, const pos_xyz& block_pos, const pos_xyz& direction, const std::array<u32, 6>& baked_face)
{
	constexpr bs::Vertex basicVert = 
	{
		.position = { 0.0f, 0.0f, 0.0f },
		.normal = { 0.0f, 0.0f, 0.0f },
		.uv = { 0.0f, 0.0f }
	};

	const bs::vec3 offset(block_pos.x, block_pos.y, block_pos.z);

	//Verts are in counter clockwise orientation
	auto v1 = basicVert;
	v1.normal = { 0.0f, 0.0f, 0.0f };//direction;
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

	//Generic Quad Indices:
	const static std::vector<u32> quad_indices
	{
		0, 1, 2,
		2, 3, 0
	};

	for(const auto& offsetBase : quad_indices)
	{
		chunkmesh.indicies.emplace_back(offsetBase + currentIndex);
	}
}