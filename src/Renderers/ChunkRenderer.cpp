#include "ChunkRenderer.h"

#include "../World/Meshing/ChunkMeshManager.h"
#include "../World/Meshing/ChunkMesher.h"

#include <GPU/Vulkan/PipelineBuilder.h>

constexpr u32 VERTEX_INPUT_BINDING = 0;
constexpr u32 INSTANCE_INPUT_BINDING = 1;

ChunkRenderer::ChunkRenderer(bs::Device* mainDevice, VkRenderPass& rpass, VkDescriptorSetLayout desclayout)	: 
	p_device(mainDevice), m_renderpass(rpass), recorded(false)
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
	chunkPipelineBuilder.addVertexShader("res/Shaders/Chunk.vert.spv");
	chunkPipelineBuilder.addFragmentShader("res/Shaders/Chunk.frag.spv");
	chunkPipelineBuilder.setDrawMode(bs::vk::DrawMode::FILL);
	chunkPipelineBuilder.setRasterizingData(false, true);
	chunkPipelineBuilder.setPushConstantSize(0);

	//Create the chunk vertex chunkDescription
	chunkPipelineBuilder.useVertexDescription(getChunkInputDescription());
	
	chunkPipelineBuilder.build();
	chunkPipelineBuilder.getResults(m_pipeline, m_pipelineLayout);

	//Make the chunk vertex mesh
	generateChunkData();

	//Build a demo cmd buffer so validation layers don't complain about it being empty
	auto& cmd = m_renderlist[0];
	vkBeginCommandBuffer(cmd, &m_beginInfo);
	vkEndCommandBuffer(cmd);
}

ChunkRenderer::~ChunkRenderer()
{
	vkDestroyPipelineLayout(p_device->getDevice(), m_pipelineLayout, nullptr);
	vkDestroyPipeline(p_device->getDevice(), m_pipeline, nullptr);
	vkDestroyCommandPool(p_device->getDevice(), m_pool, nullptr);
}

void ChunkRenderer::buildRenderCommands(const std::vector<Chunk::ChunkMesh>& drawInfos)
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

	//Names of the buffers
	const std::string chunk_buffer_name("chunk_indices");
	const std::string instance_buffer_name("chunk_instance_data");
	const std::string texture_storage_buffer_name("chunk_texture_data");

	//Command Buffer for recording
	auto& cmd = m_renderlist[0];

	//Begin the RECORDING!!!
	clearCommandBuffer();
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
	vkCmdBindVertexBuffers(cmd, VERTEX_INPUT_BINDING, 1, &m_chunkbuffer->getAPIResource(), &offset);

	//Bind instance data buffer (holding transforms, textures, and whatever else)
	vkCmdBindVertexBuffers(cmd, INSTANCE_INPUT_BINDING, 1, &bs::asset_manager->getBuffer(instance_buffer_name)->getAPIResource(), &offset);

	//Bind Index Buffer Data
	vkCmdBindIndexBuffer(cmd, bs::asset_manager->getBuffer(chunk_buffer_name)->getAPIResource(), offset, VK_INDEX_TYPE_UINT32);

	//Temp section, replace when doing multidraw indirect
	for(const auto& chunk : drawInfos)
	{
		std::cout << "Chunk Draw Data:\n\t"
			<< "Indices Count: " << chunk->numIndices << "\n\t"
			<< "Faces Count: " << chunk->faces.size() << "\n\t"
			<< "Instance ID: " << chunk->instanceID << "\n\t"
			<< "Starting Byte Offset: " << chunk->startOffset << "\n";
		
		//From byte offset divided by stride to index offset
		u32 baseIndex = chunk->startOffset / sizeof(u32);
		vkCmdDrawIndexed(cmd, chunk->numIndices, 1, 0, 0, 0/*chunk->instanceID*/);
	}

	vkEndCommandBuffer(cmd);

	recorded = true;
}

void ChunkRenderer::executeCommands(VkCommandBuffer cmd)
{
	if(recorded)
	{
		vkCmdExecuteCommands(cmd, 1, m_renderlist.data());
	}
}

void ChunkRenderer::clearCommandBuffer()
{
	vkResetCommandPool(p_device->getDevice(), m_pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
	recorded = false;
}

void ChunkRenderer::generateChunkData()
{
	auto chunkVerts = createFullChunkMesh();

	std::cout << "Base Chunk Mesh Data:\n\t"
		<< "Num Vertices: " << chunkVerts.size() << "\n\t"
		<< "Size in Bytes: " << chunkVerts.size() * sizeof(bs::vec4) << "\n\t"
		<< "TBD: " << " " << "\n";

	const bs::vk::BufferDescription chunkdata
	{
		.dev = p_device,
		.bufferType = bs::vk::BufferUsage::VERTEX_BUFFER,
		.size = chunkVerts.size() * sizeof(bs::vec4),
		.stride = sizeof(bs::vec4),
		.bufferData = chunkVerts.data(),
	};

	m_chunkbuffer = std::make_shared<bs::vk::Buffer>(chunkdata);
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
		.format = VK_FORMAT_R32G32B32_SINT,	//vec3i
		.offset = 0,
	});
	//The offset index into the storage buffer for the chunk textures
	chunkDescription.attributes.emplace_back(VkVertexInputAttributeDescription 
	{
		.location = cur_location++,
		.binding = INSTANCE_INPUT_BINDING,
		.format = VK_FORMAT_R32_UINT,	//u32 or smth
		.offset = sizeof(bs::vec3i),
	});

	return chunkDescription;
}