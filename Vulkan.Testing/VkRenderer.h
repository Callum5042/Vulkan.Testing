#pragma once

#include <vector>

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.hpp>
typedef unsigned int uint;

class VkRenderer
{
public:
	VkRenderer(SDL_Window* window);
	virtual ~VkRenderer();

	bool Create();

private:
	SDL_Window* m_Window = nullptr;

	// Vulkan instance
	std::vector<const char*> m_InstanceExtensions;
	VkInstance m_VkInstance;
	VkSurfaceKHR m_VkSurfaceKHR;
	void CreateVkInstance();


};

//class VkRenderer
//{
//public:
//	VkRenderer(SDL_Window* window);
//
//	bool Create();
//
//private:
//	SDL_Window* m_Window = nullptr;
//
//	// Vulkan instance
//	VkInstance m_VkInstance;
//	void CreateInstance();
//
//	// Physical devices
//	std::vector<VkPhysicalDevice> m_PhysicalDevices;
//	void QueryPhysicalDevices();
//
//	// Logical device
//	std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties;
//	uint m_QueueFamilyIndex = 0;
//	VkDevice m_VkDevice;
//	void CreateDevice();
//
//	// Command buffer
//	VkCommandPool m_VkCommandPool;
//	VkCommandBuffer m_VkCommandBuffer;
//	void CreateCommandBuffer();
//
//	// Swapchain
//	VkSurfaceKHR m_VkSurface;
//	VkSwapchainKHR m_VkSwapchain;
//	std::vector<VkImageView> m_VkImageViews;
//	void CreateSwapchain();
//
//	// Depth buffer
//	void CreateDepthBuffer();
//};