#pragma once

#include "GeneralRenderer.h"
#include "UIRenderer.h"
#include "ChunkRenderer.h"

class Camera;

class Renderer
{
public:
	Renderer(bs::Device* device, VkRenderPass genericPass);
	~Renderer();

	void drawObject(const bs::GameObject& entity);
	//NOT DONE DO NOT USE
	void drawText();

	//Pass tell the subrenderers to generate list queues
	void render(Camera& cam);

	//Render to the Framebuffer
	void finish(bs::vk::FramebufferData& fbo, int index);

	//Empty drawqueues
	void clearQueue();

	std::unique_ptr<GeneralRenderer>	m_generalRenderer;
	std::unique_ptr<UIRenderer>			m_UIRenderer;
		
	//Push the uniform buffer and image descriptor to the gpu
	void pushGPUData(Camera& cam);

private:
	void initGUI();

	void initDescriptorPool();
	void initDescriptorSets();

	void initCommandPoolAndBuffers();
	void initDescriptorSetBuffers();

	VkPipeline imguipipeline;
	VkPipelineLayout guilayout;

	VkRenderPass m_renderpassdefault;
	std::vector<VkCommandBuffer> m_primaryBuffers;
	VkCommandPool m_pool;

	bs::Device* device;

	VkDescriptorPool m_descpool;
	VkDescriptorSet m_descsetglobal;
	VkDescriptorSetLayout desclayout;
};