/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Standalone benchmark for spritebatch sort optimization.
// Measures the time to sort spritebatch_sprite_t arrays by (sort_bits, texture_id).
//
// Build standalone:
//   g++ -O2 -std=c++20 -I../include -I../src -I../libraries test/bench_spritebatch_sort.cpp -o bench_spritebatch_sort
//
// Or via cmake:
//   cmake --build build --target bench_spritebatch_sort && ./build/bench_spritebatch_sort

#ifndef _CRT_SECURE_NO_WARNINGS
#	define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _CRT_NONSTDC_NO_DEPRECATE
#	define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include <climits>

// Minimal definitions needed to include cute_spritebatch.h standalone.

#ifndef SPRITEBATCH_U64
#define SPRITEBATCH_U64 unsigned long long
#endif

// Minimal CF_V2/CF_Color/CF_Pixel types matching cute_framework.
struct CF_V2 { float x, y; };
struct CF_Color { float r, g, b, a; };
struct CF_Pixel { uint8_t r, g, b, a; };

static CF_V2 cf_v2(float x, float y) { return { x, y }; }

enum BatchGeometryType : int
{
	BATCH_GEOMETRY_TYPE_TRI,
	BATCH_GEOMETRY_TYPE_TRI_SDF,
	BATCH_GEOMETRY_TYPE_QUAD,
	BATCH_GEOMETRY_TYPE_SPRITE,
	BATCH_GEOMETRY_TYPE_CIRCLE,
	BATCH_GEOMETRY_TYPE_CAPSULE,
	BATCH_GEOMETRY_TYPE_SEGMENT,
	BATCH_GEOMETRY_TYPE_POLYGON,
};

struct BatchGeometry
{
	BatchGeometryType type;
	CF_Pixel color;
	CF_V2 box[4];
	CF_V2 boxH[4];
	int n;
	CF_V2 shape[8];
	float alpha;
	float radius;
	float stroke;
	float aa;
	bool is_text;
	bool is_sprite;
	bool fill;
	bool use_tri_colors;
	bool use_tri_attributes;
	CF_Color user_params;
	CF_Pixel tri_colors[3];
	CF_Color tri_attributes[3];
	float uv_bounds[4];
};

#define SPRITEBATCH_SPRITE_GEOMETRY BatchGeometry
#define SPRITEBATCH_ASSERT assert
#define SPRITEBATCH_LOG(...) ((void)0)

// Include the implementation of the spritebatch header.
#define SPRITEBATCH_IMPLEMENTATION
#include <cute/cute_spritebatch.h>

static double get_time_sec()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

// Simple xorshift PRNG for reproducible benchmarks.
static uint64_t s_rng_state = 12345678901234567ULL;
static uint64_t xorshift64()
{
	uint64_t x = s_rng_state;
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	s_rng_state = x;
	return x;
}

static void fill_random_sprites(spritebatch_sprite_t* sprites, int count, int num_textures)
{
	for (int i = 0; i < count; ++i) {
		memset(&sprites[i], 0, sizeof(spritebatch_sprite_t));
		sprites[i].image_id = xorshift64();
		sprites[i].texture_id = (xorshift64() % num_textures) + 1;
		sprites[i].sort_bits = i; // Ascending order as assigned by spritebatch_push
		sprites[i].w = 32;
		sprites[i].h = 32;
		sprites[i].minx = 0.0f;
		sprites[i].miny = 0.0f;
		sprites[i].maxx = 1.0f;
		sprites[i].maxy = 1.0f;
		sprites[i].geom.type = BATCH_GEOMETRY_TYPE_SPRITE;
		sprites[i].geom.alpha = 1.0f;
		sprites[i].geom.is_sprite = true;
	}
}

struct BenchResult {
	double avg_ms;
	double min_ms;
	double max_ms;
};

static BenchResult run_sort_benchmark(int sprite_count, int num_textures, int iterations)
{
	spritebatch_sprite_t* sprites = (spritebatch_sprite_t*)malloc(sizeof(spritebatch_sprite_t) * sprite_count);
	int* sort_indices = (int*)malloc(sizeof(int) * sprite_count);
	int* sort_scratch = (int*)malloc(sizeof(int) * sprite_count);
	spritebatch_sprite_t* original = (spritebatch_sprite_t*)malloc(sizeof(spritebatch_sprite_t) * sprite_count);

	s_rng_state = 42;
	fill_random_sprites(original, sprite_count, num_textures);

	double total_ms = 0;
	double min_ms = 1e9;
	double max_ms = 0;

	for (int iter = 0; iter < iterations; ++iter) {
		memcpy(sprites, original, sizeof(spritebatch_sprite_t) * sprite_count);

		double start = get_time_sec();
		spritebatch_internal_merge_sort(sprites, sort_indices, sort_scratch, sprite_count);
		double end = get_time_sec();

		double ms = (end - start) * 1000.0;
		total_ms += ms;
		if (ms < min_ms) min_ms = ms;
		if (ms > max_ms) max_ms = ms;
	}

	free(sprites);
	free(sort_indices);
	free(sort_scratch);
	free(original);

	return { total_ms / iterations, min_ms, max_ms };
}

int main(int argc, char* argv[])
{
	(void)argc;
	(void)argv;

	printf("=============================================================\n");
	printf("  Spritebatch Sort Benchmark\n");
	printf("  sizeof(spritebatch_sprite_t) = %zu bytes\n", sizeof(spritebatch_sprite_t));
	printf("  sizeof(BatchGeometry)        = %zu bytes\n", sizeof(BatchGeometry));
	printf("=============================================================\n\n");

	struct TestCase {
		int sprite_count;
		int num_textures;
		int iterations;
		const char* label;
	};

	TestCase cases[] = {
		{    100,   4,  1000, "100 sprites, 4 textures"    },
		{    500,   8,   500, "500 sprites, 8 textures"    },
		{   1000,  16,   200, "1000 sprites, 16 textures"  },
		{   5000,  32,   100, "5000 sprites, 32 textures"  },
		{  10000,  64,    50, "10000 sprites, 64 textures" },
		{  50000, 128,    10, "50000 sprites, 128 textures"},
	};

	printf("%-35s %10s %10s %10s\n", "Test Case", "Avg (ms)", "Min (ms)", "Max (ms)");
	printf("------------------------------------------------------------------------\n");

	for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); ++i) {
		BenchResult r = run_sort_benchmark(cases[i].sprite_count, cases[i].num_textures, cases[i].iterations);
		printf("%-35s %10.3f %10.3f %10.3f\n", cases[i].label, r.avg_ms, r.min_ms, r.max_ms);
	}

	printf("\n");
	return 0;
}
