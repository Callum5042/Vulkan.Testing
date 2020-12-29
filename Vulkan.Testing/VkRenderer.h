#pragma once

#include <vector>
#include <optional>

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

	// Validation layer
	std::vector<const char*> m_ValidationLayers;

	// Vulkan instance
	std::vector<const char*> m_InstanceExtensions;
	VkInstance m_VkInstance;
	VkSurfaceKHR m_VkSurfaceKHR;
	void CreateVkInstance();

	// Vulkan physical device
	VkPhysicalDevice m_VkPhysicalDevice;
	std::optional<uint32_t> m_GraphicsFamily;
	std::optional<uint32_t> m_PresentFamily;
	void PickPhysicalDevice();

	// Vulkan device
	VkDevice m_VkDevice;
	VkQueue m_VkPresentQueue;
	void CreateLogicalDevice();

	// Swapchain
	VkSurfaceCapabilitiesKHR m_Capabilities;
	std::vector<VkSurfaceFormatKHR> m_Formats;
	std::vector<VkPresentModeKHR> m_PresentModes;
	VkPresentModeKHR m_PresentMode;
	VkSurfaceFormatKHR m_Format;
	VkSwapchainKHR m_VkSwapchainKHR;
	std::vector<VkImage> m_SwapChainImages;
	void CreateSwapchain();

	// Image view
	std::vector<VkImageView> m_SwapChainImageViews;
	VkImageView mVkImageView;
	void CreateImageViews();
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