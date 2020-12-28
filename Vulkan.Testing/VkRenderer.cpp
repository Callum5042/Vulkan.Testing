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
	//CreateDepthBuffer();

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

	// Validation layer
	const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };


	// Create the Vulkan instance
	VkInstanceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	create_info.ppEnabledExtensionNames = extensions.data();

	create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	create_info.ppEnabledLayerNames = validationLayers.data();

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
	device_info.enabledExtensionCount = (uint32_t)device_extensions.size();
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

	// Create image view
	auto swapchain_image_count = 0u;
	Vk::Check(vkGetSwapchainImagesKHR(m_VkDevice, m_VkSwapchain, &swapchain_image_count, NULL));

	std::vector<VkImage> images(swapchain_image_count);
	Vk::Check(vkGetSwapchainImagesKHR(m_VkDevice, m_VkSwapchain, &swapchain_image_count, images.data()));

	m_VkImageViews.resize(swapchain_image_count);
	for (uint32_t i = 0; i < swapchain_image_count; i++)
	{
		VkImageViewCreateInfo color_image_view = {};
		color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		color_image_view.pNext = NULL;
		color_image_view.flags = 0;
		color_image_view.image = images[i];
		color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		color_image_view.format = format;
		color_image_view.components.r = VK_COMPONENT_SWIZZLE_R;
		color_image_view.components.g = VK_COMPONENT_SWIZZLE_G;
		color_image_view.components.b = VK_COMPONENT_SWIZZLE_B;
		color_image_view.components.a = VK_COMPONENT_SWIZZLE_A;
		color_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		color_image_view.subresourceRange.baseMipLevel = 0;
		color_image_view.subresourceRange.levelCount = 1;
		color_image_view.subresourceRange.baseArrayLayer = 0;
		color_image_view.subresourceRange.layerCount = 1;

		Vk::Check(vkCreateImageView(m_VkDevice, &color_image_view, NULL, &m_VkImageViews[i]));
	}
}

void VkRenderer::CreateDepthBuffer()
{
	VkImageCreateInfo image_info = {};
	const VkFormat depth_format = VK_FORMAT_D16_UNORM;
	VkFormatProperties props;

	vkGetPhysicalDeviceFormatProperties(m_PhysicalDevices[0], depth_format, &props);
	if (props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) 
	{
		image_info.tiling = VK_IMAGE_TILING_LINEAR;
	}
	else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) 
	{
		image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	}
	else 
	{
		/* Try other depth formats? */
		std::cout << "VK_FORMAT_D16_UNORM Unsupported.\n";
		exit(-1);
	}

	// Get surface capailities
	VkSurfaceCapabilitiesKHR surface_capabilities;
	Vk::Check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevices[0], m_VkSurface, &surface_capabilities));

	auto width = 0;
	auto height = 0;
	SDL_Vulkan_GetDrawableSize(m_Window, &width, &height);

	width = std::clamp(static_cast<uint32_t>(width), surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
	height = std::clamp(static_cast<uint32_t>(height), surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);

	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.pNext = NULL;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.format = depth_format;
	image_info.extent.width = width;
	image_info.extent.height = height;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	image_info.queueFamilyIndexCount = 0;
	image_info.pQueueFamilyIndices = NULL;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.flags = 0;

	VkMemoryAllocateInfo mem_alloc = {};
	mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_alloc.pNext = NULL;
	mem_alloc.allocationSize = 0;
	mem_alloc.memoryTypeIndex = 0;

	VkImageViewCreateInfo view_info = {};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.pNext = NULL;
	view_info.image = VK_NULL_HANDLE;
	view_info.format = depth_format;
	view_info.components.r = VK_COMPONENT_SWIZZLE_R;
	view_info.components.g = VK_COMPONENT_SWIZZLE_G;
	view_info.components.b = VK_COMPONENT_SWIZZLE_B;
	view_info.components.a = VK_COMPONENT_SWIZZLE_A;
	view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = 1;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_info.flags = 0;


	/* Create image */
	VkImage depth_image;
	Vk::Check(vkCreateImage(m_VkDevice, &image_info, NULL, &depth_image));

	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(m_VkDevice, depth_image, &mem_reqs);

	mem_alloc.allocationSize = mem_reqs.size;

	/* Use the memory properties to determine the type of memory required */
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevices[0], &memory_properties);

	VkPhysicalDeviceProperties gpu_props;
	vkGetPhysicalDeviceProperties(m_PhysicalDevices[0], &gpu_props);

	auto memory_type_index = 0;
	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) 
	{
		if ((mem_reqs.memoryTypeBits & 1) == 1)
		{
			// Type is available, does it match user properties?
			if ((memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			{
				memory_type_index = i;
				break;
			}
		}

		mem_reqs.memoryTypeBits >>= 1;
	}

	/* Allocate memory */
	VkDeviceMemory memory;
	Vk::Check(vkAllocateMemory(m_VkDevice, &mem_alloc, NULL, &memory));

	/* Bind memory */
	Vk::Check(vkBindImageMemory(m_VkDevice, depth_image, memory, 0));

	/* Create image view */
	view_info.image = depth_image;

	VkImageView m_VkImageView;
	Vk::Check(vkCreateImageView(m_VkDevice, &view_info, NULL, &m_VkImageView));
}
