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
	//Indices count
	constexpr auto worstCaseIndicesPerChunk = NUM_FACES_IN_FULL_CHUNK * 6; //147456 B

	const u32 NUM_CHUNKS = renderDistance * renderDistance * 4 * 16; // (2r)^2 * 16 (for height)

	//Placing the initial full span for open slots
	m_open_spans.emplace_back(0, NUM_CHUNKS * worstCaseIndicesPerChunk * sizeof(u32));
	
	//Get device pointer
	bs::Device* p_device = bs::asset_manager->getTextureMutable(0).getDevice();

	//Creating the buffers
	constexpr auto indexType = bs::vk::BufferUsage::INDEX_BUFFER;
	constexpr auto instancedType = bs::vk::BufferUsage::VERTEX_BUFFER;
	constexpr auto storageType = bs::vk::BufferUsage::STORAGE_BUFFER;
	bs::vk::BufferDescription basicDescription
	{
		.dev = p_device,
		.bufferType = indexType,
		.size = NUM_CHUNKS * worstCaseIndicesPerChunk * sizeof(u32),
		.stride = sizeof(u32),
	};

	//Chunk Index Buffer
	bs::asset_manager->addBuffer(std::make_shared<bs::vk::Buffer>(basicDescription), chunk_buffer_name);

	basicDescription.bufferType = instancedType;
	basicDescription.stride = sizeof(ChunkInstanceData);
	basicDescription.size = NUM_CHUNKS * sizeof(ChunkInstanceData);
	//Chunks Instance Buffer
	bs::asset_manager->addBuffer(std::make_shared<bs::vk::Buffer>(basicDescription), instance_buffer_name);

	basicDescription.bufferType = storageType;
	basicDescription.size = NUM_CHUNKS * NUM_FACES_IN_FULL_CHUNK * sizeof(u16); // 48 KB per chunk
	basicDescription.stride = sizeof(u16);
	//Chunks Texture Indexing Storage Buffer
	bs::asset_manager->addBuffer(std::make_shared<bs::vk::Buffer>(basicDescription), texture_storage_buffer_name);
}

ChunkMeshManager::~ChunkMeshManager()
{

}

void ChunkMeshManager::setRenderDistance(const u32 renderDistance)
{
	m_renderDistance = renderDistance;
}

bool ChunkMeshManager::cacheChunk(const Chunk& chunk)
{
	if(chunk.hasMesh())
	{
		return false;
	}

	//createDrawInfoFromChunk is in ChunkMesher.h/cpp
	auto drawInfo = std::make_shared<ChunkDrawInfo>(createDrawInfoFromChunk(chunk));
	std::cout << "Created draw info\n";

	//For the data length in the indices array
	const u32 length = drawInfo->numIndices * sizeof(u32);
	const auto baseOffset = findOpenSlot(length);
	drawInfo->startOffset = static_cast<u32>(baseOffset);

	if(baseOffset < 0)
	{
		//COULD NOT FIND SPACE FOR CHUNK!!!
		std::cout << "Could not find space in index array.\n";
		return false;
	}

	if(!reserveSlot(baseOffset, length))
	{
		//COULD NOT ALLOCATE!!!
		std::cout << "Could not reserve slot in index array.\n";
		return false;
	}
	
	drawInfo->instanceID = m_activeChunks.size();
	m_activeChunks.emplace_back(chunk.getChunkPos());
	m_chunk_draw_data.emplace_back(drawInfo);

	//Add chunk into the buffers
	addChunkToBuffer(chunk);
	
	return true;
}

void ChunkMeshManager::canDrop(const pos_xyz chunkPosition)
{
	m_droppableChunks.emplace_back(chunkPosition);
}

void ChunkMeshManager::canDrop(const Chunk& chunk)
{
	m_droppableChunks.emplace_back(chunk.getChunkPos());
}

bool ChunkMeshManager::isChunkCached(const pos_xyz chunkPosition) const
{
	for(const auto& chunk : m_droppableChunks)
	{
		if(chunk == chunkPosition)
		{
			return false;
		}
	}

	for(const auto& chunk : m_activeChunks)
	{
		if(chunk == chunkPosition)
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
	return m_activeChunks.size();
}

const std::vector<Chunk::ChunkMesh>& ChunkMeshManager::getChunkDrawData() const
{
	return m_chunk_draw_data;
}

void ChunkMeshManager::addChunkToBuffer(const Chunk& chunk)
{
	int indexOfChunk = -1;
	for(auto i = 0; i < m_activeChunks.size(); i+=1)
	{
		if(m_activeChunks[i] == chunk.getChunkPos())
		{
			indexOfChunk = i;
			break;
		}
	}
	assert(indexOfChunk >= 0);
	//Get chunk draw info
	const auto drawInfo = m_chunk_draw_data[indexOfChunk];

	//Build the index mesh
	const IndexMesh indexMesh = buildIndexMesh(*drawInfo);

	//Add to index buffer
	auto index_buffer = bs::asset_manager->getBuffer(chunk_buffer_name);
	index_buffer->writeBuffer(indexMesh.meshindicies.data(), drawInfo->numIndices * sizeof(u32), drawInfo->startOffset);

	//Add to instance buffer
	auto instance_buffer = bs::asset_manager->getBuffer(instance_buffer_name);
	const ChunkInstanceData instanceData 
	{
		.position = chunk.getChunkPos(),
		.textureSlotOffset = indexOfChunk * NUM_FACES_IN_FULL_CHUNK * sizeof(u16),
	};
	instance_buffer->writeBuffer(&instanceData, sizeof(ChunkInstanceData), instanceData.textureSlotOffset);

	//Add to face texture index buffer
	auto face_texture_buffer = bs::asset_manager->getBuffer(texture_storage_buffer_name);
	for(const auto& face : drawInfo->faces)
	{
		const u32 offset = instanceData.textureSlotOffset + face.faceIndex;
		//This is technically safe to thread except for the mapping bc it is to different parts of the array
		//This is also very sad and inefficient because only copying 2 bytes at a time
		face_texture_buffer->writeBuffer(&face.textureID, sizeof(u16), offset);
	}
	
	//Add the draw info to the chunk
	{	
		//Mutate the passed chunk
		const_cast<Chunk*>(&chunk)->setMesh(drawInfo);
	}

	//UPDATE THE DESCRIPTOR SET FOR THE STORAGE BUFFER!!!
	const VkDescriptorBufferInfo chunkTextureInfo
	{
		.buffer = face_texture_buffer->getAPIResource(),
		.offset = instanceData.textureSlotOffset,
		.range = NUM_FACES_IN_FULL_CHUNK * sizeof(u16),
	};

	VkWriteDescriptorSet writeTextureIndexBuffer = {};
	writeTextureIndexBuffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeTextureIndexBuffer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeTextureIndexBuffer.dstSet = *bs::asset_manager->pDescsetglobal;
	writeTextureIndexBuffer.dstBinding = 2;
	writeTextureIndexBuffer.dstArrayElement = 0; //Starting array element
	writeTextureIndexBuffer.descriptorCount = 1; //Number to write over
	writeTextureIndexBuffer.pBufferInfo = &chunkTextureInfo;

	const bs::Device* p_device = bs::asset_manager->getTextureMutable(0).getDevice();
	vkUpdateDescriptorSets(p_device->getDevice(), 1, &writeTextureIndexBuffer, 0, nullptr);
}

void ChunkMeshManager::condenseBuffer()
{
	//Shift the contents of the buffer around to maximize space
	//Also FORCE drop all of the droppable chunks
	//And reallocate the buffer if needed too
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
	mesh.meshindicies.reserve(drawInfo.numIndices);

	//32 per thread, so if 1.5 of a thread, then parallelize
	constexpr u32 facesWorkPerThread = 32;
	const bool shouldThreadIndexBuilding = (drawInfo.faces.size() >= (facesWorkPerThread * 1.5f));

	if(shouldThreadIndexBuilding)
	{
		mesh.meshindicies.resize(drawInfo.numIndices);
		const auto curJobs = jobSystem.backgroundJobs();
		const u32 numWorkers = drawInfo.faces.size() / facesWorkPerThread;

		for(u32 executionID = 0; executionID < numWorkers; executionID += 1)
		{
			const Job indexBuilder = jobSystem.createJob([&, executionID](Job j)
			{
				const u32 start = executionID * facesWorkPerThread;//Starting Face
				const u32 end = start + facesWorkPerThread;		//Ending Face

				for(auto i = start; i < end; i+=1)
				{
					const u16 faceID = drawInfo.faces[i].faceIndex;
					const u32 meshIndex = i * 6;

					const auto indexArray = getIndicesFromFaceIndex(faceID);
					memcpy(&(mesh.meshindicies[meshIndex]), indexArray.data(), indexArray.size() * sizeof(indexArray[0]));
				}
			});
			jobSystem.schedule(indexBuilder);
		}

		const u32 IndexRemaining = drawInfo.faces.size() - (drawInfo.faces.size() % facesWorkPerThread);
		for(auto faceNum = IndexRemaining; faceNum < drawInfo.faces.size(); faceNum += 1)
		{
			const auto faceID = drawInfo.faces[faceNum].faceIndex;
			const auto meshIndex = faceNum * 6;

			const std::array<u32, 6> indexArray = getIndicesFromFaceIndex(faceID);
			memcpy(&(mesh.meshindicies[meshIndex]), indexArray.data(), indexArray.size() * sizeof(indexArray[0]));
		}

		jobSystem.wait(curJobs - numWorkers);
	}
	else
	{
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
	for(const auto& freeSpace : m_open_spans)
	{
		const auto space = freeSpace.length - freeSpace.start;
		if(space >= data_length)
		{
			return freeSpace.start;
		}
	}
	
	return -1;
}

bool ChunkMeshManager::reserveSlot(const u32 start, const u32 data_length)
{
	const u32 end = start + data_length;

	for(auto& freeSpace : m_open_spans)
	{
		const u32 spaceStart = freeSpace.start;
		const u32 spaceEnd = spaceStart + freeSpace.length;
		if(spaceStart <= start && spaceEnd >= end)
		{
			//Boom
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