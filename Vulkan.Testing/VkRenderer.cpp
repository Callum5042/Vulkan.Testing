#include "VkRenderer.h"
#include <iostream>
#include <set>
#include <fstream>

namespace Vk
{
	inline void Check(VkResult result)
	{
		if (result != VkResult::VK_SUCCESS)
		{
			throw std::exception();
		}
	}

	static std::vector<char> readFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			throw std::runtime_error("failed to open file!");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();
		return buffer;
	}
}

VkRenderer::VkRenderer(SDL_Window* window) : m_Window(window)
{
}

VkRenderer::~VkRenderer()
{
	vkDestroyPipelineLayout(m_VkDevice, m_PipelineLayout, nullptr);
	vkDestroyRenderPass(m_VkDevice, m_RenderPass, nullptr);
	for (auto imageView : m_SwapChainImageViews)
	{
		vkDestroyImageView(m_VkDevice, imageView, nullptr);
	}

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
	CreateImageViews();
	CreateRenderPass();
	CreateGraphicsPipeline();

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

	m_Format = m_Formats[0];
	for (const auto& availableFormat : m_Formats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			m_Format = availableFormat;
			break;
		}
	}

	// Get present mode counts
	uint32_t presentModeCount;
	Vk::Check(vkGetPhysicalDeviceSurfacePresentModesKHR(m_VkPhysicalDevice, m_VkSurfaceKHR, &presentModeCount, nullptr));

	m_PresentModes.resize(presentModeCount);
	Vk::Check(vkGetPhysicalDeviceSurfacePresentModesKHR(m_VkPhysicalDevice, m_VkSurfaceKHR, &presentModeCount, m_PresentModes.data()));

	m_PresentMode = m_PresentModes[0];

	// SDL THING
	auto width = 0, height = 0;
	SDL_Vulkan_GetDrawableSize(m_Window, &width, &height);

	width = std::clamp((uint32_t)width, m_Capabilities.minImageExtent.width, m_Capabilities.maxImageExtent.width);
	height = std::clamp((uint32_t)height, m_Capabilities.minImageExtent.height, m_Capabilities.maxImageExtent.height);

	m_Extent =
	{
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
	};

	// Create the swapchain
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_VkSurfaceKHR;

	createInfo.minImageCount = m_Capabilities.minImageCount + 1;
	createInfo.imageFormat = m_Format.format;
	createInfo.imageColorSpace = m_Format.colorSpace;
	createInfo.imageExtent = m_Extent;
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
	createInfo.presentMode = m_PresentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	Vk::Check(vkCreateSwapchainKHR(m_VkDevice, &createInfo, nullptr, &m_VkSwapchainKHR));

	// Swapchain iomages
	uint32_t imageCount;
	vkGetSwapchainImagesKHR(m_VkDevice, m_VkSwapchainKHR, &imageCount, nullptr);

	m_SwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_VkDevice, m_VkSwapchainKHR, &imageCount, m_SwapChainImages.data());
}

void VkRenderer::CreateImageViews()
{
	m_SwapChainImageViews.resize(m_SwapChainImages.size());
	for (size_t i = 0; i < m_SwapChainImages.size(); i++)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_SwapChainImages[i];

		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_Format.format;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		Vk::Check(vkCreateImageView(m_VkDevice, &createInfo, nullptr, &m_SwapChainImageViews[i]));
	}
}

void VkRenderer::CreateGraphicsPipeline()
{
	// Load SPIR_V
	auto vertShaderCode = Vk::readFile("shaders/vert.spv");
	auto fragShaderCode = Vk::readFile("shaders/frag.spv");

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	// Pipeline
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// Vertex input
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

	// Input assembler
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Viewport
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_Extent.width;
	viewport.height = (float)m_Extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_Extent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	// Multisampling???
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	// Color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	// ???
	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	// Dynamic state
	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	// Pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0; // Optional
	pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	Vk::Check(vkCreatePipelineLayout(m_VkDevice, &pipelineLayoutInfo, nullptr, &m_PipelineLayout));

	// Cleanup at the end
	vkDestroyShaderModule(m_VkDevice, fragShaderModule, nullptr);
	vkDestroyShaderModule(m_VkDevice, vertShaderModule, nullptr);
}

VkShaderModule VkRenderer::createShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	Vk::Check(vkCreateShaderModule(m_VkDevice, &createInfo, nullptr, &shaderModule));
	return shaderModule;
}

void VkRenderer::CreateRenderPass()
{
	// Attachment description
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_Format.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// Subpasses and attachment references
	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	// Render pass
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	Vk::Check(vkCreateRenderPass(m_VkDevice, &renderPassInfo, nullptr, &m_RenderPass));
}
