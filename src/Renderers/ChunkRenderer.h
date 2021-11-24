#pragma once

#include <Engine.h>

//@TODO: Add the necessary stuff
class ChunkRenderer
{
public:
	ChunkRenderer(bs::Device* mainDevice, VkRenderPass& rpass, VkDescriptorSetLayout desclayout);
	~ChunkRenderer();

	void buildRenderCommands();

	const VkCommandBuffer* getRenderCommand() const;
	
private:
	void clearCommandBuffer();
	
	void generateChunkData();
	static VertexInputDescription getChunkInputDescription();

	std::shared_ptr<bs::vk::Buffer> m_chunkbuffer;

	// Pipeline Stuff

	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;
	VkCommandBufferBeginInfo m_beginInfo;
	VkCommandBufferInheritanceInfo m_inheritanceInfo;
	VkRenderPass& m_renderpass;

	// Vulkan Stuff
	
	VkCommandPool m_pool;
	std::vector<VkCommandBuffer> m_renderlist;
	bs::Device* p_device;
};