#pragma once

#include "VulkanHelpers.h"

class GraphicsPipelineBuilder
{
public:
	GraphicsPipelineBuilder(bs::Device* mainDevice);
	~GraphicsPipelineBuilder();

	void addVertexShader(const std::string& filepath);
	void addFragmentShader(const std::string& filepath);



	void build();

	void getResults(VkPipeline& pipeline, VkPipelineLayout& pipelineLayout);
private:
	bool isBuilt;
	bool m_frag;
	bool m_vert;
	bs::Device* p_device;

	VkPipeline m_pipeline;
	VkPipelineLayout m_layout;

	VkGraphicsPipelineCreateInfo m_gfxCreateInfo;
	VkPipelineLayoutCreateInfo m_layoutinfo;

	std::vector<VkPipelineShaderStageCreateInfo> m_shaderModules;
};