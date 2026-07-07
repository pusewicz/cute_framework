# Sprite Batcher Analysis

An analysis of `libraries/cute/cute_spritebatch.h` (v1.08) and its integration into Cute
Framework via `src/cute_draw.cpp`. Covers: how the system works, bugs found (with suggested
fixes), and performance/memory optimization opportunities.

All findings below were cross-checked against upstream
[`RandyGaul/cute_headers/cute_spritebatch.h`](https://github.com/RandyGaul/cute_headers) —
the vendored copy is essentially identical, so every bug listed here is inherited from
upstream rather than introduced by this fork. Fixes should ideally be upstreamed too.

---

## 1. How the sprite batcher works

### 1.1 Purpose

`cute_spritebatch.h` implements a *runtime* texture atlas cache. Instead of pre-baking
atlases in an art pipeline, games load individual images and the batcher packs the ones
being drawn together into large atlas textures on the fly. Sprites sharing an atlas can be
submitted in a single draw call. Atlases "decay" over time as their images stop being drawn
and are rebuilt (defragmented) from the images still in use.

### 1.2 Core data structures (`spritebatch_t`)

| Field | Role |
|---|---|
| `input_buffer` | Raw sprites accepted by `spritebatch_push`, in push order |
| `sprites` / `sprites_scratch` | Post-processed sprites with resolved `texture_id` + UVs; scratch is the merge-sort double buffer |
| `sprites_to_lonely_textures` | Map: `image_id` → "lonely" texture (an image with its own standalone GPU texture, not in any atlas yet) |
| `sprites_to_atlases` | Map: `image_id` → atlas that currently contains it |
| `sprites_to_premades` | Map: `image_id` → sub-rect of a user-registered premade atlas (used by `cf_register_premade_atlas`) |
| `atlases` | Circular doubly-linked list of live atlases; each atlas owns a `sprites_to_textures` map (`image_id` → UV rect + decay timestamp) |
| `key_buffer` | Deferred-deletion key list so maps aren't mutated mid-iteration |
| `pixel_buffer` | Grow-only staging buffer for `get_pixels_fn` results |

All maps are a single open-addressing hash table implementation (`spritebatch_map_t`,
linear probing, dense item array for iteration, swap-remove).

### 1.3 The frame lifecycle

Each sprite carries a `SPRITEBATCH_SPRITE_GEOMETRY` payload (CF overrides this with its
fat `BatchGeometry` struct) and an `image_id`. Per frame:

1. **`spritebatch_push`** — appends to `input_buffer`; assigns `sort_bits = sort_id++`
   (a monotonically increasing push-order stamp, reset each flush).
2. **`spritebatch_tick`** — increments the decay `timestamp` on every atlas entry and
   lonely texture (called once per frame from `cute_app.cpp:536`).
3. **`spritebatch_defrag`** — atlas management:
   - *Decay atlases*: atlases with "too many" decayed entries are flushed — their live
     images are pushed back into the lonely buffer (without GPU textures) and the atlas
     texture is destroyed.
   - *Merge atlases*: pairs of atlases whose fill `volume_ratio` is below
     `ratio_to_merge_atlases` are flushed so their contents re-pack together.
   - *Decay lonely textures*: sorts lonely textures by timestamp and destroys decayed ones.
   - *Process input (skip mode)*: sprites whose image already lives in an atlas or premade
     are moved to `sprites` with resolved UVs; sprites referencing lonely/new images are
     **kept in `input_buffer`** (their placement may still change this frame).
   - *Build atlases*: while `lonely_count > lonely_buffer_count_till_flush`, packs lonely
     images into a new atlas (perimeter-sorted best-fit node packing into a
     `atlas_width × atlas_height` CPU buffer), generates a GPU texture, deletes the
     individual lonely textures.
4. **`spritebatch_flush`** — processes remaining input (now creating any missing lonely
   GPU textures), sorts `sprites`, then walks the array and invokes `submit_batch_fn`
   once per consecutive run of equal `texture_id`.

### 1.4 Cute Framework integration (`src/cute_draw.cpp`)

- Config (`s_init_sb`, `cute_draw.cpp:417`): 2048×2048 atlases, border pixels ON,
  `ticks_to_decay_texture = 100000` (~28 min at 60 FPS), and — importantly —
  `lonely_buffer_count_till_flush = 0`, meaning *every* image is atlased at the first
  defrag after it appears; lonely textures exist only transiently. No custom sorter is
  installed, so the internal merge sort runs.
- Callbacks: `cf_get_pixels` dispatches on `image_id` range (aseprite / custom / font
  glyph / easy sprite / premade) — see `cute_draw_internal.h:212`; textures are created
  through `cf_make_texture` + full `cf_texture_update`.
- Every drawable (sprites, text glyphs, shapes, polylines — everything) is pushed through
  the sprite batcher; shapes just don't sample the atlas. `s_draw_report`
  (`cute_draw.cpp:123`) expands each batch into 6 vertices per sprite and issues a draw.
- `spritebatch_defrag` + `spritebatch_flush` run at every "flush boundary" (render-state
  change, canvas blit, end of `cf_render_layers_to`), plus tick+defrag once per frame in
  `cute_app.cpp` (deferred to end-of-frame when `delay_defrag` is set by
  `cf_fetch_image` to keep ImGui texture IDs stable).

A key correctness constraint: CF is a painter's-algorithm 2D renderer. Batches are
submitted **in sprite order**, so the batcher must preserve push order; it only merges
*consecutive* sprites with the same texture. Draw-call reduction comes from atlasing
(most images share one atlas texture), not from reordering.

---

## 2. Bugs

Ordered by severity. Line numbers refer to `libraries/cute/cute_spritebatch.h` unless
stated otherwise.

### 2.1 [High] Sort comparator is inconsistent → transient wrong draw order

`cute_spritebatch.h:1063`:

```c
static int spritebatch_internal_sprite_less_than_or_equal(spritebatch_sprite_t* a, spritebatch_sprite_t* b)
{
	if (a->sort_bits < b->sort_bits) return 1;
	return a->texture_id <= b->texture_id;
}
```

Per the v1.07 changelog, `sort_bits` was repurposed to make the flush sort *stable*, i.e.
restore push order. But the comparator falls through to `texture_id` when
`a->sort_bits > b->sort_bits`, instead of returning 0. This makes it inconsistent: for
`X = {sort_bits 2, tex A}` and `Y = {sort_bits 1, tex B}` with `A <= B`, both `X <= Y`
and `Y <= X` hold. It is not a valid total order.

**When it matters:** after `spritebatch_defrag`'s input pass, `sprites` consists of two
concatenated runs — (run A) sprites whose images were already in atlases/premades,
(run B) sprites deferred to `spritebatch_flush` because their image was new or lonely.
The sort must interleave the runs back into push order. With the buggy comparator, a
run-A sprite pushed *after* a run-B sprite stays in front of it whenever its
`texture_id` compares `<=` — so on any frame where a new image first appears (sprite
loads, new animation frames, new text glyphs), sprites can render in the wrong z-order
for that frame. It also means the merge sort's behavior is formally undefined w.r.t.
ordering (though memory-safe).

**Fix** — restore pure push order (texture grouping must *not* reorder draws in a 2D
painter's-algorithm renderer; consecutive-run batching already handles grouping):

```c
static int spritebatch_internal_sprite_less_than_or_equal(spritebatch_sprite_t* a, spritebatch_sprite_t* b)
{
	return a->sort_bits <= b->sort_bits;
}
```

This also unlocks optimization P1 below (the sort becomes skippable in the common case).

### 2.2 [High, latent] Use-after-free: stale `lonely_textures` pointer in `spritebatch_defrag`

`cute_spritebatch.h:1946` caches the lonely map's dense item array:

```c
spritebatch_internal_lonely_texture_t* lonely_textures = ... spritebatch_map_items(&sb->sprites_to_lonely_textures);
```

Then line 1970 runs `spritebatch_internal_process_input(sb, 1)`, which **inserts** new
images into that same map (`spritebatch_internal_lonelybuffer_push` →
`spritebatch_map_insert`). If an insert triggers `spritebatch_map_expand_items` (count
crossing the item capacity, initially 1024), the items array is reallocated — but only
`lonely_count` is refreshed (line 1971); `lonely_textures` still points at freed memory
and is subsequently dereferenced at lines 1995 and 2002–2017.

**Trigger:** more than ~1024 simultaneously-lonely images processed in one defrag. In CF,
each unique rasterized text glyph is its own `image_id`, so a text-heavy first frame
(e.g. CJK fonts) or a bulk `cf_draw_prefetch` loop can realistically cross this.

**Fix:** re-fetch the pointer after `process_input` (and after any operation that can
insert into the map):

```c
spritebatch_internal_process_input(sb, 1);
lonely_count = spritebatch_map_count(&sb->sprites_to_lonely_textures);
lonely_textures = (spritebatch_internal_lonely_texture_t*)spritebatch_map_items(&sb->sprites_to_lonely_textures);
```

### 2.3 [Medium] Atlas decay ratio is inverted — one stale image flushes a whole atlas

`cute_spritebatch.h:1893`:

```c
float ratio;
if (!decayed_texture_count) ratio = 0;
else ratio = (float)texture_count / (float)decayed_texture_count;
if (ratio > ratio_to_decay_atlas) { /* flush whole atlas */ }
```

`texture_count / decayed_texture_count` is **≥ 1** whenever at least one texture has
decayed, and `ratio_to_decay_atlas` is validated to be in [0, 1] (`spritebatch_init:829`).
So the condition is equivalent to "flush the atlas as soon as *any single* texture
decays" — even if 99% of the atlas is live. The config docs say the intent is the
opposite: *"once ratio is less than `ratio_to_decay_atlas`, flush active textures in
atlas to lonely buffer"*, i.e. flush when the live fraction falls below the threshold.

Consequence: in a long play session, one sprite going unused for `ticks_to_decay_texture`
ticks tears down the whole 2048×2048 atlas; all live images get individual GPU textures
recreated in the next flush, then get re-packed into a new atlas — a large avoidable
GPU-churn spike (CF's 100000-tick decay makes this rare per-atlas, but it's guaranteed to
happen eventually in any game that ever stops drawing some image).

**Fix:**

```c
float live_ratio = 1.0f - (float)decayed_texture_count / (float)texture_count;
if (decayed_texture_count && live_ratio < ratio_to_decay_atlas) { /* flush */ }
```

### 2.4 [Medium] Lonely-texture quicksort corrupts its own ordering (index/pointer mismatch)

`cute_spritebatch.h:1758-1776`: the partition step swaps via **absolute map indices**
(`spritebatch_map_swap(lonely_table, i, low)`), but the right-hand recursion passes an
**offset item pointer**:

```c
spritebatch_internal_qsort_lonely(lonely_table, items + low + 1, count - 1 - low);
```

Inside that call, the predicate reads `items[i]` (= global element `low + 1 + i`) while
the swap still targets global elements `i` and `low` — i.e. it compares one pair of
elements and swaps a *different* pair, scrambling the already-partitioned left side.
The result is not sorted by timestamp.

The caller (defrag, lines 1949–1966) *assumes* sorted order: it scans to the first entry
with `timestamp >= ticks_to_decay_texture` and destroys **every entry from there to the
end**. With a mis-sorted array this can destroy still-live textures (they get silently
re-created next flush — GPU churn) and/or keep decayed ones alive (delayed VRAM reclaim).

**Fix:** recurse on index ranges over the same base pointer so predicate indices and swap
indices agree:

```c
static void spritebatch_internal_qsort_lonely(spritebatch_map_t* table, spritebatch_internal_lonely_texture_t* items, int lo, int hi)
{
	if (hi - lo <= 1) return;
	spritebatch_internal_lonely_texture_t pivot = items[hi - 1];
	int low = lo;
	for (int i = lo; i < hi - 1; ++i) {
		if (spritebatch_internal_lonely_pred(items + i, &pivot)) {
			spritebatch_map_swap(table, i, low);
			low++;
		}
	}
	spritebatch_map_swap(table, low, hi - 1);
	spritebatch_internal_qsort_lonely(table, items, lo, low);
	spritebatch_internal_qsort_lonely(table, items, low + 1, hi);
}
// call site: spritebatch_internal_qsort_lonely(&sb->sprites_to_lonely_textures, lonely_textures, 0, lonely_count);
```

### 2.5 [Low] `spritebatch_make_atlas`: wrong OOM check, missing check, and phantom `mem_ctx` identifier

- Line 1662: `SPRITEBATCH_CHECK(atlas_image_size, "out of mem");` checks the *size*
  (always non-zero) instead of the allocation. Should be
  `SPRITEBATCH_CHECK(atlas_pixels, "out of mem");` — as written, a failed 16MB staging
  alloc proceeds into `SPRITEBATCH_MEMSET(NULL, ...)`.
- `images_scratch` (line 1564) is allocated but never null-checked (`images` and `nodes`
  are), yet is written by the merge sort.
- Lines 1621, 1624, 1661, 1746–1749 pass a bare `mem_ctx` to
  `SPRITEBATCH_MALLOC`/`SPRITEBATCH_FREE`. **No such variable exists in the function** —
  it only compiles because the default macros discard the ctx argument. Any user who
  defines a custom allocator macro that actually *uses* its ctx parameter (the documented
  purpose of `allocator_context`) gets a compile error — or, if their macro stringizes
  or defaults it, allocations routed to the wrong heap. Should be `sb->mem_ctx`.
- Same-family nit: on the error path, `atlas_out->sprites_to_textures` /
  `texture_id` / `volume_ratio` are left uninitialized while the caller has already
  linked the atlas into the ring list — a subsequent defrag/term walks garbage. Worth
  initializing `atlas_out` at function entry.

### 2.6 [Low] Hash truncation can produce the reserved "empty" value

`spritebatch_map_hash` (`cute_spritebatch.h:572`) mixes the 64-bit key, asserts the
*64-bit* value is non-zero, then truncates: `return (unsigned)key;`. The low 32 bits can
be zero even when the 64-bit value isn't (~1 in 2³² per key). Since `key_hash == 0` marks
an empty slot everywhere (`find_slot`, `insert`, `expand_slots`), such an entry becomes
invisible: `find` misses it, and the insert-time duplicate assert can't see it either, so
a duplicate insert corrupts `base_count` bookkeeping. The original `hashtable.h` this map
replaced guarded against reserved values after truncation; that guard was lost.

**Fix:** clamp after truncation, e.g. `unsigned h = (unsigned)key; return h ? h : 1;`
(and drop the misleading 64-bit assert).

### 2.7 [Low] OOM-path issues in the hot loop

- `spritebatch_internal_get_pixels` (`cute_spritebatch.h:1104`): on realloc failure it
  returns early but has already freed the old buffer **and updated
  `pixel_buffer_size`** — the next call believes the (NULL) buffer is large enough and
  memsets it. Update the size only after a successful alloc, and null-check
  `pixel_buffer` at the top.
  Related nit: `pixel_buffer_size` is documented/initialized as a *pixel* count (1024,
  `spritebatch_init:865`) but compared/stored as *bytes* thereafter — harmless today,
  but the first growth reallocs from 4096 usable bytes down to less than it had.
- `spritebatch_push` (`cute_spritebatch.h:992`) ignores
  `spritebatch_internal_fill_internal_sprite`'s failure return (0 = grow failed) and
  writes `input_buffer[input_count++]` anyway → heap overflow under OOM. (Also,
  `spritebatch_internal_append_sprite` is dead code duplicating the same line.)
- `spritebatch_defrag:1979`: the `atlas` malloc is unchecked before being linked into
  the ring list.

### 2.8 [Info] Debug asserts misfire for large premade sub-images

`spritebatch_internal_fill_internal_sprite` (`cute_spritebatch.h:961`) asserts
`sprite.w/h <= atlas_width/height_in_pixels` for *every* pushed sprite. Premade-atlas
sub-images never enter the internal 2048×2048 atlases, so a premade atlas larger than
2048 with a big sub-image (registered through `cf_register_premade_atlas`) trips the
assert spuriously in debug builds. The check should be skipped for image ids resolved as
premades (or moved to where lonely textures are actually created).

Similarly, `spritebatch_init:807` uses bitwise `|` in `if (!config | !sb)` — works, but
should be `||`.

---

## 3. Performance & memory optimization opportunities

### P1. Skip the flush sort when sprites are already in order (biggest CPU win)

With CF's config every sprite in `spritebatch_flush` is 320 bytes
(`BatchGeometry` = 272 bytes, measured). The internal merge sort copies the whole array
to scratch and merges it back — **~640 bytes of memcpy per sprite per flush**, and CF
flushes multiple times per frame (per render-state change / canvas blit / layer range).

The punchline: in the steady state (no new images this frame) the array is *already
sorted* — `sort_bits` are assigned in push order and defrag/flush appended everything in
push order — so the entire sort is a very expensive identity operation. Only frames where
defrag deferred new-image sprites produce two ordered runs that need re-interleaving.

After fixing the comparator (bug 2.1), add an O(n) pre-pass:

```c
void spritebatch_internal_sort_sprites(spritebatch_t* sb)
{
	if (sb->sprites_sorter_callback) { sb->sprites_sorter_callback(sb->sprites, sb->sprite_count); return; }
	int sorted = 1;
	for (int i = 1; i < sb->sprite_count; ++i) {
		if (sb->sprites[i - 1].sort_bits > sb->sprites[i].sort_bits) { sorted = 0; break; }
	}
	if (!sorted) spritebatch_internal_merge_sort(sb->sprites, sb->sprites_scratch, sb->sprite_count);
}
```

(Even on the unsorted frames, the input is exactly two sorted runs, so a single-pass
merge at the run boundary would beat a full merge sort — but the sortedness check alone
captures nearly all of the win.)

### P2. Allocate `sprites_scratch` lazily

`sprites_scratch` permanently doubles the sprite buffer's memory
(320 B × capacity; a game that ever spikes to 16k sprites in one flush holds 2 × 5 MB
forever). With P1 the scratch buffer is needed only on the rare unsorted frames — allocate
it on first use (and it's already unused entirely when a user sorter callback is set;
today `spritebatch_init` correctly skips it, but the grow path in
`spritebatch_internal_push_sprite:1271-1288` still frees + reallocs both).

### P3. Shrink `BatchGeometry` (CF-side; multiplies every other cost)

Every sprite is copied ~4–5 times per frame (`CF_Command::items` → `input_buffer` →
`sprites` [→ scratch → back] → vertex expansion), so struct size is the single biggest
lever on batcher CPU/bandwidth. `BatchGeometry` (`cute_draw_internal.h:32`) carries the
union of all shape types' fields: `box[4]` + `boxH[4]` (64 B), `shape[8]` (64 B),
`tri_colors`/`tri_attributes` (60 B), `text_colors` (16 B) — largely mutually exclusive
by `type`. Converting the type-specific payloads into a `union` keyed on `type` would cut
roughly 100–130 bytes (~35–40%) per sprite with purely mechanical code changes, shrinking
`spritebatch_sprite_t` from 320 B toward ~200 B and speeding every copy, the sort, and
cache behavior in `s_draw_report`.

### P4. Reuse the atlas staging buffer

`spritebatch_make_atlas` (`cute_spritebatch.h:1661`) mallocs, memsets, fills, and frees a
full `atlas_w × atlas_h × 4` CPU buffer (16 MB at 2048²) on **every** atlas build. With
CF's `lonely_buffer_count_till_flush = 0`, an atlas build happens on the first defrag
after *any* new image appears, and atlas merging rebuilds two atlases' worth of content —
so during asset-streaming/loading phases this 16 MB alloc+memset can run many times per
second. Cache the buffer in `spritebatch_t` (grow-only, freed in `spritebatch_term`),
like `pixel_buffer` already is.

Related CF-side tuning: because `lonely_buffer_count_till_flush = 0`, a *single* new
image triggers a full atlas build containing one image, which then gets merge-flushed and
rebuilt again once more images arrive. During bulk loads, prefetching
(`cf_draw_prefetch`) everything before the next defrag — or temporarily raising the
lonely threshold during load screens — collapses N rebuild+merge cycles into one.

### P5. Right-size the premades map

`spritebatch_init:870` pre-sizes `sprites_to_premades` for 10 240 items:
16 384 × (8 + 4 + 32) bytes of item storage + 16 384 slots × 12 bytes ≈ **0.9 MB
allocated up front in every app**, even though most games never register a premade atlas.
The map already grows dynamically — initialize it at 64 (or lazily on first
`spritebatch_register_premade_atlas`).

### P6. Smaller items

- `spritebatch_internal_get_pixels:1115` memsets the entire staging buffer before the
  user callback fills `w*h*stride` of it; in border-pixel mode the border is explicitly
  re-cleared afterwards (lines 1138–1144), making the full memset redundant — clearing
  is only needed when the callback can't supply pixels.
- `volume_ratio` is computed once at atlas creation and never updated as entries decay,
  so merge decisions run on stale data; recomputing it from
  `sprites_to_textures` during the decay scan (the loop already visits every entry)
  would make merging effective for long-lived atlases.
- `spritebatch_defrag` is invoked at every flush boundary (`cute_draw.cpp:3514,3564,3616`)
  plus once per frame (`cute_app.cpp:537`); an early-out ("no lonely textures, no decay
  candidates, no merges") would make the extra calls near-free.
- Batcher allocations bypass `cf_alloc`: CF overrides allocators for `cute_png` but not
  `SPRITEBATCH_MALLOC`/`SPRITEBATCH_FREE` (`cute_draw.cpp:27`), contrary to the project
  convention (AGENTS.md "use cf_alloc/cf_free, not malloc/free") — one `#define` pair
  fixes it (after the `mem_ctx` bug in 2.5, for correctness of the ctx argument).

---

## 4. Suggested fix priority

1. **2.1** comparator fix + **P1** sortedness early-out — one-line correctness fix that
   removes a per-frame visual glitch risk and, with the early-out, the largest steady-state
   CPU cost in the batcher.
2. **2.2** stale `lonely_textures` re-fetch — trivial fix for a real crash.
3. **2.3** decay-ratio inversion and **2.4** qsort index fix — both directly reduce
   texture churn / incorrect texture destruction.
4. **P4/P5** staging-buffer reuse and premades map sizing — easy, measurable memory wins.
5. **2.5–2.8, P2, P3, P6** — hardening and larger refactors as time allows.

Since the vendored file matches upstream `cute_headers`, items 1–3 are good candidates to
submit upstream as well.
