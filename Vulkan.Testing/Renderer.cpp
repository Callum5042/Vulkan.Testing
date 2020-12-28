#include "Renderer.h"
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

Renderer::Renderer(SDL_Window* window) : m_Window(window)
{
}

bool Renderer::Create()
{
	CreateVkInstance();

	return true;
}

bool Renderer::CreateVkInstance()
{
	// Get the extension count
	unsigned int extension_count = 0;
	if (!SDL_Vulkan_GetInstanceExtensions(m_Window, &extension_count, nullptr))
	{
		std::cerr << "SDL_Vulkan_GetInstanceExtensions failed\n";
		return false;
	}

	// Create a vector to hold the extensions
	std::vector<const char*> extensions = { VK_EXT_DEBUG_REPORT_EXTENSION_NAME };
	size_t additional_extension_count = extensions.size();
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