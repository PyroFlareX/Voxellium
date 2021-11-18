#include "GeneralRenderer.h"


#include <GPU/Vulkan/PipelineBuilder.h>

GeneralRenderer::GeneralRenderer(bs::Device* mainDevice, VkRenderPass& rpass, VkDescriptorSetLayout desclayout)	: 
	p_device(mainDevice), m_renderpass(rpass)
{
	bs::vk::createCommandPool(*p_device, m_pool);

	//Renderlists for secondary cmd buffers
	m_renderlist.resize(2);

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
		throw std::runtime_error("failed to allocate command buffers!");
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
	
	bs::vk::GraphicsPipelineBuilder graphicsPipelineBuilder(p_device, m_renderpass, desclayout);
	graphicsPipelineBuilder.addVertexShader("res/Shaders/vert.spv");
	graphicsPipelineBuilder.addFragmentShader("res/Shaders/frag.spv");
	graphicsPipelineBuilder.setDrawMode(bs::vk::DrawMode::FILL);
	graphicsPipelineBuilder.setRasterizingData(false, true);
	graphicsPipelineBuilder.setPushConstantSize(sizeof(PushConstantsStruct));
	graphicsPipelineBuilder.useVertexDescription(bs::vk::getVertexDescription());
	
	graphicsPipelineBuilder.build();
	graphicsPipelineBuilder.getResults(m_genericPipeline, m_pipelineLayout);

	jobSystem.wait();
}

GeneralRenderer::~GeneralRenderer()
{
	vkDestroyPipelineLayout(p_device->getDevice(), m_pipelineLayout, nullptr);
	vkDestroyPipeline(p_device->getDevice(), m_genericPipeline, nullptr);
	vkDestroyCommandPool(p_device->getDevice(), m_pool, nullptr);
}

void GeneralRenderer::addInstance(const bs::GameObject& entity)
{
	m_queue.emplace_back(entity);
}

void GeneralRenderer::render(Camera& cam)
{
	PushConstantsStruct pushconst 
	{
		.textureids = {
			2,	//first is texture
			1,	//second is normals
			0,	//third is ???
			0	//forth is ???
		},
	};

	//For Dynamic State
	const VkExtent2D extent
	{
		.width = bs::vk::viewportwidth,
		.height = bs::vk::viewportheight,
	};
	
	// STUPID WORKAROUND THING : THIS STARTS THE RECORDING FOR THE IMGUI SECONDARY
	// The idea is thet I reserve the 0th index of the secondary cmd buffers for imgui rendering
	if (vkBeginCommandBuffer(m_renderlist.at(0), &m_beginInfo) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to begin recording command buffer!");
	}
	
	for(auto i = 1; i < m_queue.size() + 1; ++i)
	{
		if (vkBeginCommandBuffer(m_renderlist.at(i), &m_beginInfo) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to begin recording command buffer!");
		}
		
		vkCmdBindPipeline(m_renderlist.at(i), VK_PIPELINE_BIND_POINT_GRAPHICS, m_genericPipeline);
		
		const VkViewport viewport {
			.x = 0.0f,
			.y = 0.0f,
			.width = (float)extent.width,
			.height = (float)extent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		};

		const VkRect2D scissor {
			.offset = { 0, 0 },
			.extent = extent,
		};

		vkCmdSetViewport(m_renderlist.at(i), 0, 1, &viewport);
		vkCmdSetScissor(m_renderlist.at(i), 0, 1, &scissor);

		vkCmdBindDescriptorSets(m_renderlist.at(i), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, 
			bs::asset_manager->pDescsetglobal, 0, nullptr);
		

		pushconst.textureids = bs::vec4		//uncomment when it actually becomes necessary to render >1 object that have different textures
		(
			m_queue.at(i - 1).material.texture_id,	//first is texture
			m_queue.at(i - 1).material.normal_id,	//second is normals
			0,	//third is ???
			0	//forth is ???
		);
		
		vkCmdPushConstants(m_renderlist.at(i), m_pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantsStruct), &pushconst);

		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(m_renderlist.at(i), 0, 1, &bs::asset_manager->getModel(std::move(m_queue.at(i - 1).model_id)).getVertexBuffer()->getAPIResource(), &offset);
		
		vkCmdBindIndexBuffer(m_renderlist.at(i), bs::asset_manager->getModel(std::move(m_queue.at(i - 1).model_id)).getIndexBuffer()->getAPIResource(), offset, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(m_renderlist.at(i), bs::asset_manager->getModel(std::move(m_queue.at(i - 1).model_id)).getIndexBuffer()->getNumElements(), 1, 0, 0, 0);

		if (vkEndCommandBuffer(m_renderlist.at(i)) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to record command buffer!");
		}
	}
}

void GeneralRenderer::clearQueue()
{
	m_queue.clear();
	vkResetCommandPool(p_device->getDevice(), m_pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
}

// Get list of secondary cmd buffers
std::vector<VkCommandBuffer>& GeneralRenderer::getRenderlists()
{
	return m_renderlist;
}
