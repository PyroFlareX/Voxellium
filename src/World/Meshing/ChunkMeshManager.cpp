#include "ChunkMeshManager.h"

#include "MeshingData.h"
#include <Engine.h>

#include "../World.h"

//Names of buffers for asset manager
const std::string chunk_buffer_name("chunk_indices");
const std::string instance_buffer_name("chunk_instance_data");
const std::string texture_storage_buffer_name("chunk_texture_data");

ChunkMeshManager::ChunkMeshManager(const World& world, const u32 renderDistance)	:	m_world(world), m_renderDistance(renderDistance),
		m_open_spans({}), m_chunk_draw_data({}), m_activeChunks({}), m_droppableChunks({})
{
	/**
	 * Stuff that should happen:
	 * 
	 * Calculate the number of chunk space to allocate from the render distance
	 * Initialize the chunk instance buffer & size
	 * Resize the face storage buffer for number of chunks
	 * 
	 * Initialize the span for the index buffer data allocator
	*/
	
	/// Calculating initial size of the buffer
	constexpr auto INDICES_PER_CHUNK = 6 * CHUNK_AREA * 2; //Literally arbitrary
	// @TODO: Write an actual algorithm for estimating this ^

	//This is a cylinder, 16 chunks tall, and a base with radius of m_renderDistance
	//Maybe consider other geometries?
	const u32 chunkCacheSize = (glm::pi<double>() * m_renderDistance * m_renderDistance + 0.5) * CHUNK_SIZE;
	const u32 num_indices = INDICES_PER_CHUNK * chunkCacheSize;

	//Placing the initial full span for open slots
	m_open_spans.emplace_back(0, num_indices * sizeof(u32));	// [0 - byteLen)

	//Creating the buffers
	bs::vk::BufferDescription basicDescription
	{
		.dev = bs::asset_manager->getTextureMutable(0).getDevice(),
		.bufferType = bs::vk::BufferUsage::INDEX_BUFFER,
		.size = num_indices * sizeof(u32),
		.stride = sizeof(u32),
	};

	//Chunk Index Buffer
	bs::asset_manager->addBuffer(std::make_shared<bs::vk::Buffer>(basicDescription), chunk_buffer_name);

	basicDescription.bufferType = bs::vk::BufferUsage::VERTEX_BUFFER;
	basicDescription.stride = sizeof(ChunkInstanceData);
	basicDescription.size = chunkCacheSize * sizeof(ChunkInstanceData);

	//Chunks Instance Buffer
	bs::asset_manager->addBuffer(std::make_shared<bs::vk::Buffer>(basicDescription), instance_buffer_name);

	//Resizing the chunk texture face buffer with the number of chunks
	auto texture_storage_buffer = bs::asset_manager->getBuffer(texture_storage_buffer_name);
	texture_storage_buffer->setAllocationSize(NUM_FACES_IN_FULL_CHUNK * chunkCacheSize * sizeof(u16));
	texture_storage_buffer->allocateBuffer();
}

/*ChunkMeshManager::~ChunkMeshManager()
{
	
}*/

void ChunkMeshManager::setRenderDistance(const u32 renderDistance)
{
	m_renderDistance = renderDistance;

	//@TODO make this reallocate and regenerate EVERYTHING
}

bool ChunkMeshManager::cacheChunk(const Chunk& chunk)
{
	//If chunk is already cached, and doesn't need to be updated, then it is already cached and up to date
	if(isChunkCached(chunk))
	{
		if(!chunk.needsMesh())
		{
			return false;
		}
	}

	//Independent Call, this is expensive however
	auto drawInfo = std::make_shared<ChunkDrawInfo>(createDrawInfoFromChunk(chunk));
	const u32 data_length = drawInfo->numIndices * sizeof(u32);

	//See if a slot can be found for it
	if(findOpenSlot(data_length) < 0)
	{
		//Do a check to see if this logically *should* happen
		// @TODO: Have the check, and reconfigure and condense buffers if necessary

		//The caching failed
		// std::cout << "Could not find space in index array.\n";
		return false;
	}

	//After this, safe syncronization must happen, because the state is mutated

	//Get the offset for the indices
	drawInfo->startOffset = reserveOpenSlot(data_length);
	
	//Stored buffer arrays/cache
	std::unique_lock<std::shared_mutex> g_array_lock(m_cache_lock);
	drawInfo->instanceID = m_activeChunks.size(); //last part filled, chunk can be added to buffer now

	m_activeChunks.emplace_back(chunk.getChunkPos());
	m_chunk_draw_data.emplace_back(drawInfo);

	//Add chunk into the buffers
	//This is can be called from multiple threads if the args don't have overlapping data
	//Can be called asyncronously
	addChunkToBuffer(drawInfo);

	return true;
}

void ChunkMeshManager::canDrop(const pos_xyz chunkPosition)
{
	std::unique_lock<std::shared_mutex> g_drop(m_drop_lock);
	m_droppableChunks.emplace_back(chunkPosition);
}

void ChunkMeshManager::canDrop(const Chunk& chunk)
{
	canDrop(chunk.getChunkPos());
}

bool ChunkMeshManager::isChunkCached(const pos_xyz chunkPosition) const
{
	// @TODO: evaluate the performance impact of these mutexes

	//For the droppable chunk section only
	// std::shared_lock<std::shared_mutex> g_slot(m_drop_lock);
	for(const auto& chunk : m_droppableChunks)
	{
		if(chunk == chunkPosition)
		{
			return false;
		}
	}

	//For the active chunk section only
	// std::shared_lock<std::shared_mutex> g_slot(m_cache_lock);
	for(const auto& chunk : m_activeChunks)
	{
		if(chunk == chunkPosition)
		{
			return true;
		}
	}

	//Chunk is NOT cached
	return false;
}

bool ChunkMeshManager::isChunkCached(const Chunk& chunk) const
{
	return isChunkCached(chunk.getChunkPos());
}

u32 ChunkMeshManager::getNumChunks() const
{
	//I don't think a shared lock is needed for this, since it's a 'safe' function according to the spec
	//std::shared_lock<std::shared_mutex> g_slot(m_cache_lock);
	return m_activeChunks.size();
}

const std::vector<Chunk::ChunkMesh>& ChunkMeshManager::getChunkDrawData() const
{
	// std::shared_lock<std::shared_mutex> g_slot(m_cache_lock);
	return m_chunk_draw_data;
}

void ChunkMeshManager::addChunkToBuffer(const Chunk::ChunkMesh chunk)
{
	//Build the index mesh
	const IndexMesh indexMesh = buildIndexMesh(*chunk);

	//Add to index buffer
	auto index_buffer = bs::asset_manager->getBuffer(chunk_buffer_name);
	index_buffer->writeBuffer(indexMesh.meshindicies.data(), chunk->numIndices * sizeof(u32), chunk->startOffset);

	//Add to instance buffer
	auto instance_buffer = bs::asset_manager->getBuffer(instance_buffer_name);
	const ChunkInstanceData instanceData 
	{
		.position = chunk->chunk_pos,
		.textureSlotOffset = chunk->instanceID * NUM_FACES_IN_FULL_CHUNK * sizeof(u16),
	};
	instance_buffer->writeBuffer(&instanceData, sizeof(ChunkInstanceData), instanceData.textureSlotOffset);

	//Add to face texture index buffer
	auto face_texture_buffer = bs::asset_manager->getBuffer(texture_storage_buffer_name);

	//ALSO:
	//	Maybe have the buffers persistently mapped?
	//		When reallocating, remap them and update the descriptor sets


	//@TODO: Fix the manager accessing to have the following capabilities:
	/**
	 * Caching must be able to be done from any arbitrary caller simultaneously, sync internally
	 * (To do this, possibly use state mutexes or something, like lock-unlock immedately
	 *  upon calling to cache a chunk, and actually lock when calling, say, the GC condenser)
	 * ^ This allows EXCLUSIVE access to the manager from just that thread, so other mutations don't screw it up
	 * 
	 * The GC will be called if there is a decent amount of micro allocations unused (probably lock to be only one caller at a time)
	 * 
	 * Possibly use an MPMC or MPSC queue to distribute local work for the chunk building/lifetime managing
	 * 
	 * Have the setRenderDistance reconfigure the sizes and setup of this so that it allocates enough room for all the chunks
	 * 	within that radius/distance [excluding index buffer]
	 * 
	 * Create a size allocator for the index buffer
	 * 
	 * Make sure the chunk DOES NOT get modified whatsoever by this.
	 * 	This means ABSOLUTELY NOTHING. Stop caching the ptr in the chunk, even though its convenient
	 * It just makes some other things harder
	 * 
	 * Have a job that checks the active chunks if any mesh updates need to occur, MUTATE THE CHUNK STATE
	 * 	(atomically maybe?) and assign the remeshing to another job
	 * 	[This should be done on another ChunkMeshInfo, and swap the pointers when it is done]
	 * 		> This allows for the chunk to continue being rendered to squeeze every last FPS
	 * 			that can be achieved before biting the time to copy to the buffer & swap
	 * 		Possible optimizations:
	 * 			1. Compare the previous face count to the new one, if equal or less, keep the existing region
	 * 			2. Do it asyncronously on another job and chunk info, swap after completion !!! <= REQUIREMENT
	 * 
	**/

	//Uncomment this when above is done
	/*for(const auto& face : drawInfo->faces)
	{
		const u32 offset = instanceData.textureSlotOffset + face.faceIndex;
		//This is technically safe to thread except for the mapping bc it is to different parts of the array
		//This is also very sad and inefficient because only copying 2 bytes at a time
		face_texture_buffer->writeBuffer(&face.textureID, sizeof(u16), offset);
	}*/
	
}

void ChunkMeshManager::condenseBuffer()
{
	//Shift the contents of the buffer around to maximize space
	//Also FORCE drop all of the droppable chunks
	//And reallocate the buffer if needed too

	//@TODO: Implement this
	throw std::runtime_error("Not yet implemented!\n");
}

void ChunkMeshManager::reallocateBuffers()
{
	//Lock EVERYTHING

	// @TODO: actually implement lol
	throw std::runtime_error("Not yet implemented!\n");

	//Set new space requirements for buffers
	//Allocate them (again ig)

	//Update the descriptor set for the storage buffer
	//	(I think) (Reasoning being the old VkBuffer was destroyed, the new one is a different handle/"pointer")
}

ChunkDrawInfo ChunkMeshManager::createDrawInfoFromChunk(const Chunk& chunk) const
{
	ChunkDrawInfo c_info;
	c_info.chunk_pos = chunk.getChunkPos();

	//THIS IS EXPENSIVE!!!
	c_info.faces = generateFacesForChunk(m_world, chunk);
	c_info.numIndices = c_info.faces.size() * 6;
	c_info.startOffset = 0;

	return c_info;
}

const ChunkMeshManager::IndexMesh ChunkMeshManager::buildIndexMesh(const ChunkDrawInfo& drawInfo) const
{
	IndexMesh mesh;
	mesh.meshindicies.resize(drawInfo.faces.size() * 6);

	constexpr auto WORKGROUP_SIZE = 32;

	//32 per thread, so if 1.5 of a thread, then parallelize
	constexpr u32 facesWorkPerThread = WORKGROUP_SIZE;
	const bool shouldThreadIndexBuilding = (drawInfo.faces.size() >= (facesWorkPerThread * 1.5f));

	if(shouldThreadIndexBuilding)
	{
		Counter meshingCounter{0};
		auto& jobSystem = bs::getJobSystem();

		const auto numWorkers = drawInfo.faces.size() / facesWorkPerThread;
		for(auto executionID = 0; executionID < numWorkers; executionID += 1)
		{
			Job index_build_job([&drawInfo, &mesh, executionID](Job j)
			{
				const u32 start = executionID * facesWorkPerThread;	//Starting Face
				const u32 end = start + facesWorkPerThread;			//Ending Face

				for(auto i = start; i < end; i += 1)
				{
					const u16 faceID = drawInfo.faces[i].faceIndex;
					const u32 meshIndex = i * 6;

					const auto indexArray = getIndicesFromFaceIndex(faceID);
					for(auto j = 0; j < indexArray.size(); j += 1)
					{
						mesh.meshindicies[meshIndex + j] = indexArray[j];
					}
				}
			}, meshingCounter);

			jobSystem.schedule(index_build_job, meshingCounter);
		}

		const u32 IndexRemaining = numWorkers * facesWorkPerThread; //drawInfo.faces.size() - (drawInfo.faces.size() % facesWorkPerThread);
		for(auto faceNum = IndexRemaining; faceNum < drawInfo.faces.size(); faceNum += 1)
		{
			const auto& faceID = drawInfo.faces[faceNum].faceIndex;
			const auto meshIndex = faceNum * 6;

			const std::array<u32, 6> indexArray = getIndicesFromFaceIndex(faceID);
			for(auto j = 0; j < indexArray.size(); j += 1)
			{
				mesh.meshindicies[meshIndex + j] = indexArray[j];
			}
		}
		jobSystem.waitWithCounter(0, meshingCounter);
	}
	else
	{
		mesh.meshindicies.clear();
		for(const auto& face : drawInfo.faces)	// [0, faces.size())
		{
			const auto indices = getIndicesFromFaceIndex(face.faceIndex);
			for(const auto& index : indices)	//[0, 6)
			{
				mesh.meshindicies.emplace_back(index);
			}
		}	//Overall does this: //[0, faces.size() * 6)
	}

	return mesh;
}

i64 ChunkMeshManager::findOpenSlot(const u32 data_length) const
{
	// std::shared_lock<std::shared_mutex> g_slot(m_slot_lock);
	
	for(const auto& freeSpace : m_open_spans)
	{
		if(freeSpace.length >= data_length)
		{
			return freeSpace.start;
		}
	}
	return -1;
}

bool ChunkMeshManager::reserveSlot(const u32 start, const u32 data_length)
{
	std::unique_lock<std::shared_mutex> g_slot(m_slot_lock);

	const u32 end = start + data_length;
	for(auto& freeSpace : m_open_spans)
	{
		const u32 spaceStart = freeSpace.start;
		const u32 spaceEnd = spaceStart + freeSpace.length;
		if(spaceStart <= start && spaceEnd >= end)
		{
			//Change the space in the buffer
			if(freeSpace.start == start)
			{
				//If the buffer begins are the same
				freeSpace.start = end;
				freeSpace.length = spaceEnd - end;
			}
			else
			{
				//start is > than spaceStart
				//The reservation is in the middle of this block, so it must be spliced into front and back
				const span front
				{
					.start = freeSpace.start,
					.length = start - freeSpace.start,	//length is the difference between alloc start to OG start
				};
				const span back
				{
					.start = end,
					.length = spaceEnd - end, //length is distance from far end to alloc end
				};

				freeSpace = front;
				m_open_spans.emplace_back(back);
			}
			return true;
		}
	}
	return false;
}

u32 ChunkMeshManager::reserveOpenSlot(const u32 data_length)
{
	std::unique_lock<std::shared_mutex> g_slot(m_slot_lock);

	for(auto& freeSpace : m_open_spans)
	{
		if(freeSpace.length >= data_length)
		{
			const auto start = freeSpace.start;
			freeSpace.start = start + data_length;
			freeSpace.length -= data_length;

			return start;
		}
	}
	
	throw std::runtime_error("Failed to reserve an open slot while supposedly guarunteed!!!");

	return -1;
}