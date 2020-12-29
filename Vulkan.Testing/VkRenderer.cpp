#include "VkRenderer.h"
#include <iostream>
#include <set>

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

VkRenderer::~VkRenderer()
{
	vkDestroySwapchainKHR(m_VkDevice, m_VkSwapchainKHR, nullptr);
	vkDestroyDevice(m_VkDevice, nullptr);
	vkDestroySurfaceKHR(m_VkInstance, m_VkSurfaceKHR, nullptr);
	vkDestroyInstance(m_VkInstance, nullptr);
}

bool VkRenderer::Create()
{
	CreateVkInstance();
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapchain();

	std::cout << "Success\n";
	return true;
}

void VkRenderer::CreateVkInstance()
{
	// Get WSI extensions from SDL (we can add more if we like - we just can't remove these)
	unsigned extension_count;
	if (!SDL_Vulkan_GetInstanceExtensions(m_Window, &extension_count, NULL))
	{
		std::cout << "Could not get the number of required instance extensions from SDL." << std::endl;
	}

	m_InstanceExtensions.resize(extension_count);
	if (!SDL_Vulkan_GetInstanceExtensions(m_Window, &extension_count, m_InstanceExtensions.data()))
	{
		std::cout << "Could not get the names of required instance extensions from SDL." << std::endl;
	}

	m_InstanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	m_InstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	// Use validation layers if this is a debug build
#if defined(_DEBUG)
	m_ValidationLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif

	// Create instance
	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Vulkan";
	app_info.applicationVersion = 1;
	app_info.pEngineName = "Vulkan";
	app_info.engineVersion = 1;
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo inst_info = {};
	inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	inst_info.pApplicationInfo = &app_info;
	inst_info.enabledLayerCount = (uint32_t)m_ValidationLayers.size();
	inst_info.ppEnabledLayerNames = m_ValidationLayers.size() ? m_ValidationLayers.data() : NULL;
	inst_info.enabledExtensionCount = (uint32_t)m_InstanceExtensions.size();
	inst_info.ppEnabledExtensionNames = m_InstanceExtensions.data();

	Vk::Check(vkCreateInstance(&inst_info, nullptr, &m_VkInstance));

	// Create surface
	if (!SDL_Vulkan_CreateSurface(m_Window, m_VkInstance, &m_VkSurfaceKHR))
	{
		std::cout << "Could not create a Vulkan surface." << std::endl;
	}
}

void VkRenderer::PickPhysicalDevice()
{
	// Get all physical devices
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, nullptr);

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, devices.data());

	// Set physical device to default
	m_VkPhysicalDevice = devices[0];

	// Device properties
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(m_VkPhysicalDevice, &deviceProperties);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(m_VkPhysicalDevice, &deviceFeatures);

	// Print GPU name
	std::cout << deviceProperties.deviceName << '\n';

	// Queue families
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_VkPhysicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_VkPhysicalDevice, &queueFamilyCount, queueFamilies.data());

	// Find graphic queue for graphics
	for (size_t i = 0; i < queueFamilies.size(); i++)
	{
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			m_GraphicsFamily = (uint32_t)i;
			break;
		}
	}

	if (!m_GraphicsFamily.has_value())
	{
		std::cout << "No graphics queue found\n";
	}

	VkBool32 presentSupport = false;
	Vk::Check(vkGetPhysicalDeviceSurfaceSupportKHR(m_VkPhysicalDevice, m_GraphicsFamily.value(), m_VkSurfaceKHR, &presentSupport));
	if (presentSupport)
	{
		m_PresentFamily = m_GraphicsFamily;
	}
}

void VkRenderer::CreateLogicalDevice()
{
	// Specifying the queues to be created
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { m_GraphicsFamily.value(), m_PresentFamily.value() };

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

	// Device extensions
	const std::vector<const char*> deviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(m_VkPhysicalDevice, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(m_VkPhysicalDevice, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		if (!requiredExtensions.empty())
		{
			std::cout << "VK_KHR_SWAPCHAIN_EXTENSION_NAME" << " - not supported\n";
		}
	}

	// Specifying device features
	VkPhysicalDeviceFeatures deviceFeatures{};

	// Creating the logical device
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
	createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

	Vk::Check(vkCreateDevice(m_VkPhysicalDevice, &createInfo, nullptr, &m_VkDevice));

	vkGetDeviceQueue(m_VkDevice, m_GraphicsFamily.value(), 0, &m_VkPresentQueue);
}

void VkRenderer::CreateSwapchain()
{
	Vk::Check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_VkPhysicalDevice, m_VkSurfaceKHR, &m_Capabilities));

	// Get supported formats
	uint32_t formatCount;
	Vk::Check(vkGetPhysicalDeviceSurfaceFormatsKHR(m_VkPhysicalDevice, m_VkSurfaceKHR, &formatCount, nullptr));

	m_Formats.resize(formatCount);
	Vk::Check(vkGetPhysicalDeviceSurfaceFormatsKHR(m_VkPhysicalDevice, m_VkSurfaceKHR, &formatCount, m_Formats.data()));

	VkSurfaceFormatKHR format;
	format = m_Formats[0];
	for (const auto& availableFormat : m_Formats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
		{
			format = availableFormat;
			break;
		}
	}

	// Get present mode counts
	uint32_t presentModeCount;
	Vk::Check(vkGetPhysicalDeviceSurfacePresentModesKHR(m_VkPhysicalDevice, m_VkSurfaceKHR, &presentModeCount, nullptr));

	m_PresentModes.resize(presentModeCount);
	Vk::Check(vkGetPhysicalDeviceSurfacePresentModesKHR(m_VkPhysicalDevice, m_VkSurfaceKHR, &presentModeCount, m_PresentModes.data()));

	VkPresentModeKHR presentMode;
	presentMode = m_PresentModes[0];

	// SDL THING
	auto width = 0, height = 0;
	SDL_Vulkan_GetDrawableSize(m_Window, &width, &height);

	width = std::clamp((uint32_t)width, m_Capabilities.minImageExtent.width, m_Capabilities.maxImageExtent.width);
	height = std::clamp((uint32_t)height, m_Capabilities.minImageExtent.height, m_Capabilities.maxImageExtent.height);

	VkExtent2D extent =
	{
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
	};

	// Create the swapchain
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_VkSurfaceKHR;

	createInfo.minImageCount = m_Capabilities.minImageCount + 1;
	createInfo.imageFormat = format.format;
	createInfo.imageColorSpace = format.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = { m_GraphicsFamily.value(), m_PresentFamily.value() };
	if (m_GraphicsFamily != m_PresentFamily) 
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else 
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = m_Capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	Vk::Check(vkCreateSwapchainKHR(m_VkDevice, &createInfo, nullptr, &m_VkSwapchainKHR));
}
