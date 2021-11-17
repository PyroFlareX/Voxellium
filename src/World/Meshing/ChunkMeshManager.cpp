#include "ChunkMeshManager.h"

ChunkMeshManager::ChunkMeshManager()
{

}

ChunkMeshManager::~ChunkMeshManager()
{

}

void ChunkMeshManager::setRenderDistance(const u32 renderDistance)
{

}

bool ChunkMeshManager::cacheChunk(const Chunk& chunk)
{
	return false;
}

void ChunkMeshManager::canDrop(const pos_xyz chunkPosition)
{

}

void ChunkMeshManager::canDrop(const Chunk& chunk)
{

}

bool ChunkMeshManager::isChunkCached(const pos_xyz chunkPosition)
{
	return false;
}

bool ChunkMeshManager::isChunkCached(const Chunk& chunk)
{
	return isChunkCached(chunk.getChunkPos());
}