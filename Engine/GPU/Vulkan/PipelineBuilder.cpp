#include "PipelineBuilder.h"

GraphicsPipelineBuilder::GraphicsPipelineBuilder(bs::Device* mainDevice)	: isBuilt(false), m_frag(false), m_vert(false), p_device(mainDevice)
{
	m_shaderModules.resize(2);
}

GraphicsPipelineBuilder::~GraphicsPipelineBuilder()
{
	if(m_vert)
	{
		vkDestroyShaderModule(p_device->getDevice(), m_shaderModules.at(0).module, nullptr);
	}
	if(m_frag)
	{
		vkDestroyShaderModule(p_device->getDevice(), m_shaderModules.at(1).module, nullptr);
	}
}

void GraphicsPipelineBuilder::addVertexShader(const std::string& filepath)
{
	auto shaderCode = bs::readFile(filepath);
	VkShaderModule shaderModule = createShaderModule(shaderCode, device.getDevice());

	auto& stageInfo = m_shaderModules.at(0);
	stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	stageInfo.module = shaderModule;
	stageInfo.pName = "main";
}

void GraphicsPipelineBuilder::addFragmentShader(const std::string& filepath)
{
	auto shaderCode = bs::readFile(filepath);
	VkShaderModule shaderModule = createShaderModule(shaderCode, device.getDevice());
	
	auto& stageInfo = m_shaderModules.at(1);
	stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stageInfo.module = shaderModule;
	stageInfo.pName = "main";
}

void GraphicsPipelineBuilder::build()
{
	isBuilt = true;
}

void GraphicsPipelineBuilder::getResults(VkPipeline& pipeline, VkPipelineLayout& pipelineLayout)
{

}