/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute.h>
#include <internal/cute_draw_internal.h>

#include <string.h>
#include <stdlib.h>

// Helper to create a minimal sprite for testing sort behavior.
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

// Helper to run spritebatch_internal_merge_sort with auto-allocated index buffers.
static void sort_sprites(spritebatch_sprite_t* sprites, int count)
{
	int* indices = (int*)malloc(sizeof(int) * count);
	int* scratch = (int*)malloc(sizeof(int) * count);
	spritebatch_internal_merge_sort(sprites, indices, scratch, count);
	free(indices);
	free(scratch);
}

// The spritebatch sort comparison function sorts:
//   - Primary: sort_bits ascending (preserving draw order), used as tiebreaker
//   - Primary: texture_id ascending (for batching), checked when sort_bits are not less
// In practice: if a.sort_bits < b.sort_bits then a <= b.
//              Otherwise, a.texture_id <= b.texture_id determines a <= b.
// This groups sprites by texture_id to minimize draw calls, using sort_bits
// to preserve relative draw order within the same texture group.
static bool is_correctly_sorted(spritebatch_sprite_t* sprites, int count)
{
	for (int i = 1; i < count; ++i) {
		const auto& a = sprites[i - 1];
		const auto& b = sprites[i];
		// Check: a should come before b according to spritebatch_internal_sprite_less_than_or_equal.
		// If b < a (b would come before a), the sort is wrong.
		int b_lte_a = (b.sort_bits < a.sort_bits) ? 1 : (b.texture_id <= a.texture_id);
		int a_lte_b = (a.sort_bits < b.sort_bits) ? 1 : (a.texture_id <= b.texture_id);
		// If b should strictly come before a, sort is wrong.
		if (b_lte_a && !a_lte_b) return false;
	}
	return true;
}

TEST_CASE(test_spritebatch_sort_basic)
{
	// 4 sprites with varying texture_ids and sort_bits.
	spritebatch_sprite_t sprites[4];
	sprites[0] = make_test_sprite(2, 10, 0);
	sprites[1] = make_test_sprite(1, 20, 1);
	sprites[2] = make_test_sprite(2, 5, 2);
	sprites[3] = make_test_sprite(1, 15, 3);

	sort_sprites(sprites, 4);

	// The comparison sorts primarily by texture_id, with sort_bits as a
	// secondary ordering hint. Expected result from original sort:
	//   (sort=2,tex=5) (sort=2,tex=10) (sort=1,tex=15) (sort=1,tex=20)
	REQUIRE(sprites[0].texture_id == 5);
	REQUIRE(sprites[1].texture_id == 10);
	REQUIRE(sprites[2].texture_id == 15);
	REQUIRE(sprites[3].texture_id == 20);
	REQUIRE(is_correctly_sorted(sprites, 4));

	return true;
}

TEST_CASE(test_spritebatch_sort_stability)
{
	// Test that the sort is stable: sprites with equal (sort_bits, texture_id)
	// should preserve their original relative order.
	const int N = 8;
	spritebatch_sprite_t sprites[N];

	// All have the same sort_bits and texture_id.
	// Use image_id to track original position.
	for (int i = 0; i < N; ++i) {
		sprites[i] = make_test_sprite(1, 100, (SPRITEBATCH_U64)i);
	}

	sort_sprites(sprites, N);

	// After stable sort, order should be preserved (all keys are equal).
	for (int i = 0; i < N; ++i) {
		REQUIRE(sprites[i].image_id == (SPRITEBATCH_U64)i);
	}

	return true;
}

TEST_CASE(test_spritebatch_sort_stability_same_texture)
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

TEST_CASE(test_spritebatch_sort_groups_by_texture)
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

	// Verify grouping: consecutive sprites should have the same texture_id in runs.
	SPRITEBATCH_U64 last_tex = 0;
	bool seen_textures[4] = {};
	for (int i = 0; i < N; ++i) {
		SPRITEBATCH_U64 tid = sprites[i].texture_id;
		if (tid != last_tex) {
			// New group started.
			int idx = (tid == 10) ? 1 : (tid == 20) ? 2 : 3;
			REQUIRE(!seen_textures[idx]); // Should not have seen this texture before.
			seen_textures[idx] = true;
			last_tex = tid;
		}
	}
	REQUIRE(is_correctly_sorted(sprites, N));

	return true;
}

TEST_CASE(test_spritebatch_sort_empty_and_single)
{
	// Empty array.
	spritebatch_internal_merge_sort(NULL, NULL, NULL, 0);

	// Single element.
	spritebatch_sprite_t single = make_test_sprite(5, 42, 99);
	sort_sprites(&single, 1);
	REQUIRE(single.sort_bits == 5);
	REQUIRE(single.texture_id == 42);
	REQUIRE(single.image_id == 99);

	return true;
}

TEST_CASE(test_spritebatch_sort_already_sorted)
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

TEST_CASE(test_spritebatch_sort_reverse_order)
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

TEST_CASE(test_spritebatch_sort_large_random)
{
	const int N = 2048;
	spritebatch_sprite_t* sprites = (spritebatch_sprite_t*)malloc(sizeof(spritebatch_sprite_t) * N);

	uint64_t rng = 0xDEADBEEF;
	for (int i = 0; i < N; ++i) {
		rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
		int sort_bits = (int)(rng % 100);
		SPRITEBATCH_U64 tex_id = (rng >> 8) % 64 + 1;
		sprites[i] = make_test_sprite(sort_bits, tex_id, (SPRITEBATCH_U64)i);
	}

	sort_sprites(sprites, N);
	REQUIRE(is_correctly_sorted(sprites, N));

	// Verify all original sprites are still present.
	bool* seen = (bool*)calloc(N, sizeof(bool));
	for (int i = 0; i < N; ++i) {
		int orig_idx = (int)sprites[i].image_id;
		REQUIRE(orig_idx >= 0 && orig_idx < N);
		REQUIRE(!seen[orig_idx]);
		seen[orig_idx] = true;
	}

	free(seen);
	free(sprites);
	return true;
}

TEST_CASE(test_spritebatch_sort_all_same_keys)
{
	// All sprites have identical sort keys. Stability means original order is preserved.
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

TEST_CASE(test_spritebatch_sort_preserves_geometry)
{
	// Verify that all geometry fields survive the sort (no data corruption).
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
		REQUIRE(sprites[i].geom.box[0].y == (float)(orig + 1));
		REQUIRE(sprites[i].w == orig + 10);
		REQUIRE(sprites[i].h == orig + 20);
		REQUIRE(sprites[i].minx == (float)orig * 0.1f);
		REQUIRE(sprites[i].maxx == (float)orig * 0.2f);
	}

	return true;
}

TEST_CASE(test_spritebatch_sort_two_elements)
{
	spritebatch_sprite_t sprites[2];
	sprites[0] = make_test_sprite(5, 100, 0);
	sprites[1] = make_test_sprite(1, 50, 1);

	sort_sprites(sprites, 2);

	// tex=50 should come before tex=100.
	REQUIRE(sprites[0].texture_id == 50);
	REQUIRE(sprites[1].texture_id == 100);

	return true;
}

TEST_CASE(test_spritebatch_sort_power_of_two_sizes)
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

TEST_SUITE(test_spritebatch_sort)
{
	RUN_TEST_CASE(test_spritebatch_sort_basic);
	RUN_TEST_CASE(test_spritebatch_sort_stability);
	RUN_TEST_CASE(test_spritebatch_sort_stability_same_texture);
	RUN_TEST_CASE(test_spritebatch_sort_groups_by_texture);
	RUN_TEST_CASE(test_spritebatch_sort_empty_and_single);
	RUN_TEST_CASE(test_spritebatch_sort_already_sorted);
	RUN_TEST_CASE(test_spritebatch_sort_reverse_order);
	RUN_TEST_CASE(test_spritebatch_sort_large_random);
	RUN_TEST_CASE(test_spritebatch_sort_all_same_keys);
	RUN_TEST_CASE(test_spritebatch_sort_preserves_geometry);
	RUN_TEST_CASE(test_spritebatch_sort_two_elements);
	RUN_TEST_CASE(test_spritebatch_sort_power_of_two_sizes);
}
