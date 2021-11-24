#include "Renderer.h"

#include <imgui.h>
#include <algorithm>

constexpr int numDescriptors = 3;

Renderer::Renderer(bs::Device* renderingDevice, VkRenderPass genericPass)	: device(renderingDevice), m_renderpassdefault(genericPass)
{
	//Add textures
	//Create image + texture
	bs::vk::Texture font(device);
	//Adds empty texture, gets updated later, index 0 is the font texture
	bs::asset_manager->addTexture(font, 0);	
	
	//Blank white img
	const bs::Image imgblank({32, 32}, bs::u8vec4(255));

	// Duping img
	bs::vk::Texture texture(device);
	texture.loadFromImage(imgblank);
	//Adding black white image for texture index 1
	bs::asset_manager->addTexture(texture, 1);

	//Generate a special image: (diagonal purple)
	constexpr bs::u8vec4 purple = bs::u8vec4(255, 0, 255, 255);
	constexpr bs::u8vec4 cyan = bs::u8vec4(64, 225, 205, 255);
	bs::Image notFoundImg(imgblank.getSize(), cyan);
	{
		//This assumes a square img
		const auto numColumns = notFoundImg.getSize().y;
		for(auto y = 0; y < numColumns; y += 1)
		{
			//Calc the number of pixels to set on this row
			auto lengthToSet = numColumns - y;

			for(auto x = 0; x < lengthToSet; x+=1)
			{
				notFoundImg.setPixel(x, y, purple);
			}
		}
	}
	bs::vk::Texture textureblank(device);
	textureblank.loadFromImage(notFoundImg);
	//Adding a diagonal purple texture for index 2
	bs::asset_manager->addTexture(textureblank, 2);

	//For IMGUI
	initGUI();
	
	//Init the command pool and the cmd buffer
	initCommandPoolAndBuffers();

	// Descriptor Pool
	initDescriptorPool();
	// Descriptor Set
	initDescriptorSets();
	// Initializing the descriptor set buffers
	initDescriptorSetBuffers();

	//Create General Renderer
	m_generalRenderer = std::make_unique<GeneralRenderer>(device, m_renderpassdefault, desclayout);
	//Create the chunk renderer
	m_chunkRenderer = std::make_unique<ChunkRenderer>(device, m_renderpassdefault, desclayout);
	//Create UI Renderer Pipeline And the Renderer
	bs::vk::createUIPipeline(*device, imguipipeline, m_renderpassdefault, guilayout, desclayout);
	m_UIRenderer = std::make_unique<UIRenderer>(device, imguipipeline, guilayout);
}

Renderer::~Renderer()
{
	// GUI/ImGui related
	vkDestroyPipeline(device->getDevice(), imguipipeline, nullptr);
	vkDestroyPipelineLayout(device->getDevice(), guilayout, nullptr);
	// Destroy the rest of the Vulkan allocations
	vkDestroyCommandPool(device->getDevice(), m_pool, nullptr);
	vkDestroyDescriptorSetLayout(device->getDevice(), desclayout, nullptr);
	vkDestroyDescriptorPool(device->getDevice(), m_descpool, nullptr);
}

void Renderer::initGUI()
{
	//Init IMGUI
	ImGuiIO& io = ImGui::GetIO();
	//IMGUI STUFF
	{
		uint8_t zeroarray[120] = { 0 };

		bs::vk::BufferDescription vbufdesc = {};
		vbufdesc.bufferType = bs::vk::VERTEX_BUFFER;
		vbufdesc.bufferData = zeroarray;
		vbufdesc.dev = device;
		vbufdesc.stride = sizeof(ImDrawVert);
		vbufdesc.size = 120;
		
		bs::vk::BufferDescription ibufdesc = {};
		ibufdesc.bufferType = bs::vk::INDEX_BUFFER;
		ibufdesc.bufferData = zeroarray;
		ibufdesc.dev = device;
		ibufdesc.stride = sizeof(ImDrawIdx);
		ibufdesc.size = 120;

		////Create the ImGui Vertex Buffer
		bs::asset_manager->addBuffer(std::make_shared<bs::vk::Buffer>(vbufdesc), "GUIvert");

		//Create the ImGui Index Buffer
		bs::asset_manager->addBuffer(std::make_shared<bs::vk::Buffer>(ibufdesc), "GUIindex");
	}

	//IMGUI FONT STUFF
	{
		bs::Image imgfont;
		unsigned char* fontData;
		bs::vec2i fontsize;
		io.Fonts->AddFontDefault();
		io.Fonts->GetTexDataAsRGBA32(&fontData, &fontsize.x, &fontsize.y);
		imgfont.create(fontsize.x, fontsize.y, (bs::u8vec4*)fontData);
		
		bs::vk::Texture& font = bs::asset_manager->getTextureMutable(0);
		font.loadFromImage(imgfont);
		bs::asset_manager->addImg(imgfont, "font");
	}
}

void Renderer::drawObject(const bs::GameObject& entity)
{
	m_generalRenderer->addInstance(entity);
}

void Renderer::drawText()
{
	m_UIRenderer->addText("Example Text", {500, 500});
}

void Renderer::recreateChunkDrawCommands(const std::vector<Chunk::ChunkMesh>& drawInfos)
{
	m_chunkRenderer->buildRenderCommands(drawInfos);
}

void Renderer::render(Camera& cam)
{
	m_UIRenderer->render();

	ImGui::Render();
	ImGui::EndFrame();

	pushGPUData(cam);
	m_UIRenderer->bakeImGui();

	//Main Pass
	jobSystem.wait();

	m_generalRenderer->render(cam);

	jobSystem.wait();
}

void Renderer::finish(bs::vk::FramebufferData& fbo, int index)
{
	vkResetCommandPool(device->getDevice(), m_pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
	//Second Pass
	auto renderLists = m_generalRenderer->getRenderlists(); // The secondary command buffers

	// FOR IM GUI BULLSHIT 
	{
		auto& cmd = renderLists.at(0);
		
		m_UIRenderer->finish(cmd);
	}

	//Renderpass info stuff
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_renderpassdefault;
	renderPassInfo.framebuffer = fbo.handle[index];
	renderPassInfo.renderArea.offset = { 0, 0 };
	//Extent defining for renderpass
	const VkExtent2D extent
	{
		.width = bs::vk::viewportwidth,
		.height = bs::vk::viewportheight,
	};
	renderPassInfo.renderArea.extent = extent;

	//Clear values for renderpass
	const VkClearValue clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
	VkClearValue clearDepth = { };
	clearDepth.depthStencil.depth = 1.0f;
	const VkClearValue clearValues[2] = { clearColor, clearDepth };
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = &clearValues[0];

	//Primary Buffer Recording
	//Begin Info
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	for(auto& cmd : m_primaryBuffers)
	{
		//Begin Recording the buffer
		if(vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) 
		{
			throw std::runtime_error("Failed to begin recording command buffer!");
		}
		//Begin the renderpass
		//VK_SUBPASS_CONTENTS_INLINE //VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
		vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

		///	ACTUAL RECORDING DONE HERE:

		/**
		 * Order: (if/when it matters, depth buffer makes this partially irrelevant)
		 * #1: General Renderer / UI
		 * ...
		 * Chunk Renderer
		 * 
		 * The others
		 * ...
		 * 
		**/
		
		//Execute all the cmd buffers for the general renderer and UI
		vkCmdExecuteCommands(cmd, renderLists.size(), renderLists.data());
		

		//Chunk Renderer
		vkCmdExecuteCommands(cmd, 1, m_chunkRenderer->getRenderCommand());
		//OTHERS
		// ...

		/// ENDING THE RECORDING
		vkCmdEndRenderPass(cmd);
		if(vkEndCommandBuffer(cmd) != VK_SUCCESS) 
		{
			throw std::runtime_error("Failed to record command buffer!");
		}
	}

	//Submit main rendering
	device->submitWork(m_primaryBuffers);
}

void Renderer::clearQueue()
{
	m_generalRenderer->clearQueue();
}

void Renderer::pushGPUData(Camera& cam)
{
	//Buffer Writing Info
	VkDescriptorBufferInfo bufferInfo1 = {};
	auto buf = bs::asset_manager->getBuffer("MVP");
	bufferInfo1.buffer = buf->getAPIResource();
	bufferInfo1.offset = 0;
	bufferInfo1.range = 192;

	const bs::Transform t;

	struct MVPstruct
	{
		bs::mat4 proj;
		bs::mat4 view;
		bs::mat4 model;
	};
	MVPstruct MVP = { 
		.proj	= cam.getProjMatrix(), 
		.view	= cam.getViewMatrix(), 
		.model	= bs::makeModelMatrix(t),
	};

	buf->writeBuffer(&MVP);

	//Image Writing Info
	//Collect textures
	const auto& textures = bs::asset_manager->getTextures();
	std::vector<VkDescriptorImageInfo> imageinfo;
	imageinfo.reserve(textures.size());

	for(int i = 0; i < textures.size(); ++i)
	{
		VkDescriptorImageInfo imginfo;
		imginfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imginfo.imageView = textures[i].imgviewvk;
		imginfo.sampler = textures[i].sampler;
		imageinfo.emplace_back(imginfo);
	}
	
	//Writing Info
	VkWriteDescriptorSet descWrite[numDescriptors] = {};
	descWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descWrite[0].dstSet = m_descsetglobal;
	descWrite[0].dstBinding = 0;
	descWrite[0].dstArrayElement = 0; //Starting array element
	descWrite[0].descriptorCount = 1; //Number to write over
	descWrite[0].pBufferInfo = &bufferInfo1;

	descWrite[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descWrite[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descWrite[1].dstSet = m_descsetglobal;
	descWrite[1].dstBinding = 1;
	descWrite[1].dstArrayElement = 0;
	descWrite[1].descriptorCount = imageinfo.size();
	descWrite[1].pImageInfo = imageinfo.data();

	vkUpdateDescriptorSets(device->getDevice(), numDescriptors - 1, &descWrite[0], 0, nullptr);
}

void Renderer::initDescriptorPool()
{
	//Descriptor pool stuff
	VkDescriptorPoolCreateInfo descpoolinfo{};

	descpoolinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descpoolinfo.pNext = nullptr;
	descpoolinfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

	VkDescriptorPoolSize descpoolsize[numDescriptors] = {};
	descpoolsize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;	//For the (M?) VP matrix
	descpoolsize[0].descriptorCount = 1;

	descpoolsize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descpoolsize[1].descriptorCount = bs::asset_manager->getNumTextures();

	descpoolsize[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descpoolsize[2].descriptorCount = 1;

	descpoolinfo.pPoolSizes = &descpoolsize[0];
	descpoolinfo.poolSizeCount = numDescriptors;
	descpoolinfo.maxSets = 4;

	VkResult result = vkCreateDescriptorPool(device->getDevice(), &descpoolinfo, nullptr, &m_descpool);
	if(result != VK_SUCCESS)
	{
		std::cerr << "Creating Descriptor Pool Failed, result = " << result << "\n";
		throw std::runtime_error("Creating Descriptor Pool Failed!!!");
	}
}

void Renderer::initDescriptorSets()
{
	// Descriptor Sets
	VkDescriptorSetLayoutBinding setlayoutbinding[numDescriptors] = {};
	setlayoutbinding[0].binding = 0;
	setlayoutbinding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	setlayoutbinding[0].stageFlags = VK_SHADER_STAGE_ALL;
	setlayoutbinding[0].descriptorCount = 1;

	setlayoutbinding[1].binding = 1;
	setlayoutbinding[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	setlayoutbinding[1].stageFlags = VK_SHADER_STAGE_ALL;
	setlayoutbinding[1].descriptorCount = bs::asset_manager->getNumTextures();

	setlayoutbinding[2].binding = 2;
	setlayoutbinding[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	setlayoutbinding[2].stageFlags = VK_SHADER_STAGE_ALL;
	setlayoutbinding[2].descriptorCount = 1;

	//For texture indexing:
	VkDescriptorSetLayoutBindingFlagsCreateInfo layoutbindingflags = {};
	layoutbindingflags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	layoutbindingflags.bindingCount = numDescriptors;
	VkDescriptorBindingFlags flags[numDescriptors] = { 0, VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT, 0 };
	layoutbindingflags.pBindingFlags = flags;
		
	//Layout Creation for bindings
	VkDescriptorSetLayoutCreateInfo desclayoutinfo{};
	desclayoutinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	desclayoutinfo.pNext = &layoutbindingflags;
	desclayoutinfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
	desclayoutinfo.bindingCount = numDescriptors;
	desclayoutinfo.pBindings = &setlayoutbinding[0];
		
	//Creating the layouts for the descriptor sets
	VkResult result = vkCreateDescriptorSetLayout(device->getDevice(), &desclayoutinfo, nullptr, &desclayout);
	if(result != VK_SUCCESS)
	{
		std::cerr << "Creating Descriptor Set Layout Failed, result = " << result << "\n";
	}

	//Descriptor Allocation Info
	VkDescriptorSetAllocateInfo descriptorAllocInfo{};
	descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

	//For descriptor indexing
	VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescAlloc = {};
	variableDescAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
	variableDescAlloc.descriptorSetCount = 1;
	u32 varDescCount[] = { (u32)bs::asset_manager->getNumTextures() };
	variableDescAlloc.pDescriptorCounts = varDescCount;
	//Filling the pNext
	descriptorAllocInfo.pNext = &variableDescAlloc;
	descriptorAllocInfo.descriptorPool = m_descpool;
	descriptorAllocInfo.descriptorSetCount = 1;
	descriptorAllocInfo.pSetLayouts = &desclayout;
		
	result = vkAllocateDescriptorSets(device->getDevice(), &descriptorAllocInfo, &m_descsetglobal);
	if(result != VK_SUCCESS)
	{
		std::cerr << "Allocate Descriptor Sets Failed, result = " << result << "\n";
	}

	bs::asset_manager->pDescsetglobal = &m_descsetglobal;
}

void Renderer::initCommandPoolAndBuffers()
{
	bs::vk::createCommandPool(*device, m_pool);

	m_primaryBuffers.resize(1);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_pool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = m_primaryBuffers.size();
	
	VkResult result = vkAllocateCommandBuffers(device->getDevice(), &allocInfo, m_primaryBuffers.data());
	if(result != VK_SUCCESS) 
	{
		std::cerr << "Failed to allocate command buffers, error code: " << result << "\n";
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void Renderer::initDescriptorSetBuffers()
{
	typedef struct
	{
		bs::mat4 proj;
		bs::mat4 view;
	} ProjView;
	
	struct MVP
	{
		bs::mat4 proj;
		bs::mat4 view;
		bs::mat4 model;
	} uniformbufferthing;

	// Descriptor Set Buffers:

	// Uniform buffer
	bs::vk::BufferDescription uniform;
	uniform.bufferType = bs::vk::BufferUsage::UNIFORM_BUFFER;
	uniform.dev = device;
	uniform.size = sizeof(MVP);
	uniform.stride = sizeof(bs::mat4);
	uniform.bufferData = &uniformbufferthing;

	bs::asset_manager->addBuffer(std::make_shared<bs::vk::Buffer>(uniform), "MVP");
}