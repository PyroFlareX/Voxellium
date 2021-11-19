#include "ChunkMeshManager.h"

ChunkMeshManager::ChunkMeshManager()
{

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

}

void ChunkMeshManager::condenseBuffer()
{

}

ChunkDrawInfo ChunkMeshManager::createDrawInfoFromChunk(const Chunk& chunk) const
{

}

const std::array<u32, 6> ChunkMeshManager::getIndicesFromFaceIndex(const u16 faceIndex)
{

}

ChunkMeshManager::IndexMesh ChunkMeshManager::buildIndexMesh(const ChunkDrawInfo& drawInfo) const
{
	IndexMesh mesh;
	for(const auto& face : drawInfo.faces)
	{
		const auto indices = getIndicesFromFaceIndex(face.faceIndex);
		mesh.meshindicies.emplace_back(indices);
	}

	return mesh;
}