# FetchContent'd dependencies for the cute target: PhysFS (always), s2n
# (Linux only), and SDL3 (everywhere except Emscripten, which provides its
# own port). Each block owns its cache-variable setup, fetch, and link step
# together so the dependency is self-contained and reorderable.

# PhysicsFS for virtual file system and archive support.
if(CF_FRAMEWORK_STATIC)
	set(PHYSFS_BUILD_SHARED OFF CACHE BOOL "Build PhysFS as shared library")
	set(PHYSFS_BUILD_STATIC ON CACHE BOOL "Build PhysFS as static library")
else()
	set(PHYSFS_BUILD_SHARED ON CACHE BOOL "Build PhysFS as shared library")
	set(PHYSFS_BUILD_STATIC OFF CACHE BOOL "Build PhysFS as static library")
endif()
set(PHYSFS_BUILD_TEST OFF CACHE BOOL "Do not build PhysFS tests.")
set(PHYSFS_BUILD_DOCS OFF CACHE BOOL "Do not build PhysFS docs.")
FetchContent_Declare(
	physfs
	GIT_REPOSITORY https://github.com/icculus/physfs.git
	GIT_TAG        9d18d36b5a5207b72f473f05e1b2834e347d8144
	UPDATE_DISCONNECTED TRUE
	EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(physfs)
if(CF_FRAMEWORK_STATIC)
	target_link_libraries(cute PUBLIC PhysFS::PhysFS-static)
else()
	target_link_libraries(cute PUBLIC PhysFS::PhysFS)
endif()

# Add s2n for Linux builds.
if(LINUX)
	# Ensure tests are not built.
	set(BUILD_TESTING OFF CACHE BOOL "Disable building s2n tests." FORCE)

	FetchContent_Declare(
		s2n
		URL https://github.com/aws/s2n-tls/archive/refs/tags/v1.6.3.zip
		DOWNLOAD_EXTRACT_TIMESTAMP ON
		UPDATE_DISCONNECTED TRUE
		EXCLUDE_FROM_ALL
	)
	FetchContent_MakeAvailable(s2n)
	target_link_libraries(cute PUBLIC s2n)
endif()

if(NOT EMSCRIPTEN) # Emscripten provides it's own SDL3.
	# SDL for platform support.
	set(SDL_REQUIRED_VERSION 3.4.0)
	# Just don't build the shared library at all, it's not needed.
	set(SDL_SHARED_ENABLED_BY_DEFAULT OFF)
	if(CF_FRAMEWORK_STATIC)
		set(SDL_STATIC ON CACHE BOOL "Build SDL as a static library")
		set(SDL_SHARED OFF CACHE BOOL "Do not build SDL as a shared library")
	else()
		set(SDL_STATIC OFF CACHE BOOL "Do not build SDL as a static library")
		set(SDL_SHARED ON CACHE BOOL "Build SDL as a shared library")
	endif()
	set(SDL_TEST_LIBRARY OFF CACHE BOOL "Do not build SDL tests")
	set(SDL_EXAMPLES OFF CACHE BOOL "Do not build SDL examples")
	FetchContent_Declare(
		SDL3
		URL https://github.com/libsdl-org/SDL/releases/download/release-${SDL_REQUIRED_VERSION}/SDL3-${SDL_REQUIRED_VERSION}.zip
		DOWNLOAD_EXTRACT_TIMESTAMP ON
		UPDATE_DISCONNECTED TRUE
		EXCLUDE_FROM_ALL
	)
	FetchContent_MakeAvailable(SDL3)
	if(CF_FRAMEWORK_STATIC)
		target_link_libraries(cute PUBLIC SDL3::SDL3-static)
	else ()
		target_link_libraries(cute PUBLIC SDL3::SDL3)
	endif()
	target_include_directories(cute PUBLIC ${SDL3_INCLUDE_DIRS})

	# Build-time tool to extract D3D12 struct layout offsets from SDL3 private headers.
	if(WIN32)
		set(D3D12_LAYOUT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/d3d12_layout.gen.h")
		file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
		add_executable(gen_d3d12_layout EXCLUDE_FROM_ALL tools/gen_d3d12_layout.c)
		add_custom_command(
			OUTPUT ${D3D12_LAYOUT_HEADER}
			COMMAND gen_d3d12_layout
				"${sdl3_SOURCE_DIR}/src/gpu/SDL_sysgpu.h"
				"${sdl3_SOURCE_DIR}/src/gpu/d3d12/SDL_gpu_d3d12.c"
				"${D3D12_LAYOUT_HEADER}"
			DEPENDS gen_d3d12_layout
				"${sdl3_SOURCE_DIR}/src/gpu/SDL_sysgpu.h"
				"${sdl3_SOURCE_DIR}/src/gpu/d3d12/SDL_gpu_d3d12.c"
			COMMENT "Generating D3D12 struct layout offsets"
		)
		add_custom_target(gen_d3d12_layout_target DEPENDS ${D3D12_LAYOUT_HEADER})
		add_dependencies(cute gen_d3d12_layout_target)
		target_include_directories(cute PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
	endif()
endif()
