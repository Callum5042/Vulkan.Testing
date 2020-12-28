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
	vkDestroySurfaceKHR(m_VkInstance, m_VkSurfaceKHR, nullptr);
	vkDestroyInstance(m_VkInstance, nullptr);
}

bool VkRenderer::Create()
{
	CreateVkInstance();
	PickPhysicalDevice();

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
	std::vector<const char*> validationLayers;
#if defined(_DEBUG)
	validationLayers.push_back("VK_LAYER_KHRONOS_validation");
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
	inst_info.enabledLayerCount = (uint32_t)validationLayers.size();
	inst_info.ppEnabledLayerNames = validationLayers.size() ? validationLayers.data() : NULL;
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
}
