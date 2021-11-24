#include "Device.h"

namespace bs
{
	Device::Device() : destroyed(false)
	{
		requiredDeviceExtensions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		optionalDeviceExtensions = {};
	}

	Device::~Device() 
	{
		destroy();
	}

	void Device::init()
	{
		int max_score = -1;
		const auto device_list = getPhysicalDevices();
		for(const auto& device : device_list)
		{
			if(max_score < device.second)
			{
				max_score = device.second;
				physDevice = device.first;
			}
		}

		// bs::vk::pickPhysicalDevice(physDevice);
		// bs::vk::createLogicalDevice(device, physDevice, graphicsQueue, presentQueue);

		createDevice();

		//initialize the memory allocator
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = physDevice;
		allocatorInfo.device = device;
		allocatorInfo.instance = vk::m_instance;
		vmaCreateAllocator(&allocatorInfo, &m_allocatorVMA);

		bs::vk::createCommandPool(*this, m_pool);
	}

	void Device::destroy()
	{
		if(destroyed)
		{
			return;
		}

		vkDestroyCommandPool(device, m_pool, nullptr);
		vkDestroyDevice(device, nullptr);
		destroyed = true;
	}

	QueueFamilyIndices Device::getQueueFamilies()
	{
		return bs::vk::findQueueFamilies(physDevice);
	}

	SwapChainSupportDetails Device::getSwapchainDetails()
	{
		return bs::vk::querySwapChainSupport(physDevice);
	}

	VkDevice& Device::getDevice()
	{
		return device;
	}

	VkDevice Device::getDevice() const
	{
		return device;
	}

	void Device::submitWork(std::vector<VkCommandBuffer>& cmdbuffer)
	{
		static int i = 0;

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		
		const VkSemaphore waitSemaphores[] = { bs::vk::imageAvailableSemaphores[i] };
		const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = cmdbuffer.data();

		submitInfo.signalSemaphoreCount = 1;

		const VkSemaphore* signalSemaphores = &bs::vk::renderFinishedSemaphores[i];
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(device, 1, &bs::vk::inFlightFences[i]);
		
		const VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, bs::vk::inFlightFences[i]);
		
		if (result != VK_SUCCESS) 
		{
			std::cout << "Queue Submission error: " << result << "\n";
			throw std::runtime_error("Failed to submit draw command buffer!");
		}
		
		vkQueueWaitIdle(graphicsQueue);
		
		if (bs::vk::inFlightFences[i] != VK_NULL_HANDLE) 
		{
			vkWaitForFences(device, 1, &bs::vk::inFlightFences[i], VK_TRUE, 500000000);
		}

		// goes from 0 to the buffer count for the swapchain buffers
		i = (i + 1) % bs::vk::NUM_SWAPCHAIN_FRAMEBUFFERS;
	}

	//NO GFX STUFF BC BREAKS AND SYNCH AND PAIN AND CRASHED DRIVERS
	void Device::submitImmediate(std::function<void(VkCommandBuffer cmd)>&& function)
	{
		VkCommandBuffer cmdbuffer;

		const VkCommandBufferAllocateInfo allocInfo
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = m_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};

		if (vkAllocateCommandBuffers(device, &allocInfo, &cmdbuffer) != VK_SUCCESS) 
		{
			throw std::runtime_error("Failed to allocate command buffers!");
		}

		constexpr VkCommandBufferBeginInfo beginInfo
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			.pInheritanceInfo = nullptr,
		};

		vkBeginCommandBuffer(cmdbuffer, &beginInfo);

		function(cmdbuffer);

		vkEndCommandBuffer(cmdbuffer);

		const VkSubmitInfo submitInfo
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,

			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = nullptr,

			.commandBufferCount = 1,
			.pCommandBuffers = &cmdbuffer,

			.signalSemaphoreCount = 0,
			.pSignalSemaphores = nullptr,
		};


		vkQueueWaitIdle(graphicsQueue);
		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkResetCommandPool(device, m_pool, 0);
	}

	VmaAllocator& Device::getAllocator()
	{
		return m_allocatorVMA;
	}

	VkPhysicalDevice& Device::getPhysicalDevice()
	{
		return physDevice; 
	}

	VkQueue Device::getPresentQueue()
	{
		return presentQueue;
	}

	VkQueue Device::getGraphicsQueue()
	{
		return graphicsQueue;
	}

	int Device::getScore(VkPhysicalDevice device) const
	{
		const QueueFamilyIndices indices = vk::findQueueFamilies(device);
		if(!indices.isComplete())
		{
			return -1;
		}

		//List of the extensions
		std::vector<VkExtensionProperties> availableExtensions;
		{	
			//Inside another score for idk why but I felt like it
			u32 extensionCount = 0;
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
			availableExtensions.resize(extensionCount);
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
		}

		//Go through the requirements for the needed extensions
		std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());
		for(const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		//Check the swapchain support
		if(requiredExtensions.empty())
		{
			SwapChainSupportDetails swapChainSupport = vk::querySwapChainSupport(device);
			const bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
			if(!swapChainAdequate)	//returns -1 if swapchain support is bad
			{
				return -1;
			}
		}
		else	// returns -1 if the required extensions aren't found
		{
			return -1;
		}

		//Counting the device score
		int score = 0;

		//Add score for the extensions
		const std::set<std::string> optionalExtensions(optionalDeviceExtensions.begin(), optionalDeviceExtensions.end());
		for(const auto& extension : availableExtensions)
		{
			if(optionalExtensions.contains(extension.extensionName))
			{
				score += 1;
			}
		}

		//Now for the device properties score
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			score += 5;
		}
		else if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		{
			//Integrated GPU
		}
		
		//Now for the features score
		VkPhysicalDeviceFeatures2	deviceFeatures;
		deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures.pNext = nullptr;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures.features);
		vkGetPhysicalDeviceFeatures2(device, &deviceFeatures);
		
		const auto& features = deviceFeatures.features;	//For every feature thing wanted, add to the score
		if(features.multiDrawIndirect == VK_TRUE)	{	score += 1;	}	//Multidraw indirect
		if(features.drawIndirectFirstInstance == VK_TRUE)	{	score += 1;	}	//Others
		if(features.shaderInt16 == VK_TRUE)	{	score += 1;	}
		if(features.variableMultisampleRate == VK_TRUE)	{	score += 1;	}

		return score;
	}

	std::vector<std::pair<VkPhysicalDevice, int>> Device::getPhysicalDevices() const
	{
		//Get device count
		u32 deviceCount = 0;
		vkEnumeratePhysicalDevices(vk::m_instance, &deviceCount, nullptr);

		if(deviceCount == 0) 
		{
			//this might cause cases where someone has a shitty GT 530 and it doesn't work with vulkan
			//eventually we might want to have an opengl backup just in case
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");
		}

		//Get list of physical devices
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(vk::m_instance, &deviceCount, devices.data());

		//The list of devices and scores
		std::vector<std::pair<VkPhysicalDevice, int>> device_score_list;

		for(const auto& device : devices) 
		{
			const auto deviceScore = getScore(device);
			if(deviceScore < 0)
			{
				continue;
			}

			device_score_list.emplace_back(std::make_pair(device, deviceScore));
		}

		if(device_score_list.empty()) 
		{
			throw std::runtime_error("Failed to find a suitable GPU!");
		}

		return device_score_list;
	}

	void Device::createDevice()
	{
		QueueFamilyIndices indices = vk::findQueueFamilies(physDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) 
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures2 deviceFeatures{};
		deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		vkGetPhysicalDeviceFeatures(physDevice, &deviceFeatures.features);
		vkGetPhysicalDeviceFeatures2(physDevice, &deviceFeatures);

		VkPhysicalDeviceVulkan12Features requestedFeatures = {};
		requestedFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		requestedFeatures.descriptorIndexing = VK_TRUE;
		requestedFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
		requestedFeatures.runtimeDescriptorArray = VK_TRUE;
		requestedFeatures.drawIndirectCount = VK_TRUE;
		
		/*requestedFeatures.shaderInputAttachmentArrayDynamicIndexing = VK_TRUE;
		requestedFeatures.shaderUniformTexelBufferArrayDynamicIndexing = VK_TRUE;
		requestedFeatures.shaderStorageTexelBufferArrayDynamicIndexing = VK_TRUE;
		requestedFeatures.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
		requestedFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
		requestedFeatures.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
		requestedFeatures.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
		requestedFeatures.shaderInputAttachmentArrayNonUniformIndexing = VK_TRUE;
		requestedFeatures.shaderUniformTexelBufferArrayNonUniformIndexing = VK_TRUE;
		requestedFeatures.shaderStorageTexelBufferArrayNonUniformIndexing = VK_TRUE;
		requestedFeatures.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
		requestedFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
		requestedFeatures.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
		requestedFeatures.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
		requestedFeatures.descriptorBindingUniformTexelBufferUpdateAfterBind = VK_TRUE;
		requestedFeatures.descriptorBindingStorageTexelBufferUpdateAfterBind = VK_TRUE;
		requestedFeatures.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
		requestedFeatures.descriptorBindingPartiallyBound = VK_TRUE;*/
		
		deviceFeatures.pNext = &requestedFeatures;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();

		createInfo.pEnabledFeatures = nullptr;
		createInfo.pNext = &deviceFeatures;

		if(vk::validationlayers) 
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(vk::validationLayers.size());
			createInfo.ppEnabledLayerNames = vk::validationLayers.data();
		}
		else 
		{
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physDevice, &createInfo, nullptr, &device) != VK_SUCCESS) 
		{
			throw std::runtime_error("Failed to create logical device!");
		}

		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

		//Output which device is chosen:
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physDevice, &properties);
		const std::string deviceName(properties.deviceName);
		u32 GPUID = properties.deviceID;

		std::cout << "Chose GPU #" << GPUID << "\t\t" << deviceName << std::endl;
	}
}