#pragma once

#include <Engine.h>

//@TODO: Add the necessary stuff
class ChunkRenderer
{
public:
	ChunkRenderer();
	~ChunkRenderer() = default;

	
private:
	void generateChunkData();

	std::shared_ptr<bs::vk::Buffer> m_chunkbuffer;
};