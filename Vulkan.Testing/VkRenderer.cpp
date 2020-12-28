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
	CreateInstanceAndSurface();
	QueryPhysicalDevices();
	CreateDevice();
	CreateCommandBuffer();

	return true;
}

bool VkRenderer::CreateInstanceAndSurface()
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

	// Create the Vulkan surface
	if (!SDL_Vulkan_CreateSurface(m_Window, m_VkInstance, &m_VkSurface))
	{
		std::cerr << "SDL_Vulkan_CreateSurface failed\n";
		return false;
	}

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
	VkDeviceCreateInfo device_info = {};
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pNext = nullptr;
	device_info.queueCreateInfoCount = 1;
	device_info.pQueueCreateInfos = &queue_info;
	device_info.enabledExtensionCount = 0;
	device_info.ppEnabledExtensionNames = nullptr;
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
	cmd.pNext = NULL;
	cmd.commandPool = m_VkCommandPool;
	cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd.commandBufferCount = 1;

	Vk::Check(vkAllocateCommandBuffers(m_VkDevice, &cmd, &m_VkCommandBuffer));
}
