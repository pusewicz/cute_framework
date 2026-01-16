# Random Numbers

Random numbers are essential for creating variety in games - from dice rolls and loot drops to enemy AI behavior and procedural generation. Cute Framework provides a fast, high-quality random number generator that's easy to use and gives you full control.

## Why Not Use C's `rand()`?

You might already know the [`rand()`](https://en.cppreference.com/w/c/numeric/random/rand) function from C. While it works for simple cases, it has some problems:

1. **Quality varies** - Different compilers produce different quality random numbers
2. **Global state** - You can only have one random sequence at a time
3. **Limited range** - Returns values only from 0 to `RAND_MAX`

CF's random number generator solves all these issues. It uses the [XorShift+](https://en.wikipedia.org/wiki/Xorshift) algorithm, which is both fast and produces high-quality random numbers.

## Getting Started

### Creating a Random Number Generator

To create a random number generator, use `cf_rnd_seed` with any integer value:

```cpp
CF_Rnd rnd = cf_rnd_seed(42);
```

The [`CF_Rnd`](https://randygaul.github.io/cute_framework/#/random/cf_rnd) struct is very small - just 16 bytes (two 64-bit numbers). Create as many as you need without worrying about memory.

### Understanding Seeds

A **seed** is the starting point for your random number sequence. Here's the key insight: **the same seed always produces the same sequence of numbers**.

```cpp
CF_Rnd rnd1 = cf_rnd_seed(42);
CF_Rnd rnd2 = cf_rnd_seed(42);

// Both generate the exact same number!
int a = cf_rnd_range_int(&rnd1, 1, 100);
int b = cf_rnd_range_int(&rnd2, 1, 100);
// a == b (always)
```

This is called **determinism**, and it's incredibly useful:

- **Debugging**: Found a bug with a specific seed? Use that seed to reproduce it every time
- **Replays**: Record the seed to replay an entire game session
- **Procedural generation**: Generate the same world from the same seed

### Random Seeds for Different Playthroughs

If you want different random numbers each time your game runs, use the current time as a seed:

```cpp
#include <time.h>

CF_Rnd rnd = cf_rnd_seed((uint64_t)time(NULL));
```

The `time()` function returns the number of seconds since January 1, 1970, so it's different every second.

## API Reference

### Generating Random Numbers

CF provides several functions to generate random numbers in different formats:

| Function | Returns | Range |
|----------|---------|-------|
| [`cf_rnd_uint64`](https://randygaul.github.io/cute_framework/#/random/cf_rnd_uint64) | `uint64_t` | 0 to 18,446,744,073,709,551,615 |
| [`cf_rnd_float`](https://randygaul.github.io/cute_framework/#/random/cf_rnd_float) | `float` | 0.0 to 1.0 (exclusive) |
| [`cf_rnd_double`](https://randygaul.github.io/cute_framework/#/random/cf_rnd_double) | `double` | 0.0 to 1.0 (exclusive) |

```cpp
CF_Rnd rnd = cf_rnd_seed(42);

uint64_t big_number = cf_rnd_uint64(&rnd);  // e.g., 12345678901234567890
float percentage = cf_rnd_float(&rnd);       // e.g., 0.7312...
double precise = cf_rnd_double(&rnd);        // e.g., 0.4521893...
```

### Generating Numbers in a Range

Often you need random numbers within specific bounds. These functions take a minimum and maximum value (both **inclusive**):

| Function | Returns | Description |
|----------|---------|-------------|
| [`cf_rnd_range_int`](https://randygaul.github.io/cute_framework/#/random/cf_rnd_range_int) | `int` | Integer in range [min, max] |
| [`cf_rnd_range_uint64`](https://randygaul.github.io/cute_framework/#/random/cf_rnd_range_uint64) | `uint64_t` | 64-bit unsigned in range [min, max] |
| [`cf_rnd_range_float`](https://randygaul.github.io/cute_framework/#/random/cf_rnd_range_float) | `float` | Float in range [min, max] |
| [`cf_rnd_range_double`](https://randygaul.github.io/cute_framework/#/random/cf_rnd_range_double) | `double` | Double in range [min, max] |

```cpp
CF_Rnd rnd = cf_rnd_seed(42);

// Roll a six-sided die (1 to 6)
int die_roll = cf_rnd_range_int(&rnd, 1, 6);

// Random damage between 10 and 25
int damage = cf_rnd_range_int(&rnd, 10, 25);

// Random position on screen
float x = cf_rnd_range_float(&rnd, -400.0f, 400.0f);
float y = cf_rnd_range_float(&rnd, -300.0f, 300.0f);
```

## Practical Examples

### Rolling Dice

Simulate any kind of dice roll:

```cpp
// Roll a single die with N sides
int roll_die(CF_Rnd* rnd, int sides)
{
    return cf_rnd_range_int(rnd, 1, sides);
}

// Roll multiple dice and sum the results (like 3d6)
int roll_dice(CF_Rnd* rnd, int count, int sides)
{
    int total = 0;
    for (int i = 0; i < count; i++) {
        total += cf_rnd_range_int(rnd, 1, sides);
    }
    return total;
}

// Usage
CF_Rnd rnd = cf_rnd_seed(42);
int d20 = roll_die(&rnd, 20);        // Roll a d20
int stats = roll_dice(&rnd, 3, 6);   // Roll 3d6 for character stats
```

### Probability Checks

Check if something happens based on a percentage chance:

```cpp
// Returns true with the given probability (0.0 to 1.0)
bool chance(CF_Rnd* rnd, float probability)
{
    return cf_rnd_float(rnd) < probability;
}

// Usage
CF_Rnd rnd = cf_rnd_seed(42);

if (chance(&rnd, 0.25f)) {
    // 25% chance to drop rare loot
    spawn_rare_item();
}

if (chance(&rnd, 0.05f)) {
    // 5% critical hit chance
    damage *= 2;
}
```

### Picking a Random Item from an Array

Select a random element from a collection:

```cpp
// Pick a random index from an array of a given size
int random_index(CF_Rnd* rnd, int array_size)
{
    return cf_rnd_range_int(rnd, 0, array_size - 1);
}

// Usage
const char* enemy_types[] = {"goblin", "orc", "troll", "dragon"};
int count = sizeof(enemy_types) / sizeof(enemy_types[0]);

CF_Rnd rnd = cf_rnd_seed(42);
const char* enemy = enemy_types[random_index(&rnd, count)];
```

### Shuffling an Array

Randomly reorder elements using the [Fisher-Yates shuffle](https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle):

```cpp
void shuffle_int_array(CF_Rnd* rnd, int* array, int count)
{
    for (int i = count - 1; i > 0; i--) {
        int j = cf_rnd_range_int(rnd, 0, i);
        // Swap array[i] and array[j]
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

// Usage - shuffle a deck of cards (represented as 0-51)
int deck[52];
for (int i = 0; i < 52; i++) deck[i] = i;

CF_Rnd rnd = cf_rnd_seed(42);
shuffle_int_array(&rnd, deck, 52);
```

### Random Position in a Circle

Spawn objects at random positions within a circular area:

```cpp
CF_V2 random_point_in_circle(CF_Rnd* rnd, CF_V2 center, float radius)
{
    // Use square root for uniform distribution
    float r = radius * cf_sqrt(cf_rnd_float(rnd));
    float angle = cf_rnd_range_float(rnd, 0, CF_PI * 2);

    CF_V2 point;
    point.x = center.x + r * cf_cos(angle);
    point.y = center.y + r * cf_sin(angle);
    return point;
}

// Usage - spawn enemies around the player
CF_Rnd rnd = cf_rnd_seed(42);
CF_V2 player_pos = cf_v2(100, 200);

for (int i = 0; i < 5; i++) {
    CF_V2 spawn_pos = random_point_in_circle(&rnd, player_pos, 150.0f);
    spawn_enemy(spawn_pos);
}
```

### Weighted Random Selection

Choose from options with different probabilities:

```cpp
// weights array contains relative weights for each option
// Returns the index of the selected option
int weighted_random(CF_Rnd* rnd, float* weights, int count)
{
    // Calculate total weight
    float total = 0;
    for (int i = 0; i < count; i++) {
        total += weights[i];
    }

    // Pick a random point in the total weight
    float roll = cf_rnd_range_float(rnd, 0, total);

    // Find which option this corresponds to
    float cumulative = 0;
    for (int i = 0; i < count; i++) {
        cumulative += weights[i];
        if (roll < cumulative) {
            return i;
        }
    }

    return count - 1; // Fallback (shouldn't happen)
}

// Usage - loot table with different drop rates
const char* items[] = {"common", "uncommon", "rare", "legendary"};
float weights[] = {60.0f, 25.0f, 12.0f, 3.0f};  // Drop percentages

CF_Rnd rnd = cf_rnd_seed(42);
int drop = weighted_random(&rnd, weights, 4);
printf("Dropped: %s\n", items[drop]);
```

## Using Multiple Generators

Since `CF_Rnd` is so lightweight, you can create separate generators for different game systems:

```cpp
typedef struct GameRng
{
    CF_Rnd world;    // For procedural world generation
    CF_Rnd combat;   // For combat calculations
    CF_Rnd loot;     // For loot drops
    CF_Rnd visual;   // For visual effects (particles, etc.)
} GameRng;

GameRng init_game_rng(uint64_t master_seed)
{
    GameRng rng;
    rng.world = cf_rnd_seed(master_seed);
    rng.combat = cf_rnd_seed(master_seed + 1);
    rng.loot = cf_rnd_seed(master_seed + 2);
    rng.visual = cf_rnd_seed(master_seed + 3);
    return rng;
}
```

This separation is useful because:
- Visual randomness (particles, screen shake) doesn't affect gameplay determinism
- You can save/load specific generators for replay systems
- Debugging becomes easier when systems are isolated

## C++ API

If you're using C++, CF provides a cleaner API with function overloading:

```cpp
#include <cute.h>
using namespace Cute;

CF_Rnd rnd = rnd_seed(42);

// All these use the same function name with different types
int i = rnd_range(rnd, 1, 10);           // Returns int
float f = rnd_range(rnd, 0.0f, 1.0f);    // Returns float
double d = rnd_range(rnd, 0.0, 100.0);   // Returns double

// You can pass by reference instead of pointer
float val = rnd_float(rnd);              // No need for &rnd
```

## Tips for Game Development

1. **Save your seeds**: When generating procedural content, save the seed so players can share interesting worlds or you can reproduce bugs.

2. **Don't reseed constantly**: Create your generator once and reuse it. Reseeding defeats the purpose of having a sequence.

3. **Separate gameplay from visuals**: Keep a separate RNG for particle effects and screen shake. This way, visual randomness doesn't affect gameplay replays.

4. **Test with fixed seeds**: During development, use a fixed seed to get reproducible behavior. Switch to time-based seeding for release.

5. **Beware of order dependence**: The sequence of random numbers depends on the order you call the functions. Adding a new random call can shift all subsequent values.
