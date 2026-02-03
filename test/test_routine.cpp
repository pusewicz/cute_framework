/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"
#include <cute.h>

using namespace Cute;

// Test: struct initialization (C++ default init)
TEST_CASE(test_routine_init) {
	CF_Routine rt = {};
	REQUIRE(rt.at == 0);
	REQUIRE(rt.elapsed == 0);
	REQUIRE(rt.seconds == 0);
	REQUIRE(rt.wait_elapsed == 0);
	return true;
}

// Test: string labels
TEST_CASE(test_routine_string_labels) {
	CF_Routine rt = {};
	float dt = 0.016f;
	int visited_a = 0;
	int visited_b = 0;

	// Routines need multiple iterations to progress through states
	for (int i = 0; i < 5; i++) {
		rt_begin(rt, dt)
		{
			nav_goto("state_a");
		}
		rt_label("state_a")
		{
			visited_a++;
			nav_goto("state_b");
		}
		rt_label("state_b")
		{
			visited_b++;
		}
		rt_end();
	}

	REQUIRE(visited_a == 1);
	REQUIRE(visited_b == 1);
	return true;
}

// Test: integer labels in C++
TEST_CASE(test_routine_integer_labels) {
	enum { STATE_A = 1, STATE_B };
	CF_Routine rt = {};
	float dt = 0.016f;
	int visited_a = 0;
	int visited_b = 0;

	// Routines need multiple iterations to progress through states
	for (int i = 0; i < 5; i++) {
		rt_begin(rt, dt)
		{
			nav_goto(STATE_A);
		}
		rt_label(STATE_A)
		{
			visited_a++;
			nav_goto(STATE_B);
		}
		rt_label(STATE_B)
		{
			visited_b++;
		}
		rt_end();
	}

	REQUIRE(visited_a == 1);
	REQUIRE(visited_b == 1);
	return true;
}

// Test: rt_once behavior
TEST_CASE(test_routine_once) {
	CF_Routine rt = {};
	float dt = 0.016f;
	int count = 0;

	for (int i = 0; i < 3; i++) {
		rt_begin(rt, dt)
		{
			count++;
		}
		rt_once()
		{
			count++;
		}
		rt_end();
	}

	REQUIRE(count == 2);  // First block once, rt_once once
	return true;
}

// Test: rt_seconds behavior
TEST_CASE(test_routine_seconds) {
	CF_Routine rt = {};
	float dt = 0.5f;
	int count = 0;

	// Run 4 times: 0.5 + 0.5 + 0.5 + 0.5 = 2.0 seconds
	for (int i = 0; i < 4; i++) {
		rt_begin(rt, dt)
		{
			// Starting block runs once
		}
		rt_seconds(1.0f)
		{
			count++;
		}
		rt_end();
	}

	// rt_seconds(1.0f) with dt=0.5f runs for 2 iterations (0.5 + 0.5 >= 1.0)
	REQUIRE(count == 2);
	return true;
}

// Test: nav_restart
TEST_CASE(test_routine_restart) {
	CF_Routine rt = {};
	float dt = 0.016f;
	int start_count = 0;

	for (int i = 0; i < 3; i++) {
		rt_begin(rt, dt)
		{
			start_count++;
			if (start_count < 3) {
				nav_restart();
			}
		}
		rt_once()
		{
			// Should only reach here after start_count >= 3
		}
		rt_end();
	}

	REQUIRE(start_count == 3);
	return true;
}

// Test: nav_redo
TEST_CASE(test_routine_redo) {
	CF_Routine rt = {};
	float dt = 0.016f;
	int redo_count = 0;

	for (int i = 0; i < 5; i++) {
		rt_begin(rt, dt)
		{
			redo_count++;
			if (redo_count < 3) {
				nav_redo();
			}
		}
		rt_once()
		{
			// Should reach here after redo_count >= 3
		}
		rt_end();
	}

	REQUIRE(redo_count == 3);
	return true;
}

TEST_SUITE(test_routine) {
	RUN_TEST_CASE(test_routine_init);
	RUN_TEST_CASE(test_routine_string_labels);
	RUN_TEST_CASE(test_routine_integer_labels);
	RUN_TEST_CASE(test_routine_once);
	RUN_TEST_CASE(test_routine_seconds);
	RUN_TEST_CASE(test_routine_restart);
	RUN_TEST_CASE(test_routine_redo);
}
