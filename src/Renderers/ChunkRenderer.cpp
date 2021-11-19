#include "ChunkRenderer.h"

#include "../World/Meshing/ChunkMeshManager.h"
#include <GPU/Vulkan/PipelineBuilder.h>

constexpr u32 VERTEX_INPUT_BINDING = 0;
constexpr u32 INSTANCE_INPUT_BINDING = 1;

ChunkRenderer::ChunkRenderer(bs::Device* mainDevice, VkRenderPass& rpass, VkDescriptorSetLayout desclayout)	: 
	p_device(mainDevice), m_renderpass(rpass)
{
	bs::vk::createCommandPool(*p_device, m_pool);

	//Renderlists for secondary cmd buffers
	m_renderlist.resize(1);

	const VkCommandBufferAllocateInfo allocInfo
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = m_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
		.commandBufferCount = (u32)m_renderlist.size()
	};

	if(vkAllocateCommandBuffers(p_device->getDevice(), &allocInfo, m_renderlist.data()) != VK_SUCCESS) 
	{
		throw std::runtime_error("Failed to allocate command buffers!");
	}

	//Cmd buffer Inheritance info init
	m_inheritanceInfo = VkCommandBufferInheritanceInfo
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		.pNext = nullptr,
		.renderPass = m_renderpass,
		.subpass = 0,
		// Set to Null because the finish render function has the target frame buffer to render to
		.framebuffer = VK_NULL_HANDLE,
		.occlusionQueryEnable = 0,
		.queryFlags = 0,
		.pipelineStatistics = 0
	};

	//Cmd buffer starting info
	m_beginInfo = VkCommandBufferBeginInfo 
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
		.pInheritanceInfo = &m_inheritanceInfo
	};
	
	bs::vk::GraphicsPipelineBuilder chunkPipelineBuilder(p_device, m_renderpass, desclayout);
	chunkPipelineBuilder.addVertexShader("res/Shaders/vert.spv");
	chunkPipelineBuilder.addFragmentShader("res/Shaders/frag.spv");
	chunkPipelineBuilder.setDrawMode(bs::vk::DrawMode::FILL);
	chunkPipelineBuilder.setRasterizingData(false, true);
	chunkPipelineBuilder.setPushConstantSize(0);

	//Create the chunk vertex chunkDescription
	chunkPipelineBuilder.useVertexDescription(getChunkInputDescription());
	
	chunkPipelineBuilder.build();
	chunkPipelineBuilder.getResults(m_pipeline, m_pipelineLayout);
}

ChunkRenderer::~ChunkRenderer()
{
	vkDestroyPipelineLayout(p_device->getDevice(), m_pipelineLayout, nullptr);
	vkDestroyPipeline(p_device->getDevice(), m_pipeline, nullptr);
	vkDestroyCommandPool(p_device->getDevice(), m_pool, nullptr);
}

void ChunkRenderer::buildRenderCommands()
{
	//Dynamic State Stuff:
	const VkExtent2D extent
	{
		.width = bs::vk::viewportwidth,
		.height = bs::vk::viewportheight,
	};
	const VkViewport viewport
	{
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)extent.width,
		.height = (float)extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	const VkRect2D scissor
	{
		.offset = { 0, 0 },
		.extent = extent,
	};

	//RECORDERINO!
	auto& cmd = m_renderlist[0];

	//Begin the RECORDING!!!
	vkBeginCommandBuffer(cmd, &m_beginInfo);
	
	//Bind the descriptor set
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, 
		bs::asset_manager->pDescsetglobal, 0, nullptr);

	//Bind the chunk pipeline
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
	
	//Set dynamic state
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	vkCmdSetScissor(cmd, 0, 1, &scissor);
	
	//Set the push constant
	vkCmdPushConstants(cmd, m_pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 0, nullptr);

	VkDeviceSize offset = 0;
	//Bind Mesh Vertex Data, this should be staged to the GPU to ensure MAX performance
	vkCmdBindVertexBuffers(cmd, 0, 1, &m_chunkbuffer->getAPIResource(), &offset);

	//Bind instance data buffer (holding transforms, textures, and whatever else)
	// vkCmdBindVertexBuffers(cmd, 1, 1, &m_chunkbuffer->getAPIResource(), &offset);

	//Temp section, replace when doing multidraw indirect
	std::vector<Chunk*> chunklist;
	for(const auto* chunk : chunklist)
	{
		// vkCmdBindIndexBuffer(cmd, ->getAPIResource(), offset, VK_INDEX_TYPE_UINT16);

		constexpr auto maxnumindices = 16 * 16 * 16 * 6 * 6;//147456
		constexpr auto maxnumverts = 16 * 16 * 16 * 6 * 4;	//98304
		constexpr auto minverts = 17 * 17 * 17; //4913
		vkCmdDrawIndexed(cmd, maxnumindices, 1, 0, 0, 0);
	}

	vkEndCommandBuffer(cmd);
}

const VkCommandBuffer* ChunkRenderer::getRenderCommand() const
{
	return &m_renderlist[0];
}

void ChunkRenderer::generateChunkData()
{
	bs::vk::BufferDescription chunkdata{};
	chunkdata.bufferType = bs::vk::BufferUsage::VERTEX_BUFFER;

	
}

VertexInputDescription ChunkRenderer::getChunkInputDescription()
{
	VertexInputDescription chunkDescription;

	///VERTEX
	//This is for the per Vertex Binding
	chunkDescription.bindings.emplace_back(VkVertexInputBindingDescription 
	{
		.binding = VERTEX_INPUT_BINDING,
		.stride = sizeof(bs::vec4),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	});
	///INSTANCE
	//This is for the per Instance Binding
	chunkDescription.bindings.emplace_back(VkVertexInputBindingDescription 
	{
		.binding = INSTANCE_INPUT_BINDING,
		.stride = sizeof(ChunkInstanceData),
		.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE,
	});

	u32 cur_location = 0;

	//Vertex Data will be stored at Location 0
	//Chunk Vert Attribute
	chunkDescription.attributes.emplace_back(VkVertexInputAttributeDescription 
	{
		.location = cur_location++,
		.binding = VERTEX_INPUT_BINDING,
		.format = VK_FORMAT_R32G32B32A32_SFLOAT,	//vec4
		.offset = 0,
	});

	//Instance Data
	//The Chunk Position attribute
	chunkDescription.attributes.emplace_back(VkVertexInputAttributeDescription 
	{
		.location = cur_location++,
		.binding = INSTANCE_INPUT_BINDING,
		.format = VK_FORMAT_R32G32_SINT,	//vec3i
		.offset = 0,
	});
	//The Array of Textures Attribute?
	chunkDescription.attributes.emplace_back(VkVertexInputAttributeDescription 
	{
		.location = cur_location++,
		.binding = INSTANCE_INPUT_BINDING,
		.format = VK_FORMAT_R16_UINT,	//u16 or smth
		.offset = sizeof(bs::vec3i),
	});

	return chunkDescription;
}