/*
	Standalone test runner for spritebatch sort tests.
	Build: g++ -O2 -std=c++20 -I../include -I../src -I../libraries test/run_spritebatch_sort_tests.cpp -o run_sort_tests
*/

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
#include <assert.h>
#include <climits>

// Minimal type definitions to match cute_framework types.
struct CF_V2 { float x, y; };
struct CF_Color { float r, g, b, a; };
struct CF_Pixel { uint8_t r, g, b, a; };

static CF_V2 cf_v2(float x, float y) { return { x, y }; }

#ifndef SPRITEBATCH_U64
#define SPRITEBATCH_U64 unsigned long long
#endif

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

#define SPRITEBATCH_IMPLEMENTATION
#include <cute/cute_spritebatch.h>

// Minimal test framework.
static int s_tests_run = 0;
static int s_tests_passed = 0;

#define REQUIRE(expr) do { if (!(expr)) { printf("  FAIL: %s (line %d)\n", #expr, __LINE__); return false; } } while(0)

#define RUN_TEST(name) do { \
	s_tests_run++; \
	printf("  Running: %s... ", #name); \
	if (name()) { s_tests_passed++; printf("PASS\n"); } \
	else { printf("FAILED\n"); } \
} while(0)

// --- Test helpers ---

static spritebatch_sprite_t make_test_sprite(int sort_bits, SPRITEBATCH_U64 texture_id, SPRITEBATCH_U64 image_id = 0)
{
	spritebatch_sprite_t s;
	memset(&s, 0, sizeof(s));
	s.sort_bits = sort_bits;
	s.texture_id = texture_id;
	s.image_id = image_id;
	s.w = 16;
	s.h = 16;
	return s;
}

static void sort_sprites(spritebatch_sprite_t* sprites, int count)
{
	int* indices = (int*)malloc(sizeof(int) * count);
	int* scratch = (int*)malloc(sizeof(int) * count);
	spritebatch_internal_merge_sort(sprites, indices, scratch, count);
	free(indices);
	free(scratch);
}

// The spritebatch sort comparison: if a.sort_bits < b.sort_bits then a <= b,
// otherwise a.texture_id <= b.texture_id determines ordering.
static bool is_correctly_sorted(spritebatch_sprite_t* sprites, int count)
{
	for (int i = 1; i < count; ++i) {
		const auto& a = sprites[i - 1];
		const auto& b = sprites[i];
		int b_lte_a = (b.sort_bits < a.sort_bits) ? 1 : (b.texture_id <= a.texture_id);
		int a_lte_b = (a.sort_bits < b.sort_bits) ? 1 : (a.texture_id <= b.texture_id);
		if (b_lte_a && !a_lte_b) return false;
	}
	return true;
}

// --- Tests ---

static bool test_basic()
{
	spritebatch_sprite_t sprites[4];
	sprites[0] = make_test_sprite(2, 10, 0);
	sprites[1] = make_test_sprite(1, 20, 1);
	sprites[2] = make_test_sprite(2, 5, 2);
	sprites[3] = make_test_sprite(1, 15, 3);

	sort_sprites(sprites, 4);

	// Sorted by texture_id primarily: 5, 10, 15, 20
	REQUIRE(sprites[0].texture_id == 5);
	REQUIRE(sprites[1].texture_id == 10);
	REQUIRE(sprites[2].texture_id == 15);
	REQUIRE(sprites[3].texture_id == 20);
	REQUIRE(is_correctly_sorted(sprites, 4));
	return true;
}

static bool test_stability()
{
	const int N = 8;
	spritebatch_sprite_t sprites[N];
	for (int i = 0; i < N; ++i) {
		sprites[i] = make_test_sprite(1, 100, (SPRITEBATCH_U64)i);
	}
	sort_sprites(sprites, N);
	for (int i = 0; i < N; ++i) {
		REQUIRE(sprites[i].image_id == (SPRITEBATCH_U64)i);
	}
	return true;
}

static bool test_stability_same_texture()
{
	// All sprites have the same texture_id. With equal texture_ids, the
	// comparison always returns a <= b, so original insertion order is preserved.
	const int N = 8;
	spritebatch_sprite_t sprites[N];
	for (int i = 0; i < N; ++i) {
		sprites[i] = make_test_sprite(N - 1 - i, 50, (SPRITEBATCH_U64)i);
	}
	sort_sprites(sprites, N);
	// Order is preserved (stable sort with equal keys).
	for (int i = 0; i < N; ++i) {
		REQUIRE(sprites[i].image_id == (SPRITEBATCH_U64)i);
	}
	REQUIRE(is_correctly_sorted(sprites, N));
	return true;
}

static bool test_groups_by_texture()
{
	// Test that sprites with the same sort_bits get grouped by texture_id.
	// When sort_bits are all equal, the fallthrough comparison by texture_id
	// will group them contiguously.
	const int N = 12;
	spritebatch_sprite_t sprites[N];
	SPRITEBATCH_U64 tex_ids[] = { 20, 10, 30, 20, 10, 30, 20, 10, 30, 20, 10, 30 };
	for (int i = 0; i < N; ++i) {
		// All same sort_bits so texture_id comparison kicks in.
		sprites[i] = make_test_sprite(0, tex_ids[i], (SPRITEBATCH_U64)i);
	}
	sort_sprites(sprites, N);

	SPRITEBATCH_U64 last_tex = 0;
	bool seen[4] = {};
	for (int i = 0; i < N; ++i) {
		SPRITEBATCH_U64 tid = sprites[i].texture_id;
		if (tid != last_tex) {
			int idx = (tid == 10) ? 1 : (tid == 20) ? 2 : 3;
			REQUIRE(!seen[idx]);
			seen[idx] = true;
			last_tex = tid;
		}
	}
	REQUIRE(is_correctly_sorted(sprites, N));
	return true;
}

static bool test_empty_and_single()
{
	spritebatch_internal_merge_sort(NULL, NULL, NULL, 0);

	spritebatch_sprite_t single = make_test_sprite(5, 42, 99);
	sort_sprites(&single, 1);
	REQUIRE(single.sort_bits == 5);
	REQUIRE(single.texture_id == 42);
	REQUIRE(single.image_id == 99);
	return true;
}

static bool test_already_sorted()
{
	const int N = 16;
	spritebatch_sprite_t sprites[N];
	for (int i = 0; i < N; ++i) {
		sprites[i] = make_test_sprite(i, (SPRITEBATCH_U64)(i / 4) + 1, (SPRITEBATCH_U64)i);
	}
	sort_sprites(sprites, N);
	REQUIRE(is_correctly_sorted(sprites, N));
	return true;
}

static bool test_reverse_order()
{
	const int N = 32;
	spritebatch_sprite_t sprites[N];
	for (int i = 0; i < N; ++i) {
		sprites[i] = make_test_sprite(N - 1 - i, (SPRITEBATCH_U64)(N - 1 - i), (SPRITEBATCH_U64)i);
	}
	sort_sprites(sprites, N);
	REQUIRE(is_correctly_sorted(sprites, N));
	return true;
}

static bool test_large_random()
{
	const int N = 2048;
	spritebatch_sprite_t* sprites = (spritebatch_sprite_t*)malloc(sizeof(spritebatch_sprite_t) * N);
	uint64_t rng = 0xDEADBEEF;
	for (int i = 0; i < N; ++i) {
		rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
		sprites[i] = make_test_sprite((int)(rng % 100), (rng >> 8) % 64 + 1, (SPRITEBATCH_U64)i);
	}
	sort_sprites(sprites, N);
	REQUIRE(is_correctly_sorted(sprites, N));

	bool* seen = (bool*)calloc(N, sizeof(bool));
	for (int i = 0; i < N; ++i) {
		int orig = (int)sprites[i].image_id;
		REQUIRE(orig >= 0 && orig < N);
		REQUIRE(!seen[orig]);
		seen[orig] = true;
	}
	free(seen);
	free(sprites);
	return true;
}

static bool test_all_same_keys()
{
	const int N = 64;
	spritebatch_sprite_t* sprites = (spritebatch_sprite_t*)malloc(sizeof(spritebatch_sprite_t) * N);
	for (int i = 0; i < N; ++i) {
		sprites[i] = make_test_sprite(42, 7, (SPRITEBATCH_U64)i);
	}
	sort_sprites(sprites, N);
	for (int i = 0; i < N; ++i) {
		REQUIRE(sprites[i].image_id == (SPRITEBATCH_U64)i);
	}
	free(sprites);
	return true;
}

static bool test_preserves_geometry()
{
	const int N = 16;
	spritebatch_sprite_t sprites[N];
	for (int i = 0; i < N; ++i) {
		sprites[i] = make_test_sprite(N - 1 - i, (SPRITEBATCH_U64)(i + 1), (SPRITEBATCH_U64)i);
		sprites[i].geom.type = BATCH_GEOMETRY_TYPE_SPRITE;
		sprites[i].geom.alpha = (float)i / N;
		sprites[i].geom.radius = (float)(i * 3);
		sprites[i].geom.box[0] = cf_v2((float)i, (float)(i + 1));
		sprites[i].w = i + 10;
		sprites[i].h = i + 20;
		sprites[i].minx = (float)i * 0.1f;
		sprites[i].maxx = (float)i * 0.2f;
	}
	sort_sprites(sprites, N);
	for (int i = 0; i < N; ++i) {
		int orig = (int)sprites[i].image_id;
		REQUIRE(sprites[i].geom.type == BATCH_GEOMETRY_TYPE_SPRITE);
		REQUIRE(sprites[i].geom.alpha == (float)orig / N);
		REQUIRE(sprites[i].geom.radius == (float)(orig * 3));
		REQUIRE(sprites[i].geom.box[0].x == (float)orig);
		REQUIRE(sprites[i].w == orig + 10);
		REQUIRE(sprites[i].h == orig + 20);
	}
	return true;
}

static bool test_two_elements()
{
	spritebatch_sprite_t sprites[2];
	sprites[0] = make_test_sprite(5, 100, 0);
	sprites[1] = make_test_sprite(1, 50, 1);
	sort_sprites(sprites, 2);
	REQUIRE(sprites[0].texture_id == 50);
	REQUIRE(sprites[1].texture_id == 100);
	return true;
}

static bool test_power_of_two_sizes()
{
	int sizes[] = { 2, 3, 4, 7, 8, 15, 16, 31, 32, 63, 64, 127, 128, 255, 256 };
	for (int si = 0; si < (int)(sizeof(sizes) / sizeof(sizes[0])); ++si) {
		int N = sizes[si];
		spritebatch_sprite_t* sprites = (spritebatch_sprite_t*)malloc(sizeof(spritebatch_sprite_t) * N);
		uint64_t rng = 0xCAFEBABE + si;
		for (int i = 0; i < N; ++i) {
			rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
			sprites[i] = make_test_sprite((int)(rng % 50), (rng >> 4) % 16 + 1, (SPRITEBATCH_U64)i);
		}
		sort_sprites(sprites, N);
		REQUIRE(is_correctly_sorted(sprites, N));

		bool* seen = (bool*)calloc(N, sizeof(bool));
		for (int i = 0; i < N; ++i) {
			int orig = (int)sprites[i].image_id;
			REQUIRE(orig >= 0 && orig < N);
			REQUIRE(!seen[orig]);
			seen[orig] = true;
		}
		free(seen);
		free(sprites);
	}
	return true;
}

int main()
{
	printf("Spritebatch Sort Tests\n");
	printf("======================\n");

	RUN_TEST(test_basic);
	RUN_TEST(test_stability);
	RUN_TEST(test_stability_same_texture);
	RUN_TEST(test_groups_by_texture);
	RUN_TEST(test_empty_and_single);
	RUN_TEST(test_already_sorted);
	RUN_TEST(test_reverse_order);
	RUN_TEST(test_large_random);
	RUN_TEST(test_all_same_keys);
	RUN_TEST(test_preserves_geometry);
	RUN_TEST(test_two_elements);
	RUN_TEST(test_power_of_two_sizes);

	printf("\nResults: %d/%d tests passed.\n", s_tests_passed, s_tests_run);
	return s_tests_passed == s_tests_run ? 0 : 1;
}
