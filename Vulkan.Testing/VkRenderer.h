#pragma once

#include <vector>

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

class VkRenderer
{
public:
	VkRenderer(SDL_Window* window);

	bool Create();

private:
	SDL_Window* m_Window = nullptr;

	// Vulkan instance
	VkInstance m_VkInstance;
	VkSurfaceKHR m_VkSurface;
	bool CreateInstanceAndSurface();

	// Physical devices
	std::vector<VkPhysicalDevice> m_PhysicalDevices;
	void QueryPhysicalDevices();

	// Logical device
	std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties;
	VkDevice m_VkDevice;
	void CreateDevice();
};