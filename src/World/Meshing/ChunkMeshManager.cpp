#include "ChunkMeshManager.h"

#include "MeshingData.h"
#include <Engine.h>

#include "../World.h"

ChunkMeshManager::ChunkMeshManager(const World& world)	:	m_world(world)
{
	//Get device
	bs::Device* p_device = bs::asset_manager->getTextureMutable(0).getDevice();

	//Names of buffers for asset manager
	const std::string chunk_buffer_name("chunk_indices");
	const std::string instance_buffer_name("chunk_instance_data");

	//Creating the buffers
	constexpr auto indexType = bs::vk::BufferUsage::INDEX_BUFFER;
	constexpr auto instancedType = bs::vk::BufferUsage::VERTEX_BUFFER;

	constexpr auto worstCaseIndicesPerChunk = 147456;

	bs::vk::BufferDescription basicDescription
	{
		.dev = p_device,
		.bufferType = indexType,
		.size = worstCaseIndicesPerChunk * sizeof(u32),
		.stride = sizeof(u32),
	};

	std::shared_ptr<bs::vk::Buffer> chunk_indices = std::make_shared<bs::vk::Buffer>(basicDescription);
	m_open_spans.emplace_back(0, basicDescription.size());
	
	basicDescription.bufferType = instancedType;
	basicDescription.stride = sizeof(ChunkInstanceData);
	std::shared_ptr<bs::vk::Buffer> chunk_instances = std::make_shared<bs::vk::Buffer>(basicDescription);
	

	bs::asset_manager->addBuffer(chunk_indices, chunk_buffer_name);
	bs::asset_manager->addBuffer(chunk_instances, instance_buffer_name);
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
	return false;
}

void ChunkMeshManager::canDrop(const pos_xyz chunkPosition)
{
	m_droppableChunks.emplace_back(chunkPosition);
}

void ChunkMeshManager::canDrop(const Chunk& chunk)
{
	m_droppableChunks.emplace_back(chunk.getChunkPos());
}

bool ChunkMeshManager::isChunkCached(const pos_xyz chunkPosition)
{
	return false;
}

bool ChunkMeshManager::isChunkCached(const Chunk& chunk)
{
	return isChunkCached(chunk.getChunkPos());
}

void ChunkMeshManager::addChunkToBuffer(const Chunk& chunk)
{
	const ChunkDrawInfo drawInfo = createDrawInfoFromChunk(chunk);
	const IndexMesh indexMesh = buildIndexMesh(drawInfo);

	
}

void ChunkMeshManager::condenseBuffer()
{

}

ChunkDrawInfo ChunkMeshManager::createDrawInfoFromChunk(const Chunk& chunk) const
{
	ChunkDrawInfo c_info;
	c_info.chunk_pos = chunk.getChunkPos();

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

u32 ChunkMeshManager::findOpenSlot(const u32 data_length)
{
	return 0;
}