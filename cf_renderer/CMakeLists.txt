cmake_minimum_required(VERSION 3.14)
project(cf_renderer
	DESCRIPTION "Standalone 2D renderer extracted from Cute Framework, based on SDL3 GPU API"
	LANGUAGES C
	VERSION 1.0.0
)

# C99 standard
set(CMAKE_C_STANDARD 99)

# Options
option(CFR_BUILD_EXAMPLES "Build CF Renderer examples" ON)
option(CFR_BUILD_SHARED "Build CF Renderer as shared library" OFF)

# Find SDL3
find_package(SDL3 QUIET)

if(NOT SDL3_FOUND)
	message(STATUS "SDL3 not found locally, fetching from GitHub...")
	include(FetchContent)
	Set(FETCHCONTENT_QUIET FALSE)
	
	FetchContent_Declare(
		SDL3
		GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
		GIT_TAG main
		GIT_PROGRESS TRUE
	)
	
	FetchContent_MakeAvailable(SDL3)
endif()

# CF Renderer library
set(CFR_SOURCES
	src/cf_renderer.c
)

set(CFR_PUBLIC_HEADERS
	include/cf_renderer.h
)

if(CFR_BUILD_SHARED)
	add_library(cf_renderer SHARED ${CFR_SOURCES})
else()
	add_library(cf_renderer STATIC ${CFR_SOURCES})
endif()

target_include_directories(cf_renderer
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
)

target_link_libraries(cf_renderer
	PUBLIC
		SDL3::SDL3
)

# Set output directories
set_target_properties(cf_renderer PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Examples
if(CFR_BUILD_EXAMPLES)
	add_executable(simple_drawing examples/simple_drawing.c)
	target_link_libraries(simple_drawing PRIVATE cf_renderer SDL3::SDL3)
	
	set_target_properties(simple_drawing PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	)
endif()

# Installation
install(TARGETS cf_renderer
	EXPORT cf_renderer-targets
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
)

install(FILES ${CFR_PUBLIC_HEADERS}
	DESTINATION include/cf_renderer
)

install(EXPORT cf_renderer-targets
	FILE cf_renderer-targets.cmake
	NAMESPACE CFRenderer::
	DESTINATION lib/cmake/cf_renderer
)
