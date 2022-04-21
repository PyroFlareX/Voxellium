#include "ChunkMeshManager.h"

#include "MeshingData.h"
#include <Engine.h>

#include "../World.h"

//Names of buffers for asset manager
const std::string index_buffer_name("chunk_indices");
const std::string instance_buffer_name("chunk_instance_data");
const std::string texture_storage_buffer_name("chunk_texture_data");

ChunkMeshManager::ChunkMeshManager(const World& world, u32 renderDistance)	:	m_world(world), m_renderDistance(renderDistance),
		m_chunkInfo({}), m_droppableChunks({}), m_chunkBufferingCounter(0)
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

	///----------------------------------------///

	//Creating the buffers
	bs::vk::BufferDescription basicDescription
	{
		.dev = bs::asset_manager->getTextureMutable(0).getDevice(),
		.bufferType = bs::vk::BufferUsage::INDEX_BUFFER,
		.size = num_indices * sizeof(indexType),
		.stride = sizeof(indexType),
	};

	//Chunk Index Buffer
	// bs::asset_manager->addBuffer(std::make_shared<bs::vk::Buffer>(basicDescription), index_buffer_name);

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


void ChunkMeshManager::setRenderDistance(u32 renderDistance)
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

	//Store the chunk info and get the instance for the data; locked because changing vector
	std::unique_lock<std::shared_mutex> g_drawdata(m_cache_lock);
	
	drawInfo->instanceID = m_chunkInfo.size(); //Last part filled, chunk can be added to buffer now
	//Chunk info vector
	m_chunkInfo.push_back(drawInfo);

	g_drawdata.unlock();

	//Add chunk into the buffers
	//const Job bufferChunkData([drawInfo](Job j) { addChunkToBuffer(drawInfo); }, m_chunkBufferingCounter);

	//Buffer chunk data job
	bs::getJobSystem().schedule(Job([drawInfo](Job j) {
		addChunkToBuffer(drawInfo);
	}, m_chunkBufferingCounter), m_chunkBufferingCounter);

	return true;
}

void ChunkMeshManager::canDrop(pos_xyz chunkPosition)
{
	std::unique_lock<std::shared_mutex> g_drop(m_drop_lock);
	m_droppableChunks.emplace_back(std::move(chunkPosition));
}

void ChunkMeshManager::canDrop(const Chunk& chunk)
{
	canDrop(chunk.getChunkPos());
}

bool ChunkMeshManager::isChunkCached(pos_xyz chunkPosition) const
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
	IndexMesh&& indexMesh = buildIndexMesh(*chunk);

	chunk->gpu_buffer = std::make_shared<bs::vk::Buffer>(
		bs::asset_manager->getTextureMutable(0).getDevice(),
		bs::vk::BufferUsage::INDEX_BUFFER, 
		indexMesh.size() * sizeof(indexType), indexMesh.data());
	// gpu_buffer


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

size_t ChunkMeshManager::numBytesOfCachedChunks() const
{
	size_t bytes(0);
	for(const auto& chunks : m_chunkInfo)
	{
		bytes += chunks->numIndices;
	}
	return bytes * sizeof(indexType);
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

ChunkMeshManager::IndexMesh ChunkMeshManager::buildIndexMesh(const ChunkDrawInfo& drawInfo)
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

bool ChunkMeshManager::isMarkedDroppable(pos_xyz chunk_pos) const
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

