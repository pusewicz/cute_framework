/*
	CF Renderer Example - Simple Triangle
	Demonstrates the basic usage of the extracted CF renderer
*/

#include "../include/cf_renderer.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>

// Simple vertex structure
typedef struct Vertex {
	CFR_Vec2 position;
	CFR_Color color;
} Vertex;

// Very simple vertex shader (SPIR-V would be compiled from GLSL)
// This is a placeholder - real implementation would need actual SPIR-V bytecode
static const uint32_t vertex_shader_spirv[] = {
	// SPIR-V magic number and header would go here
	0x07230203, 0x00010000, 0x00080001, 0x00000000
};

static const uint32_t fragment_shader_spirv[] = {
	// SPIR-V magic number and header would go here
	0x07230203, 0x00010000, 0x00080001, 0x00000000
};

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
		"CF Renderer Example - Simple Triangle",
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
	CFR_Renderer* renderer = cfr_create_renderer(device, window);
	if (!renderer) {
		printf("Failed to create renderer\n");
		SDL_DestroyGPUDevice(device);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}
	
	// Create triangle mesh
	Vertex triangle_vertices[] = {
		{ cfr_v2(0.0f, -0.5f), cfr_make_color_f(1.0f, 0.0f, 0.0f, 1.0f) },  // Top - Red
		{ cfr_v2(0.5f, 0.5f), cfr_make_color_f(0.0f, 1.0f, 0.0f, 1.0f) },   // Right - Green
		{ cfr_v2(-0.5f, 0.5f), cfr_make_color_f(0.0f, 0.0f, 1.0f, 1.0f) }   // Left - Blue
	};
	
	CFR_VertexAttribute attributes[] = {
		{ "position", CFR_VERTEX_FORMAT_FLOAT2, offsetof(Vertex, position) },
		{ "color", CFR_VERTEX_FORMAT_FLOAT4, offsetof(Vertex, color) }
	};
	
	CFR_Mesh mesh = cfr_make_mesh(renderer, sizeof(triangle_vertices), attributes, 2, sizeof(Vertex));
	cfr_mesh_update_vertex_data(renderer, mesh, triangle_vertices, 3);
	
	// Create shader (note: these are placeholder bytecode arrays - real implementation needs actual SPIR-V)
	CFR_Shader shader = cfr_make_shader(
		renderer,
		vertex_shader_spirv, sizeof(vertex_shader_spirv),
		fragment_shader_spirv, sizeof(fragment_shader_spirv)
	);
	
	// Create material
	CFR_Material material = cfr_make_material(renderer);
	CFR_RenderState state = cfr_render_state_defaults();
	cfr_material_set_render_state(renderer, material, state);
	
	// Main loop
	bool running = true;
	SDL_Event event;
	
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
		
		// Begin frame
		cfr_begin_frame(renderer);
		
		// Render to screen
		CFR_Canvas screen = { 0 }; // ID 0 means render to screen
		cfr_apply_canvas(renderer, screen, true);
		
		// Apply mesh and material
		cfr_apply_mesh(renderer, mesh);
		cfr_apply_shader(renderer, shader, material);
		
		// Set viewport to full window
		int w, h;
		SDL_GetWindowSize(window, &w, &h);
		cfr_apply_viewport(renderer, 0, 0, (float)w, (float)h);
		
		// Draw triangle
		cfr_draw_elements(renderer);
		
		// End frame and present
		cfr_end_frame(renderer);
	}
	
	// Cleanup
	cfr_destroy_mesh(renderer, mesh);
	cfr_destroy_shader(renderer, shader);
	cfr_destroy_material(renderer, material);
	cfr_destroy_renderer(renderer);
	
	SDL_DestroyGPUDevice(device);
	SDL_DestroyWindow(window);
	SDL_Quit();
	
	return 0;
}
