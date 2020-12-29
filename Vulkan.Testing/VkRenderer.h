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

	VkExtent2D m_Extent;

	// Image view
	std::vector<VkImageView> m_SwapChainImageViews;
	VkImageView mVkImageView;
	void CreateImageViews();

	// Pipeline
	void CreateGraphicsPipeline();
	VkShaderModule createShaderModule(const std::vector<char>& code);
	VkRenderPass m_RenderPass;
	VkPipelineLayout m_PipelineLayout;
	void CreateRenderPass();

	VkPipeline m_VkPipeline;
};