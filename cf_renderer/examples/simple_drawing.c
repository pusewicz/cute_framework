/*
	CF Renderer Example - Simple Shape Drawing
	Demonstrates the full-featured 2D renderer with high-level drawing API
*/

#include "../include/cf_renderer.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	(void)argc;
	(void)argv;
	
	// Initialize SDL
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_Init failed: %s\n", SDL_GetError());
		return 1;
	}
	
	// Create window
	SDL_Window* window = SDL_CreateWindow(
		"CF Renderer Example - High-Level Drawing",
		640, 480,
		SDL_WINDOW_VULKAN
	);
	
	if (!window) {
		printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}
	
	// Create GPU device
	SDL_GPUDevice* device = SDL_CreateGPUDevice(
		SDL_GPU_SHADERFORMAT_SPIRV,
		true,
		NULL
	);
	
	if (!device) {
		printf("SDL_CreateGPUDevice failed: %s\n", SDL_GetError());
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}
	
	if (!SDL_ClaimWindowForGPUDevice(device, window)) {
		printf("SDL_ClaimWindowForGPUDevice failed: %s\n", SDL_GetError());
		SDL_DestroyGPUDevice(device);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}
	
	// Create renderer
	int width = 640, height = 480;
	CFR_Renderer* renderer = cfr_create_renderer(device, window, width, height);
	if (!renderer) {
		printf("Failed to create renderer\n");
		SDL_DestroyGPUDevice(device);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}
	
	// Main loop
	bool running = true;
	SDL_Event event;
	float time = 0.0f;
	
	while (running) {
		// Process events
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				running = false;
			}
			if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
				running = false;
			}
		}
		
		time += 0.016f;  // ~60 FPS
		
		// Begin frame
		cfr_begin_frame(renderer);
		
		// Render to screen
		CFR_Canvas screen = { 0 }; // ID 0 means render to screen
		cfr_render_to(renderer, screen, true);
		
		// Draw some shapes with different colors
		
		// Red circle
		cfr_push_color(renderer, cfr_make_color(255, 50, 50, 255));
		CFR_Circle circle = cfr_make_circle(cfr_v2(200, 200), 50);
		cfr_draw_circle_fill(renderer, circle);
		cfr_pop_color(renderer);
		
		// Green quad
		cfr_push_color(renderer, cfr_make_color(50, 255, 50, 255));
		CFR_Aabb box = cfr_make_aabb_center_half_extents(cfr_v2(400, 200), cfr_v2(60, 60));
		cfr_draw_quad_fill(renderer, box, 0);
		cfr_pop_color(renderer);
		
		// Blue line
		cfr_push_color(renderer, cfr_make_color(50, 50, 255, 255));
		cfr_draw_line(renderer, cfr_v2(100, 400), cfr_v2(540, 400), 4.0f);
		cfr_pop_color(renderer);
		
		// Animated rotating triangle
		cfr_push_transform(renderer, cfr_make_translation(320, 350));
		cfr_rotate(renderer, time);
		cfr_push_color(renderer, cfr_make_color(255, 255, 50, 255));
		cfr_draw_tri_fill(renderer, cfr_v2(0, -40), cfr_v2(40, 40), cfr_v2(-40, 40), 0);
		cfr_pop_color(renderer);
		cfr_pop_transform(renderer);
		
		// End frame and present
		cfr_end_frame(renderer);
	}
	
	// Cleanup
	cfr_destroy_renderer(renderer);
	
	SDL_DestroyGPUDevice(device);
	SDL_DestroyWindow(window);
	SDL_Quit();
	
	return 0;
}
