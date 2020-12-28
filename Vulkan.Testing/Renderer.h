#pragma once

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.hpp>


class Renderer
{
public:
	Renderer(SDL_Window* window);

	bool Create();

private:
	SDL_Window* m_Window = nullptr;

	// Vulkan instance
	VkInstance m_VkInstance;
	bool CreateVkInstance();
};