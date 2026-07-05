# AGENTS.md

This document provides essential context for AI assistants (Claude, GPT, Copilot, Cursor, etc.) working with the Cute Framework codebase.

## Project Overview

Cute Framework is a **2D game development framework** written in C/C++ designed to be portable, lightweight, and easy to use. It provides a complete solution for creating 2D games across multiple platforms including Windows, macOS, Linux, iOS, Android, and web browsers (via Emscripten).

**Key Features**:
- Cross-platform support with SDL3 as the platform abstraction layer
- Modern C++20 codebase with CMake build system (requires CMake 3.14+)
- Comprehensive graphics pipeline with shader cross-compilation
- Full audio, input, networking, and file system support
- 40+ sample programs demonstrating framework capabilities
- Extensive documentation at https://randygaul.github.io/cute_framework/

## Code Style and Conventions

When contributing to Cute Framework, follow these established coding conventions:

### Naming Conventions

**Functions**:
- Public API: `cf_` prefix with snake_case (e.g., `cf_make_app`, `cf_draw_sprite`)
- Internal functions: `s_` prefix for static functions (e.g., `s_init_video`)
- C++ namespace functions: snake_case without prefix (e.g., `sprite_draw`, `app_update`)

**Types and Structures**:
- Structs: `CF_` prefix with PascalCase (e.g., `CF_Sprite`, `CF_Canvas`, `CF_Material`)
- Enums: `CF_` prefix with UPPER_SNAKE_CASE (e.g., `CF_DISPLAY_ORIENTATION_LANDSCAPE`)
- Typedefs: Same as the underlying type convention

**Macros and Constants**:
- Macros: `CF_` prefix with UPPER_SNAKE_CASE (e.g., `CF_INLINE`, `CF_GLOBAL`, `CF_STATIC_ASSERT`)
- Constants: UPPER_SNAKE_CASE or follow enum convention

**Variables**:
- Local variables: snake_case (e.g., `display_id`, `rect`)
- Member variables: snake_case
- Global variables: Typically avoided, but when necessary use descriptive names

### File Organization

**Header Files** (.h):
- Include guards: `CF_[FILENAME]_H` (e.g., `CF_GRAPHICS_H`)
- Copyright notice at the top
- C API wrapped in `extern "C"` for C++ compatibility
- Comprehensive Doxygen-style documentation for public APIs
- Related functions grouped together with `@related` tags

**Implementation Files** (.cpp/.c):
- Copyright notice at the top
- System/external includes first, then internal headers
- Static functions and variables at file scope with `s_` prefix
- Logical grouping of related functions

### Documentation Style

**API Documentation**:
```c
/**
 * @function function_name
 * @category category_name
 * @brief    Brief one-line description.
 * @param    param_name    Description of parameter.
 * @return   Description of return value.
 * @remarks  Additional details and usage notes.
 * @related  Related functions for cross-reference.
 */
```

### Code Formatting

**Indentation and Spacing**:
- Use tabs for indentation
- Opening braces on the same line for functions and control structures
- Space after control keywords: `if (`, `while (`, `for (`
- No space after function names: `function_name(`

**Line Length**:
- Generally keep lines under 120 characters
- Break long function signatures across multiple lines if needed

### C++ Specific Conventions

**Namespace**:
- Primary namespace: `Cute` (using `namespace Cute;` in implementation files)
- C++ wrapper functions use snake_case without cf_ prefix

**Modern C++ Features**:
- Target C++20 standard
- Use `CF_INLINE` macro for inline functions (cross-platform compatibility)
- Prefer stack allocation and value semantics where appropriate

### Platform-Specific Code

**Conditional Compilation**:
- Use `#ifdef CF_WINDOWS`, `#ifdef CF_APPLE`, `#ifdef CF_LINUX`, `#ifdef CF_EMSCRIPTEN`
- Platform detection handled in CMakeLists.txt:30-55

**Portability Macros**:
- `CF_INLINE` - Platform-appropriate inline keyword
- `CF_GLOBAL` - Global variable declaration
- `CF_STATIC_ASSERT` - Compile-time assertions

### Memory Management

- Prefer stack allocation when possible
- Use `cf_alloc`/`cf_free` for dynamic allocation (not malloc/free)
- RAII patterns in C++ code
- Clear ownership semantics for allocated resources

### Error Handling

- Return `CF_Result` for operations that can fail
- Use `is_error(result)` to check for errors
- Provide meaningful error messages via result system

## Build Commands

### Building the Project
```bash
# Standard build (debug) - build/ is a flat directory, not build/debug or build/release
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Release build (use a separate dir if you want debug and release side by side)
cmake -B build-release -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build-release

# Web build (Emscripten)
emcmake cmake -B build-emscripten -S .
cmake --build build-emscripten

# Build with specific options
cmake -B build -S . \
  -DCF_FRAMEWORK_STATIC=ON \
  -DCF_RUNTIME_SHADER_COMPILATION=ON \
  -DCF_CUTE_SHADERC=ON

# Fast path: build only the library (skips samples/tests/tools)
cmake --build build --target cute
# Produces lib/libcute.a
```

### Running Tests
```bash
# Build and run all tests
cmake --build build
./build/tests

# The test executable uses pico_unit framework
# Test files are in test/ directory with pattern test_*.cpp
```

### Building Samples
```bash
# Samples are automatically built with the main project
# Run a specific sample (binary names are the sample dir name, no separator)
./build/basicsprite

# Key sample programs (40+ available):
# Basic: hello_triangle, basic_sprite, basic_input, basic_networking
# Graphics: instancing, indexed_rendering, stencil, render_to_texture
# Effects: metaballs, waves, fluid_sim, water, particles
# Games: spaceshooter, platformer, tetris
# ImGui: imgui, imgui_custom_font, imgui_backend
# Tools: docsparser (generates API documentation, source in tools/docs_parser.c)
```

### Gotchas
- **`clangd` can't resolve some includes**: `ckit.h`, `cute_net.h`, `cute_sync.h` live in
  `libraries/cute/` and clangd frequently fails to index them even though the CMake build is fine.
  Don't trust clangd diagnostics for these headers over an actual build.
- **Pre-existing warnings**: `cute_tls.h` has known enum-comparison warnings that predate any change
  you're making — they are not something you introduced.
- **X-macro enums**: many headers define enums via an `X`-macro pattern (e.g. `CF_*_DEFS`) so adding
  an enum value usually means updating one `#define ... X(...)` list, not a switch/case everywhere.
- **`cute_defines.h`**: included by nearly every header; it's the natural home for a new
  cross-cutting macro/utility rather than adding a new include everywhere.

## Architecture Overview

### Core Systems

**Graphics Pipeline**:
- SDL_gpu-based renderer
- OpenGL ES 3 renderer for web builds using Emscripten
- Shader system supports runtime compilation via the tools/ shader compiler (cute_shader.cpp/.h, cute_shaderc.cpp)
- Bytecode generation for cross-platform shader support

**Component Structure**:
- `src/cute_*.cpp` - Core framework components (app, audio, graphics, input, etc.)
- `src/internal/cute_*_internal.h` - Internal implementation headers
- `include/cute_*.h` - Public API headers
- Single header entry point: `include/cute.h`
- `libraries/` - External vendored dependencies (cimgui, imgui, glad, stb, etc.); single-header
  cute libs (ckit.h, cute_net.h, cute_sync.h, cute_aseprite.h, etc.) live in `libraries/cute/`

**Rendering Architecture**:
- Mesh system with vertex attributes (CF_MeshInternal)
- Buffer management (vertex, index, instance buffers)
- Canvas-based rendering with default and custom canvases
- Shader uniforms mapped from CF_ShaderInfo types

**ImGui Integration**:
- Wrapper at src/cute_imgui.cpp with internal header cute_imgui_internal.h

### Platform Support

The framework uses SDL3 as the platform abstraction layer and supports:
- Windows (D3D11/D3D12/Vulkan)
- macOS/iOS (Metal)
- Linux (Vulkan/OpenGL)
- Web (WebGL2/OpenGL ES3 via Emscripten)

Platform detection happens in CMakeLists.txt:30-55, with specific build configurations for each target.

### Dependencies

**External Libraries** (in libraries/):
- SDL3 (fetched via CMake)
- SDL3_shadercross (shader cross-compilation)
- PhysicsFS (virtual filesystem; CMake-fetched for the Emscripten build only, not vendored)
- imgui / cimgui (immediate mode GUI + C bindings)
- glad (OpenGL loader)
- `libraries/cute/` - single-header cute libs: ckit.h, cute_net.h, cute_sync.h, cute_aseprite.h,
  cute_c2.h, cute_png.h, cute_sound.h, cute_spritebatch.h, cute_tls.h

**Internal Libraries** (in src/internal/):
- yyjson (JSON parsing)
- Various internal headers for subsystems

### Shader System

The framework has a sophisticated shader compilation pipeline, implemented in `tools/`:
- Runtime compilation support (when CF_RUNTIME_SHADER_COMPILATION=ON)
- Offline compiler tool (cute-shaderc, source in tools/cute_shaderc.cpp)
- Cross-platform bytecode generation
- Builtin shaders in tools/builtin_shaders.h
- Bytecode cache in src/data/builtin_shaders_bytecode.h

### File Organization

**Directory Structure**:
```
cute_framework/
├── include/              # Public API headers (42 files)
│   ├── cute.h           # Single header entry point (includes all)
│   └── cute_*.h         # Individual subsystem headers
├── src/                 # Implementation files (33 .cpp files)
│   ├── cute_*.cpp       # Core framework components
│   ├── internal/        # Internal implementation headers (.h/.c/.m)
│   └── data/            # Generated/vendored data (e.g. builtin_shaders_bytecode.h)
├── libraries/           # External dependencies (cimgui, imgui, glad, dxc, stb, etc.)
│   └── cute/            # Single-header cute libs (ckit.h, cute_net.h, cute_sync.h, etc.)
├── tools/               # Dev tools: shader compiler, docs_parser, sample-page generator
├── samples/             # 40+ example programs
├── test/                # Unit tests (15+ modules using pico_unit)
├── docs/                # MkDocs documentation source
├── CMakeLists.txt       # Main build configuration
├── README.md            # Project overview
└── AGENTS.md            # This file - AI assistant guide
```

**Key Configuration Files**:
- `CMakeLists.txt:30-55` - Platform detection and configuration
- `mkdocs.yml` - Documentation site configuration
- `msvc2026.cmd` - Windows Visual Studio build helper
- `web.cmd` - Emscripten web build helper

### Documentation

**Building Documentation**:
```bash
# Generate API reference (binary is docsparser, source in tools/docs_parser.c)
./build/docsparser

# Serve documentation locally
mkdocs serve

# Build documentation site
mkdocs build
```

The API reference is available in the docs/ directory, when run using mkdocs and the docsparser binary.
Full documentation is available at https://randygaul.github.io/cute_framework/api_reference/.

### Testing

**Test Infrastructure**:
- Framework: pico_unit (lightweight C++ testing)
- Location: `test/` directory
- Pattern: `test_*.cpp` files
- Coverage: Core functionality, math, strings, collections, etc.

```bash
# Run all tests
./build/tests

# Tests are automatically built with the project
# Test results are printed to console with pass/fail status
```
