#include <iostream>
#include <string>
#include <vector>

#include <SDL.h>
#include <SDL_vulkan.h>
#include "Timer.h"

#include <vulkan/vulkan.hpp>


namespace
{
	void CalculateFPS(Timer* timer, SDL_Window* window)
	{
		static double time = 0;
		static int frameCount = 0;

		frameCount++;
		time += timer->DeltaTime();
		if (time > 1.0f)
		{
			auto fps = frameCount;
			time = 0.0f;
			frameCount = 0;

			auto title = "Vulkan Test - FPS: " + std::to_string(fps) + " (" + std::to_string(1000.0f / fps) + " ms)";
			SDL_SetWindowTitle(window, title.c_str());
		}
	}
}

int main(int argc, char** argv)
{
	std::cout << "Vulkan Test\n";

	// Init SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		std::cerr << "SDL_Init failed\n";
		return -1;
	}

	// Create window
	auto window_width = 800;
	auto window_height = 600;

	auto window = SDL_CreateWindow("Vulkan Test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, SDL_WINDOW_VULKAN);
	if (window == nullptr)
	{
		std::cerr << "SDL_CreateWindow failed\n";
		return -1;
	}

	// Vulkan
	unsigned extension_count;
	if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, NULL)) 
	{
		std::cout << "Could not get the number of required instance extensions from SDL." << std::endl;
		return 1;
	}

	std::vector<const char*> extensions(extension_count);
	if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, extensions.data())) 
	{
		std::cout << "Could not get the names of required instance extensions from SDL." << std::endl;
		return 1;
	}

	// Use validation layers if this is a debug build
	std::vector<const char*> layers;
#if defined(_DEBUG)
	layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

	// vk::ApplicationInfo allows the programmer to specifiy some basic information about the
	// program, which can be useful for layers and tools to provide more debug information.
	vk::ApplicationInfo appInfo = vk::ApplicationInfo()
		.setPApplicationName("Vulkan C++ Windowed Program Template")
		.setApplicationVersion(1)
		.setPEngineName("LunarG SDK")
		.setEngineVersion(1)
		.setApiVersion(VK_API_VERSION_1_0);
	// vk::InstanceCreateInfo is where the programmer specifies the layers and/or extensions that
	// are needed.

	vk::InstanceCreateInfo instInfo = vk::InstanceCreateInfo()
		.setFlags(vk::InstanceCreateFlags())
		.setPApplicationInfo(&appInfo)
		.setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
		.setPpEnabledExtensionNames(extensions.data())
		.setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
		.setPpEnabledLayerNames(layers.data());

	// Create the Vulkan instance.
	vk::Instance instance;
	try {
		instance = vk::createInstance(instInfo);
	}
	catch (const std::exception& e) {
		std::cout << "Could not create a Vulkan instance: " << e.what() << std::endl;
		return 1;
	}

	// Create a Vulkan surface for rendering
	VkSurfaceKHR c_surface;
	if (!SDL_Vulkan_CreateSurface(window, static_cast<VkInstance>(instance), &c_surface)) {
		std::cout << "Could not create a Vulkan surface." << std::endl;
		return 1;
	}

	vk::SurfaceKHR surface(c_surface);





	// Timer
	Timer timer;
	timer.Start();

	// Main loop
	SDL_Event e = {};
	while (e.type != SDL_QUIT)
	{
		timer.Tick();
		if (SDL_PollEvent(&e))
		{

		}
		else
		{
			CalculateFPS(&timer, window);


		}
	}

	// Clean up
	instance.destroySurfaceKHR(surface);
	SDL_DestroyWindow(window);
	SDL_Quit();
	instance.destroy();

	return 0;
}