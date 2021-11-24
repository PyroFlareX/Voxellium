#pragma once

#include "VulkanHelpers.h"


namespace bs
{
	class Device
	{
	public:
		Device();
		~Device();

		//Init device
		void init();
		//destroy
		void destroy();

		QueueFamilyIndices getQueueFamilies();
		SwapChainSupportDetails getSwapchainDetails();
		VkDevice& getDevice();
		VkDevice getDevice() const;
		VkPhysicalDevice& getPhysicalDevice();

		//Submit GFX work
		void submitWork(std::vector<VkCommandBuffer>& cmdbuffer);
		//Submit Data/Cmd buffer to GPU
		void submitImmediate(std::function<void(VkCommandBuffer cmd)>&& function);

		//Get VMA Allocator
		VmaAllocator& getAllocator();

		VkQueue getPresentQueue();
		VkQueue getGraphicsQueue();


	private:
		int getScore(VkPhysicalDevice device) const;

		std::vector<std::pair<VkPhysicalDevice, int>> getPhysicalDevices() const;

		void createDevice();

		VkPhysicalDevice physDevice;
		VkDevice device;

		VkQueue graphicsQueue;
		VkQueue presentQueue;

		VmaAllocator m_allocatorVMA;
		VkCommandPool m_pool;

		std::vector<const char*> requiredDeviceExtensions;
		std::vector<const char*> optionalDeviceExtensions;

		bool destroyed;
	};

}