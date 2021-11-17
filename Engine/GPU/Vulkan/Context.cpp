#include "Context.h"

#include <imgui.h>
#include "VulkanHelpers.h"
#include <GLFW/glfw3.h>


namespace bs
{
	Context::Context(const std::string& title)	: m_windowname(title)
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_window = glfwCreateWindow(bs::vk::viewportwidth, bs::vk::viewportheight, m_windowname.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(m_window, this);
	}

	void Context::setIcon(Image& icon)
	{
		//Make the Window Icon Something
		const GLFWimage img
		{
			.width = icon.getSize().x,
			.height = icon.getSize().y,
			.pixels = (unsigned char*)icon.getPixelsPtr()
		};

		glfwSetWindowIcon(getContext(), 1, &img);	
	}

	void Context::clear()
	{
		glfwPollEvents();
		// Acquire the INDEX into the swapchain for the next image
		VkResult result = vkAcquireNextImageKHR(m_Device->getDevice(), m_swapchain, UINT64_MAX, bs::vk::imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) 
		{
			recreateSwapchain();
			refresh = true;
		} 
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to acquire swap chain image!");
		}
	}

	void Context::update()
	{
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.pResults = nullptr;

		// Waits to present until the "render finished" semaphore (signal) is "signaled" (when the render is done, the semaphore is triggered by)
		VkSemaphore signalSemaphore = bs::vk::renderFinishedSemaphores[currentFrame];
		presentInfo.pWaitSemaphores = &signalSemaphore;
		presentInfo.waitSemaphoreCount = 1;
		//Pass swapchain
		presentInfo.pSwapchains = &m_swapchain;
		presentInfo.swapchainCount = 1;

		presentInfo.pImageIndices = &imageIndex;

		// Submit Image that just finished rendering to the presentation surface
		VkResult result = vkQueuePresentKHR(m_Device->getPresentQueue(), &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) 
		{
			recreateSwapchain();
			refresh = true;
		} 
		else if (result != VK_SUCCESS) 
		{
			throw std::runtime_error("Failed to present swap chain image!");
		}

		// Increase Frame Index
		currentFrame = (currentFrame + 1) % bs::vk::NUM_SWAPCHAIN_FRAMEBUFFERS;
	}

	void Context::close()
	{
		
	}

	void Context::initAPI()
	{
		bs::vk::createInstance("Bathsalts");
		bs::vk::createSurface(m_window);
		m_Device->init();
		bs::vk::createSwapChain(m_swapchain, *m_Device, m_scdetails, m_window);
		bs::vk::createImageViews(m_scdetails, m_Device->getDevice());

		//INIT SYNCH PRIMITIVES
		bs::vk::imageAvailableSemaphores.resize(bs::vk::NUM_SWAPCHAIN_FRAMEBUFFERS);
		bs::vk::renderFinishedSemaphores.resize(bs::vk::NUM_SWAPCHAIN_FRAMEBUFFERS);
		bs::vk::inFlightFences.resize(bs::vk::NUM_SWAPCHAIN_FRAMEBUFFERS);
		bs::vk::imagesInFlight.resize(m_scdetails.swapChainImages.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < bs::vk::NUM_SWAPCHAIN_FRAMEBUFFERS; i++) 
		{
			if (vkCreateSemaphore(m_Device->getDevice(), &semaphoreInfo, nullptr, &bs::vk::imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(m_Device->getDevice(), &semaphoreInfo, nullptr, &bs::vk::renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(m_Device->getDevice(), &fenceInfo, nullptr, &bs::vk::inFlightFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create synchronization objects for a frame!");
			}
		}

		//Start IMGUI init
		IMGUI_CHECKVERSION();
		auto* ctximgui = ImGui::CreateContext();
		ImGui::SetCurrentContext(ctximgui);

		ImGuiIO& io = ImGui::GetIO();

		io.DisplaySize = ImVec2(static_cast<float>(bs::vk::viewportwidth), static_cast<float>(bs::vk::viewportheight));
		io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
		
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		
		io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
		io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
		io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
		io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
		io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
		io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
		io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;

		//Setup style
		ImGui::StyleColorsDark();
	}

	bool Context::isOpen()
	{
		if (glfwWindowShouldClose(m_window))
		{
			return false;
		}
		return true;
	}

	GLFWwindow* Context::getContext()
	{
		return m_window;
	}

	void Context::setDeviceptr(Device* pdevice)
	{
		m_Device = pdevice;
	}

	Context::~Context()
	{
		vmaDestroyAllocator(m_Device->getAllocator());

		//Destroy Sync Primitives
		for (auto& fence : bs::vk::inFlightFences)
		{
			vkDestroyFence(m_Device->getDevice(), fence, nullptr);
		}
		for (auto& semaphore : bs::vk::renderFinishedSemaphores) 
		{
			vkDestroySemaphore(m_Device->getDevice(), semaphore, nullptr);
		}
		for (auto& semaphore : bs::vk::imageAvailableSemaphores) 
		{
			vkDestroySemaphore(m_Device->getDevice(), semaphore, nullptr);
		}

		//Destroy swapchain framebuffers & image views
		for (int i = 0; i < m_scdetails.swapChainFramebuffers.size(); ++i) 
		{
			vkDestroyFramebuffer(m_Device->getDevice(), m_scdetails.swapChainFramebuffers[i], nullptr);
		}

		for (int i = 0; i < m_scdetails.swapChainImageViews.size(); ++i)
		{
			vkDestroyImageView(m_Device->getDevice(), m_scdetails.swapChainImageViews[i], nullptr);
		}
		//Destroy swapchain and render surface
		vkDestroySwapchainKHR(m_Device->getDevice(), m_swapchain, nullptr);
		vkDestroySurfaceKHR(bs::vk::m_instance, bs::vk::m_surface, nullptr);

		m_Device->destroy();

		vkDestroyInstance(bs::vk::m_instance, nullptr);
		glfwDestroyWindow(m_window);
		glfwTerminate();
	}

	void Context::recreateSwapchain()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_window, &width, &height);
		while (width == 0 || height == 0) 
		{
			glfwGetFramebufferSize(m_window, &width, &height);
			glfwWaitEvents();
		}
		
		bs::vk::viewportwidth = width;
		bs::vk::viewportheight = height;

		vkDeviceWaitIdle(m_Device->getDevice());

		//Cleanup swapchain + stuff
		for (int i = 0; i < m_scdetails.swapChainFramebuffers.size(); ++i) 
		{
			vkDestroyFramebuffer(m_Device->getDevice(), m_scdetails.swapChainFramebuffers[i], nullptr);
		}

		for (int i = 0; i < m_scdetails.swapChainImageViews.size(); ++i)
		{
			vkDestroyImageView(m_Device->getDevice(), m_scdetails.swapChainImageViews[i], nullptr);
		}
		

		vkDestroySwapchainKHR(m_Device->getDevice(), m_swapchain, nullptr);

		//Recreate swapchain + stuff
		bs::vk::createSwapChain(m_swapchain, *m_Device, m_scdetails, m_window);
		bs::vk::createImageViews(m_scdetails, m_Device->getDevice());
		bs::vk::createFramebuffers(*rpass, m_scdetails, m_Device->getDevice());

		resized = false;
		
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(static_cast<float>(bs::vk::viewportwidth), static_cast<float>(bs::vk::viewportheight));
	}

	void resizeCallback(GLFWwindow* window, int width, int height)
	{
		auto context = reinterpret_cast<Context*>(glfwGetWindowUserPointer(window));
		context->resized = true;
	}
}