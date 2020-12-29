#include <iostream>
#include <string>
#include <vector>

#include <SDL.h>
#include "Timer.h"
#include "VkRenderer.h"

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
	VkRenderer renderer(window);
	if (!renderer.Create())
	{
		return -1;
	}

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

			renderer.DrawFrame();
		}
	}

	// Clean up
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}