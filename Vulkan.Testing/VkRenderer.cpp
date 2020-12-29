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

VkRenderer::~VkRenderer()
{
	vkDestroyDevice(m_VkDevice, nullptr);
	vkDestroySurfaceKHR(m_VkInstance, m_VkSurfaceKHR, nullptr);
	vkDestroyInstance(m_VkInstance, nullptr);
}

bool VkRenderer::Create()
{
	CreateVkInstance();
	PickPhysicalDevice();
	CreateLogicalDevice();

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
}

void VkRenderer::CreateLogicalDevice()
{
	// Specifying the queues to be created
	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = m_GraphicsFamily.value();
	queueCreateInfo.queueCount = 1;

	float queuePriority = 1.0f;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	// Specifying device features
	VkPhysicalDeviceFeatures deviceFeatures{};

	// Creating the logical device
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.queueCreateInfoCount = 1;
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = 0;

	createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
	createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

	Vk::Check(vkCreateDevice(m_VkPhysicalDevice, &createInfo, nullptr, &m_VkDevice));

	// Get queue
	vkGetDeviceQueue(m_VkDevice, m_GraphicsFamily.value(), 0, & m_VkQueue);
}
