#include "ChunkMeshManager.h"

#include "MeshingData.h"
#include <Engine.h>

#include "../World.h"

//Names of buffers for asset manager
const std::string index_buffer_name("chunk_indices");
const std::string instance_buffer_name("chunk_instance_data");
const std::string texture_storage_buffer_name("chunk_texture_data");

ChunkMeshManager::ChunkMeshManager(const World& world, const u32 renderDistance)	:	m_world(world), m_renderDistance(renderDistance),
		m_open_spans({}), m_chunkInfo({}), m_droppableChunks({}), m_failedAllocationsCounter(0), m_chunkBufferingCounter(0)
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
	constexpr auto INDICES_PER_CHUNK = 6 * CHUNK_AREA * 2;
	// @TODO: Write an actual algorithm for estimating this

	//This is a cylinder, 16 chunks tall, and a base with radius of m_renderDistance
	//Maybe consider other geometries?
	const u32 chunkCacheSize = (glm::pi<double>() * m_renderDistance * m_renderDistance + 0.5) * CHUNK_SIZE;
	const u32 num_indices = INDICES_PER_CHUNK * chunkCacheSize;

	//Placing the initial full span for open slots
	//m_open_spans.emplace_back(0, num_indices * sizeof(u32));	// [0 - byteLen)

	//Creating the buffers
	bs::vk::BufferDescription basicDescription
	{
		.dev = bs::asset_manager->getTextureMutable(0).getDevice(),
		.bufferType = bs::vk::BufferUsage::INDEX_BUFFER,
		.size = num_indices * sizeof(indexType),
		.stride = sizeof(indexType),
	};

	//Chunk Index Buffer
	bs::asset_manager->addBuffer(std::make_shared<bs::vk::Buffer>(basicDescription), index_buffer_name);

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

	//This mutates state if successful
	const auto slot = reserveSlot(drawInfo->numIndices);

	//Checks if the allocation failed
	if(slot.empty())
	{
		m_failedAllocationsCounter++;
		return false;
	}

	//After this, safe syncronization must happen, because the state is mutated

	//Get the byte offset for the indices
	drawInfo->startOffset = (slot.data() - m_parent_span.data());
	
	//Store the chunk info and get the instance for the data; locked because changing vector
	std::unique_lock<std::shared_mutex> g_drawdata(m_cache_lock);
	
	drawInfo->instanceID = m_chunkInfo.size(); //Last part filled, chunk can be added to buffer now
	m_chunkInfo.push_back(drawInfo);

	g_drawdata.unlock();

	//Add chunk into the buffers
	const Job bufferChunkData([drawInfo](Job j) { addChunkToBuffer(drawInfo); }, m_chunkBufferingCounter);
	bs::getJobSystem().schedule(bufferChunkData, m_chunkBufferingCounter);

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
	//Return false is the chunk is not cached
	if(isMarkedDroppable(chunkPosition))
	{
		return false;
	}

	//For the active chunk section only, checks list

	// @TODO: evaluate the performance impact of the mutexes
	std::shared_lock<std::shared_mutex> g_cache(m_cache_lock);
	for(const auto& chunk : m_chunkInfo)
	{
		if(chunk->chunk_pos == chunkPosition)
		{
			return true;
		}
	}

	return false;
}

bool ChunkMeshManager::isChunkCached(const Chunk& chunk) const
{
	return isChunkCached(chunk.getChunkPos());
}

u32 ChunkMeshManager::getNumChunks() const
{
	return m_chunkInfo.size();
}

const std::vector<Chunk::ChunkMesh>& ChunkMeshManager::getChunkDrawData() const
{
	// std::shared_lock<std::shared_mutex> g_slot(m_cache_lock);
	
	bs::getJobSystem().waitWithCounter(0, m_chunkBufferingCounter);
	return m_chunkInfo;
}

void ChunkMeshManager::addChunkToBuffer(const Chunk::ChunkMesh chunk)
{
	//Build the index mesh, and add it to the index buffer
	//Can be put as a job?
	const IndexMesh indexMesh = buildIndexMesh(*chunk);

	auto index_buffer = bs::asset_manager->getBuffer(index_buffer_name);
	index_buffer->writeBuffer(indexMesh.data(), chunk->numIndices * sizeof(indexType), chunk->startOffset);

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

bool ChunkMeshManager::shouldCondense() const
{
	bool condense = false;
	//If more than 4 failed allocations
	if(m_failedAllocationsCounter.load() > 4)
	{
		condense = true;
	}

	//If there is sufficient droppable data
	u32 freedIndices = 0;
	for(const auto chunks : m_chunkInfo)
	{
		if(isMarkedDroppable(chunks->chunk_pos))
		{
			freedIndices += chunks->numIndices;
		}
	}
	constexpr u32 MIN_DROPPABLE_INDICIES = 100;	//Arbitrary
	if(freedIndices >= MIN_DROPPABLE_INDICIES)
	{
		condense = true;
	}
	return condense;
}

bool ChunkMeshManager::shouldReallocate() const
{
	//6 indicies per quad	|	num bytes per index	|	16 Faces (arbitrary)
	constexpr auto REALLOCATION_DIFFERENCE = 6 * sizeof(indexType) * 16;

	//One-sixth more | 16.66% more | should avoid using tbh at large sizes, only on the low ends
	constexpr auto REALLOCATION_MULTIPLIER = 1 + (1.0f / 6.0f);

	//If more than 4 failed allocations
	if(m_failedAllocationsCounter.load() > 4)
	{	//Check other conditions for whether it is necessary for reallocation
		const auto max_size = m_parent_span.size_bytes();
		const auto currentUsage = numBytesOfCachedChunks();

		const bool sum_condition = (currentUsage + REALLOCATION_DIFFERENCE >= max_size);
		const bool percent_condition = (currentUsage * REALLOCATION_MULTIPLIER >= max_size);

		//If either condition is met, then reallocate
		return sum_condition || percent_condition;
	}
	else
	{	//Otherwise, no need to extra work for checks, since nothing is failing
		return false;
	}
}

size_t ChunkMeshManager::numBytesOfCachedChunks() const
{
	size_t bytes = 0;
	for(const auto& chunks : m_chunkInfo)
	{
		bytes += chunks->numIndices * sizeof(indexType);
	}
	return bytes;
}

void ChunkMeshManager::condenseBuffer()
{
	//Shift the contents of the buffer around to maximize space 
	//	because the vulkan buffer is only guarunteed to be written, and might not be synced to read from, must regenerate indices
	//Drops all of the droppable chunks

	std::vector<Chunk::ChunkMesh> savedDraws;
	savedDraws.reserve(getNumChunks());

	auto& jsys = bs::getJobSystem();

	//Save only the non dropped chunks
	std::shared_lock<std::shared_mutex> g_cache_read(m_cache_lock);
	for(const auto& drawInfo : m_chunkInfo)
	{
		if(!isMarkedDroppable(drawInfo->chunk_pos))
		{
			savedDraws.push_back(drawInfo);
		}
	}
	g_cache_read.unlock();

	//Clear slots so they can be redone
	std::unique_lock<std::shared_mutex> g_slots(m_slot_lock);
	m_open_spans.clear();
	m_open_spans.emplace_back(m_parent_span);
	//First span
	auto& span = m_open_spans[0];

	std::unique_lock<std::shared_mutex> g_cache(m_cache_lock);
	for(auto i = 0; i < savedDraws.size(); i += 1)
	{
		auto drawInfo = savedDraws[i];
		drawInfo->instanceID = i;

		//Because it was cleared before, it is GUARUNTEEABLE that ALL the
		//	existing reservations can be redone without needing a success check

		//Because the `reserveSlot` function locks the mutex that is already locked, have to do this manually
		//Leave the remainder of the span
		span = span.last(span.size() - drawInfo->numIndices);
		drawInfo->startOffset = (span.data() - m_parent_span.data());

		//Jobify the adding to buffer
		const Job addToBuffer([drawInfo](Job j)
		{
			addChunkToBuffer(drawInfo);	
		}, m_chunkBufferingCounter);
		jsys.schedule(addToBuffer, m_chunkBufferingCounter);
		
	}
	//Swap them after updating the infos, so the dropped list can be cleared whenever
	m_chunkInfo.swap(savedDraws);

	g_cache.unlock();
	g_slots.unlock();

	//Async this whenever (after the swap):
	{
		m_failedAllocationsCounter.store(0);
		std::unique_lock<std::shared_mutex> g_drop_slot(m_drop_lock);
		m_droppableChunks.clear();
	}
}

void ChunkMeshManager::reallocateBuffers()
{
	// @TODO: actually implement lol
	throw std::runtime_error("Not yet implemented!\n");

	///Lock EVERYTHING [Access to buffers, access to slots]
	std::unique_lock<std::shared_mutex> g_cache(m_cache_lock, std::defer_lock);
	std::unique_lock<std::shared_mutex> g_slots(m_slot_lock, std::defer_lock);

	std::scoped_lock g_lock(g_cache, g_slots);

	///Set new space requirements for buffers
	//Allocate them
	auto index_buffer = bs::asset_manager->getBuffer(index_buffer_name);
	const auto oldSize = index_buffer->getSize();


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

	//These are initialized, but ARE NOT VALID TO USE!!!!
	c_info.startOffset = 0;
	c_info.instanceID = 0;

	return c_info;
}

const ChunkMeshManager::IndexMesh ChunkMeshManager::buildIndexMesh(const ChunkDrawInfo& drawInfo)
{
	IndexMesh mesh;
	mesh.resize(drawInfo.faces.size() * 6); // OR: mesh.resize(drawInfo->numIndices);

	constexpr u32 facesWorkPerJob = 32;

	Counter meshingCounter{0};
	auto& jobSystem = bs::getJobSystem();

	const auto numWorkers = drawInfo.faces.size() / facesWorkPerJob;
	for(auto executionID = 0; executionID < numWorkers; executionID += 1)
	{
		const Job index_build_job([&drawInfo, &mesh, executionID](Job j)
		{
			const u32 start = executionID * facesWorkPerJob;	//Starting Face
			const u32 end = start + facesWorkPerJob;			//Ending Face

			for(auto i = start; i < end; i += 1)
			{
				const u16 faceID = drawInfo.faces[i].faceIndex;
				const u32 meshIndex = i * 6;
				const auto indexArray = getIndicesFromFaceIndex(faceID);
				for(auto j = 0; j < indexArray.size(); j += 1)
				{
					mesh[meshIndex + j] = indexArray[j];
				}
			}
		}, meshingCounter);

		jobSystem.schedule(index_build_job, meshingCounter);
	}

	const u32 indexRemaining = numWorkers * facesWorkPerJob;
	for(auto faceNum = indexRemaining; faceNum < drawInfo.faces.size(); faceNum += 1)
	{
		const auto& faceID = drawInfo.faces[faceNum].faceIndex;
		const auto meshIndex = faceNum * 6;

		const std::array<u32, 6> indexArray = getIndicesFromFaceIndex(faceID);
		for(auto j = 0; j < indexArray.size(); j += 1)
		{
			mesh[meshIndex + j] = indexArray[j];
		}
	}
	jobSystem.waitWithCounter(0, meshingCounter);

	return mesh;
}

bool ChunkMeshManager::isMarkedDroppable(const pos_xyz chunk_pos) const
{
	std::shared_lock<std::shared_mutex> g_drop_slot(m_drop_lock);
	for(const auto& chunk : m_droppableChunks)
	{
		if(chunk == chunk_pos)
		{
			return true;
		}
	}
	return false;
}

std::span<ChunkMeshManager::indexType> ChunkMeshManager::reserveSlot(u32 indicesCount)
{
	//@TODO: IMPORTANT!!!
	//		Try to find a way to do this lockless (atomics or otherwise, must be safe)
	std::unique_lock<std::shared_mutex> g_slot(m_slot_lock);
	//Find out if there is space
	for(auto& span : m_open_spans)
	{
		if(span.size() >= indicesCount)
		{
			const auto allocation = span.first(indicesCount);

			//Modify the span
			span = span.last(span.size() - indicesCount);
			return allocation;
		}
	}
	//If the span can't be achieved, return an empty span
	return m_parent_span.subspan(0, 0);
}
