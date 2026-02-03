/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_ROUTINE_H
#define CF_ROUTINE_H

#include <stdint.h>
#include "cute_defines.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct   CF_Routine
 * @category utility
 * @brief    A portable "coroutine"-like thing for implementing FSM and behavior-cycles.
 * @remarks  Original implementation by Noel Berry.
 *           See: https://gist.github.com/NoelFB/7a5fa66fc29dd7ed1c11042c30f1b00e
 *
 *           Routine is a set of macros useful to implement Finite State Machine type of
 *           behaviors. It works really well for simpler behaviors with loops or cycles,
 *           and works especially well for anything resembling a cutscene.
 *
 *           The macros here are wrappers around a switch statement. All of the `rt_***`
 *           macros save a bookmark. On the next frame the routine is resumed at the last
 *           bookmark.
 *
 *           Each `rt_***` macro has a block { }. It contains code to run, and is attached
 *           to the `rt_***` macro. The exception is `rt_end` has no block. Blocks can have
 *           local variables but they don't persist, so be careful with them. A block will
 *           run for one frame.
 */
typedef struct CF_RoutineBase
{
	/** A "hidden feature" - Goes from 0 to 1 over X seconds during `rt_seconds`. Useful for animating things. */
	float elapsed;

	/** Used in `rt_seconds` to repeat the block for X seconds. */
	float seconds;

	/** Used in `rt_wait` to wait for X seconds. */
	float wait_elapsed;

	/** Bookmark for which block in the routine we are currently at. */
	uint64_t at;
} CF_RoutineBase;

#ifndef __cplusplus
typedef CF_RoutineBase CF_Routine;
#endif

/**
 * @function cf_routine_init
 * @category utility
 * @brief    Initializes a CF_Routine to default values.
 * @param    rt       The routine to initialize.
 */
CF_INLINE void cf_routine_init(CF_RoutineBase* rt)
{
	rt->elapsed = 0;
	rt->seconds = 0;
	rt->wait_elapsed = 0;
	rt->at = 0;
}

#ifdef __cplusplus
}
#endif

//--------------------------------------------------------------------------------------------------
// C-only macros (use pointers internally)

#ifndef __cplusplus

// -------------------------------------------------------------------------------------------------
// Example:
// enum { STATE_ATTACK = 1, STATE_COOLDOWN };
// rt_begin(routine, delta_time())
// {
//     // This is the starting block. Runs once by default.
// }
//
// rt_label(STATE_ATTACK)  // C version: use integer/enum labels
// {
//     // Runs once.
// }
// rt_seconds(3)
// {
//     // Runs once per frame for 3 seconds.
// }
// rt_wait(1)
// {
//     // Waits 1 second, then runs this once.
// }
// rt_once()
// {
//     // Runs this one time.
//
//     if (key_pressed(SPACE))
//         nav_goto(STATE_COOLDOWN); // Next frame will begin on `rt_label(STATE_COOLDOWN)`.
//     else
//         nav_goto(STATE_ATTACK);   // Restart this state.
// }
//
// rt_label(STATE_COOLDOWN) { } // Empty block.
// rt_wait(5.0f) { } // Empty block. This is like a long "cooldown" of 5 seconds.
// rt_once()
// {
//     nav_restart(); // Restart the whole routine back at `rt_begin`.
// }
// rt_end();

/** Begins the routine. */
#define rt_begin(routine, dt)                                                \
    do {                                                                     \
        CF_Routine* __rt = &(routine);                                       \
        bool __mn = true;                                                    \
        float __dt = (float)(dt);                                            \
        if (__rt->wait_elapsed > 0) __rt->wait_elapsed -= __dt;              \
        else switch (__rt->at) {                                             \
        case 0: do {                                                         \

/** Has a label that can be jumped to with `nav_goto(id)`. C version: accepts integers/enums only. */
#define rt_label(id)                                                         \
        } while (0); if (__mn) __rt->at = (uint64_t)(id); break;             \
        case (uint64_t)(id): do {                                            \

/** Runs its block once. */
#define rt_once()                                                            \
        } while (0); if (__mn) __rt->at = __LINE__; break;                   \
        case __LINE__: do {                                                  \

/** Runs its block for `time` seconds. */
#define rt_seconds(time)                                                     \
        rt_once();                                                           \
        CF_Routine* rt = __rt;                                               \
        if (__rt->seconds < (time)) {                                        \
            __rt->seconds += __dt;                                           \
            __mn = __rt->seconds >= (time);                                  \
            if (__mn) {                                                      \
                __rt->elapsed = 1;                                           \
                __rt->seconds = 0;                                           \
            } else {                                                         \
                __rt->elapsed = __rt->seconds / (float)(time);               \
            }                                                                \
        }                                                                    \

/** Runs its block while `condition` is true. */
#define rt_while(condition)                                                  \
        } while (0); if (__mn) __rt->at = __LINE__; break;                   \
        case __LINE__: if (condition)                                        \
            do { __mn = false;                                               \

/** Runs the block indefinitely, until a `nav_***` macro is used. */
#define rt_always()                                                          \
        rt_while(true)

/** Runs its block once when `condition` becomes true. */
#define rt_upon(condition)                                                   \
        } while (0); if (__mn) {                                             \
            __rt->at = (uint64_t)((condition) ? __LINE__ : -__LINE__);       \
        } break;                                                             \
        case -__LINE__:                                                      \
            if (condition) __rt->at = __LINE__;                              \
            break;                                                           \
        case __LINE__: do {                                                  \

/** Waits for `time` seconds before running its block. */
#define rt_wait(time)                                                        \
        } while (0); if (__mn) {                                             \
            __rt->wait_elapsed = (float)(time);                              \
            __rt->at = __LINE__;                                             \
        }                                                                    \
        break;                                                               \
        case __LINE__: do {                                                  \

/** End of the routine. Does not have a block. */
#define rt_end()                                                             \
        } while (0); if (__mn) __rt->at = (uint64_t)-1;                      \
        break;                                                               \
        } __rt_end:;                                                         \
    } while (0)                                                              \

// -------------------------------------------------------------------------------------------------
// The `nav_***` macros can be used anywhere in a coroutine.
// Call them like normal functions (inside if-statements, loops, etc.).

/** Repeats the block of code this is placed within. Skips the rest of the block. */
#define nav_redo()                                                           \
        do { goto __rt_end; } while (0)                                      \

/** Goes to the block with the id set by `rt_label(id)`. C version: accepts integers/enums only. */
#define nav_goto(id)                                                         \
        do {                                                                 \
            __rt->at = (uint64_t)(id);                                       \
            goto __rt_end;                                                   \
        } while (0)                                                          \

/** Restarts the whole routine. */
#define nav_restart()                                                        \
        do {                                                                 \
            __rt->at = 0;                                                    \
            __rt->elapsed = 0;                                               \
            __rt->seconds = 0;                                               \
            __rt->wait_elapsed = 0;                                          \
            goto __rt_end;                                                   \
        } while (0)                                                          \

#endif /* !__cplusplus */

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

// -------------------------------------------------------------------------------------------------
// For internal use by `rt_label(name)` and `nav_goto(name)`.

CF_INLINE constexpr uint64_t rt_fnv1a(const char* name)
{
	uint64_t h = 14695981039346656037ULL;
	char c = 0;
	while ((c = *name++)) {
		h = h ^ (uint64_t)c;
		h = h * 1099511628211ULL;
	}
	return h;
}

// Overloaded constexpr helper - accepts both integers and strings
CF_INLINE constexpr uint64_t rt_label_val(uint64_t id) { return id; }
CF_INLINE constexpr uint64_t rt_label_val(int id) { return (uint64_t)id; }
CF_INLINE constexpr uint64_t rt_label_val(const char* name) { return rt_fnv1a(name); }

}

// -------------------------------------------------------------------------------------------------
// C++ struct with member functions (extends base C struct)

struct CF_Routine : CF_RoutineBase
{
	CF_Routine() { elapsed = seconds = wait_elapsed = 0; at = 0; }

	// Sets which label (defined by `rt_label`) to resume from next.
	CF_INLINE void set_next(const char* label) { at = Cute::rt_fnv1a(label); elapsed = seconds = wait_elapsed = 0; }
	CF_INLINE void reset() { *this = CF_Routine(); }
	CF_INLINE void init() { reset(); }
};

// -------------------------------------------------------------------------------------------------
// Example:
// rt_begin(routine, delta_time())
// {
//     // This is the starting block. Runs once by default.
// }
//
// rt_label("state 1")  // C++ version: use string or integer/enum labels
// {
//     // Runs once.
// }
// rt_seconds(3)
// {
//     // Runs once per frame for 3 seconds.
// }
// rt_wait(1)
// {
//     // Waits 1 second, then runs this once.
// }
// rt_once()
// {
//     // Runs this one time.
//
//     if (key_pressed(SPACE))
//         nav_goto("state 2"); // Next frame will begin on `rt_label("state 2")`.
//     else
//         nav_goto("state 1"); // Restart this state.
// }
//
// rt_label("state 2") { } // Empty block.
// rt_wait(5.0f) { } // Empty block. This is like a long "cooldown" of 5 seconds.
// rt_once()
// {
//     nav_restart(); // Restart the whole routine back at `rt_begin`.
// }
// rt_end();

/** Begins the routine. */
#define rt_begin(routine, dt)                                                \
    do {                                                                     \
        CF_Routine& __rt = routine;                                          \
        bool __mn = true;                                                    \
        float __dt = dt;                                                     \
        if (__rt.wait_elapsed > 0) __rt.wait_elapsed -= __dt;                \
        else switch (__rt.at) {                                              \
        case 0: do {                                                         \

/** Has a label that can be jumped to with `nav_goto(x)`. C++ version: accepts integers/enums or string literals. */
#define rt_label(x)                                                          \
        } while (0); if (__mn) __rt.at = Cute::rt_label_val(x); break;       \
        case Cute::rt_label_val(x): do {                                     \

/** Runs its block once. */
#define rt_once()                                                            \
        } while (0); if (__mn) __rt.at = __LINE__; break;                    \
        case __LINE__: do {                                                  \

/** Runs its block for `time` seconds. */
#define rt_seconds(time)                                                     \
        rt_once();                                                           \
        auto& rt = __rt;                                                     \
        if (__rt.seconds < time) {                                           \
            __rt.seconds += __dt;                                            \
            __mn = __rt.seconds >= time;                                     \
            if (__mn) {                                                      \
                __rt.elapsed = 1;                                            \
                __rt.seconds = 0;                                            \
            } else {                                                         \
                __rt.elapsed = __rt.seconds / (float)time;                   \
            }                                                                \
        }                                                                    \

/** Runs its block while `condition` is true. */
#define rt_while(condition)                                                  \
        } while (0); if (__mn) __rt.at = __LINE__; break;                    \
        case __LINE__: if (condition)                                        \
            do { __mn = false;                                               \

/** Runs the block indefinitely, until a `nav_***` macro is used. */
#define rt_always()                                                          \
        rt_while(true)

/** Runs its block once when `condition` becomes true. */
#define rt_upon(condition)                                                   \
        } while (0); if (__mn) {                                             \
            __rt.at = (uint64_t)((condition) ? __LINE__ : -__LINE__);        \
        } break;                                                             \
        case -__LINE__:                                                      \
            if (condition) __rt.at = __LINE__;                               \
            break;                                                           \
        case __LINE__: do {                                                  \

/** Waits for `time` seconds before running its block. */
#define rt_wait(time)                                                        \
        } while (0); if (__mn) {                                             \
            __rt.wait_elapsed = time;                                        \
            __rt.at = __LINE__;                                              \
        }                                                                    \
        break;                                                               \
        case __LINE__: do {                                                  \

#ifdef _MSC_VER
/* Unreferenced goto label (sometimes from rt_end). */
#pragma warning(disable:4102)
#endif

/** End of the routine. Does not have a block. */
#define rt_end()                                                             \
        } while (0); if (__mn) __rt.at = -1;                                 \
        break;                                                               \
        } __rt_end:;                                                         \
    } while (0);                                                             \

// -------------------------------------------------------------------------------------------------
// The `nav_***` macros can be used anywhere in a coroutine.
// Call them like normal functions (inside if-statements, loops, etc.).

/** Repeats the block of code this is placed within. Skips the rest of the block. */
#define nav_redo()                                                           \
        do { goto __rt_end; } while (0)                                      \

/** Goes to the block with the label set by `rt_label(x)`. C++ version: accepts integers/enums or string literals. */
#define nav_goto(x)                                                          \
        do {                                                                 \
            __rt.at = Cute::rt_label_val(x);                                 \
            goto __rt_end;                                                   \
        } while (0)                                                          \

/** Restarts the whole routine. */
#define nav_restart()                                                        \
        do {                                                                 \
            __rt.at = 0;                                                     \
            __rt.elapsed = 0;                                                \
            __rt.seconds = 0;                                                \
            __rt.wait_elapsed = 0;                                           \
            goto __rt_end;                                                   \
        } while (0)                                                          \

#endif // CF_CPP

#endif // CF_ROUTINE_H
