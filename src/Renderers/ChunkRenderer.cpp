#include "ChunkRenderer.h"

ChunkRenderer::ChunkRenderer(bs::Device* mainDevice, VkRenderPass& rpass, VkDescriptorSetLayout desclayout)	: 
	p_device(mainDevice), m_renderpass(rpass)
{
	VkCommandBufferBeginInfo bufferBeginInfo = {};
	bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	bufferBeginInfo.pInheritanceInfo = &m_inheritanceInfo;

	m_beginInfo = bufferBeginInfo;

	VkCommandBufferInheritanceInfo bufferInheritanceInfo = {};
	bufferInheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	bufferInheritanceInfo.renderPass = m_renderpass;
	// Set to Null because the finish render function has the target frame buffer to render to
	bufferInheritanceInfo.framebuffer = VK_NULL_HANDLE;

	m_inheritanceInfo = bufferInheritanceInfo;


	//Command Buffer Allocation
	bs::vk::createCommandPool(*p_device, m_pool);
	//Renderlists for secondary cmd buffers
	m_renderlist.resize(10);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_pool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	allocInfo.commandBufferCount = m_renderlist.size();

	if (vkAllocateCommandBuffers(p_device->getDevice(), &allocInfo, m_renderlist.data()) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

ChunkRenderer::~ChunkRenderer()
{
	vkDestroyPipelineLayout(p_device->getDevice(), m_pipelineLayout, nullptr);
	vkDestroyPipeline(p_device->getDevice(), m_pipeline, nullptr);
	vkDestroyCommandPool(p_device->getDevice(), m_pool, nullptr);
}

void ChunkRenderer::generateChunkData()
{
	bs::vk::BufferDescription chunkdata{};
	chunkdata.bufferType = bs::vk::BufferUsage::VERTEX_BUFFER;

	
}