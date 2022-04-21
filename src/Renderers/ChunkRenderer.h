#pragma once

#include <Engine.h>

#include "../World/World.h"

class ChunkRenderer
{
public:
	ChunkRenderer(bs::Device* mainDevice, VkRenderPass& rpass, VkDescriptorSetLayout desclayout);
	~ChunkRenderer();

	//Build the chunk drawing commands
	void buildRenderCommands();
	
	//Clear the draw command buffer
	void clearCommandBuffer();

	//Execute the commands for chunk drawing
	void executeCommands(VkCommandBuffer cmd);

	//Chunk Mesh Manager
	ChunkMeshManager* p_mesh_manager;
private:
	void generateChunkData();
	static VertexInputDescription getChunkInputDescription();

	//The staged GPU-only buffer for the vertex buffer of the chunk
	std::unique_ptr<bs::vk::Buffer> m_chunkbuffer;

	// Pipeline Stuff
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;
	VkRenderPass& m_renderpass;

	//Command Buffers Stuff
	VkCommandBufferBeginInfo m_beginInfo;
	VkCommandBufferInheritanceInfo m_inheritanceInfo;
	VkCommandPool m_pool;
	std::vector<VkCommandBuffer> m_renderlist;

	//Device
	bs::Device* p_device;

	bool recorded;
};