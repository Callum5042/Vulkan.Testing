#include "VkRenderer.h"
#include <iostream>

namespace Vk
{
	inline void Check(VkResult result)
	{
		if (result != VkResult::VK_SUCCESS)
		{
			throw std::exception();
		}
	}
}

VkRenderer::VkRenderer(SDL_Window* window) : m_Window(window)
{
}

bool VkRenderer::Create()
{
	CreateInstance();
	QueryPhysicalDevices();
	CreateDevice();
	CreateCommandBuffer();
	CreateSwapchain();

	return true;
}

bool VkRenderer::CreateInstance()
{
	// Get the extension count
	auto extension_count = 0u;
	if (!SDL_Vulkan_GetInstanceExtensions(m_Window, &extension_count, nullptr))
	{
		std::cerr << "SDL_Vulkan_GetInstanceExtensions failed\n";
		return false;
	}

	// Create a vector to hold the extensions
	std::vector<const char*> extensions = { VK_EXT_DEBUG_REPORT_EXTENSION_NAME };
	auto additional_extension_count = extensions.size();
	extensions.resize(additional_extension_count + extension_count);
	if (!SDL_Vulkan_GetInstanceExtensions(m_Window, &extension_count, extensions.data() + additional_extension_count))
	{
		std::cerr << "SDL_Vulkan_GetInstanceExtensions failed\n";
		return false;
	}

	// Create the Vulkan instance
	VkInstanceCreateInfo create_info = {};
	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	create_info.ppEnabledExtensionNames = extensions.data();

	Vk::Check(vkCreateInstance(&create_info, nullptr, &m_VkInstance));
	return true;
}

void VkRenderer::QueryPhysicalDevices()
{
	auto gpu_count = 0u;
	Vk::Check(vkEnumeratePhysicalDevices(m_VkInstance, &gpu_count, nullptr));

	m_PhysicalDevices.resize(gpu_count);
	Vk::Check(vkEnumeratePhysicalDevices(m_VkInstance, &gpu_count, m_PhysicalDevices.data()));
}

void VkRenderer::CreateDevice()
{
	// Query queue family
	auto queue_family_count = 0u;
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevices[0], &queue_family_count, nullptr);

	m_QueueFamilyProperties.resize(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevices[0], &queue_family_count, m_QueueFamilyProperties.data());

	// Look for graphics queue
	auto found = false;
	VkDeviceQueueCreateInfo queue_info = {};
	for (auto i = 0u; i < queue_family_count; i++) 
	{
		if (m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			queue_info.queueFamilyIndex = i;
			m_QueueFamilyIndex = queue_info.queueFamilyIndex;
			found = true;
			break;
		}
	}

	float queue_priorities[1] = { 0.0 };
	queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_info.pNext = NULL;
	queue_info.queueCount = 1;
	queue_info.pQueuePriorities = queue_priorities;

	// Create device
	std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkDeviceCreateInfo device_info = {};
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pNext = nullptr;
	device_info.queueCreateInfoCount = 1;
	device_info.pQueueCreateInfos = &queue_info;
	device_info.enabledExtensionCount = device_extensions.size();
	device_info.ppEnabledExtensionNames = device_extensions.data();
	device_info.enabledLayerCount = 0;
	device_info.ppEnabledLayerNames = nullptr;
	device_info.pEnabledFeatures = nullptr;

	Vk::Check(vkCreateDevice(m_PhysicalDevices[0], &device_info, nullptr, &m_VkDevice));
}

void VkRenderer::CreateCommandBuffer()
{
	// Create command pool
	VkCommandPoolCreateInfo cmd_pool_info = {};
	cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmd_pool_info.pNext = nullptr;
	cmd_pool_info.queueFamilyIndex = m_QueueFamilyIndex;
	cmd_pool_info.flags = 0;

	Vk::Check(vkCreateCommandPool(m_VkDevice, &cmd_pool_info, nullptr, &m_VkCommandPool));

	// Allocate command buffers
	VkCommandBufferAllocateInfo cmd = {};
	cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd.pNext = nullptr;
	cmd.commandPool = m_VkCommandPool;
	cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd.commandBufferCount = 1;

	Vk::Check(vkAllocateCommandBuffers(m_VkDevice, &cmd, &m_VkCommandBuffer));
}

void VkRenderer::CreateSwapchain()
{
	// Create the Vulkan surface
	if (!SDL_Vulkan_CreateSurface(m_Window, m_VkInstance, &m_VkSurface))
	{
		std::cerr << "SDL_Vulkan_CreateSurface failed\n";
		throw std::exception();
	}

	// Get surface capailities
	VkSurfaceCapabilitiesKHR surface_capabilities;
	Vk::Check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevices[0], m_VkSurface, &surface_capabilities));

	auto width = 0;
	auto height = 0;
	SDL_Vulkan_GetDrawableSize(m_Window, &width, &height);

	width = std::clamp(static_cast<uint32_t>(width), surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
	height = std::clamp(static_cast<uint32_t>(height), surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);

	// Query present modes
	auto present_mode_counts = 0u;
	Vk::Check(vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevices[0], m_VkSurface, &present_mode_counts, nullptr));

	std::vector<VkPresentModeKHR> present_modes(present_mode_counts);
	Vk::Check(vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevices[0], m_VkSurface, &present_mode_counts, present_modes.data()));

	// Create swapchain
	VkSwapchainCreateInfoKHR swapchain_ci = {};
	/*swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_ci.pNext = nullptr;
	swapchain_ci.surface = m_VkSurface;
	swapchain_ci.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
	swapchain_ci.imageExtent.width = width;
	swapchain_ci.imageExtent.height = height;
	swapchain_ci.minImageCount = surface_capabilities.minImageCount;
	swapchain_ci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchain_ci.presentMode = VK_PRESENT_MODE_FIFO_KHR;

	swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_ci.queueFamilyIndexCount = 0;
	swapchain_ci.pQueueFamilyIndices = NULL;*/

	VkSurfaceTransformFlagBitsKHR preTransform;
	if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else 
	{
		preTransform = surface_capabilities.currentTransform;
	}

	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

	// Find a supported composite alpha mode - one of these is guaranteed to be set
	VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[4] = 
	{
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
	};

	for (uint32_t i = 0; i < sizeof(compositeAlphaFlags) / sizeof(compositeAlphaFlags[0]); i++) 
	{
		if (surface_capabilities.supportedCompositeAlpha & compositeAlphaFlags[i]) {
			compositeAlpha = compositeAlphaFlags[i];
			break;
		}
	}

	// Get the list of VkFormats that are supported:
	uint32_t formatCount;
	Vk::Check(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevices[0], m_VkSurface, &formatCount, NULL));

	std::vector< VkSurfaceFormatKHR> surface_formats(formatCount);
	Vk::Check(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevices[0], m_VkSurface, &formatCount, surface_formats.data()));

	// If the format list includes just one entry of VK_FORMAT_UNDEFINED,
	// the surface has no preferred format.  Otherwise, at least one
	// supported format will be returned.
	VkFormat format;
	if (formatCount == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED)
	{
		format = VK_FORMAT_B8G8R8A8_UNORM;
	}
	else 
	{
		assert(formatCount >= 1);
		format = surface_formats[0].format;
	}

	swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_ci.pNext = NULL;
	swapchain_ci.surface = m_VkSurface;
	swapchain_ci.minImageCount = surface_capabilities.minImageCount;
	swapchain_ci.imageFormat = format;
	swapchain_ci.imageExtent.width = width;
	swapchain_ci.imageExtent.height = height;
	swapchain_ci.preTransform = preTransform;
	swapchain_ci.compositeAlpha = compositeAlpha;
	swapchain_ci.imageArrayLayers = 1;
	swapchain_ci.presentMode = swapchainPresentMode;
	swapchain_ci.oldSwapchain = VK_NULL_HANDLE;
	swapchain_ci.clipped = true;
	swapchain_ci.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_ci.queueFamilyIndexCount = 0;
	swapchain_ci.pQueueFamilyIndices = NULL;


	uint32_t queueFamilyIndices[2] = 
	{
		(uint32_t)m_QueueFamilyIndex,
		(uint32_t)m_QueueFamilyIndex //info.present_queue_family_index
	};

	if (m_QueueFamilyIndex != m_QueueFamilyIndex)
	{
		// If the graphics and present queues are from different queue families,
		// we either have to explicitly transfer ownership of images between the
		// queues, or we have to create the swapchain with imageSharingMode
		// as VK_SHARING_MODE_CONCURRENT
		swapchain_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchain_ci.queueFamilyIndexCount = 2;
		swapchain_ci.pQueueFamilyIndices = queueFamilyIndices;
	}

	Vk::Check(vkCreateSwapchainKHR(m_VkDevice, &swapchain_ci, nullptr, &m_VkSwapchain));
}
