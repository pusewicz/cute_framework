cmake_minimum_required(VERSION 4.2)
project(cute_framework
	DESCRIPTION "The cutest framework out there for creating 2D games in C++!"
	HOMEPAGE_URL "https://github.com/RandyGaul/cute_framework"
	LANGUAGES C CXX
	VERSION 1.1.1
)

include(cmake/StandardProjectSettings.cmake)
include(GNUInstallDirs)
# These are needed for how we use FetchContent.
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Todo - Fix how turning some of these off breaks the build.
option(CF_FRAMEWORK_STATIC "Build static library for Cute Framework." ON)
option(CF_CUTE_SHADERC "Build cute-shaderc, an offline shader compiler." ON)
option(CF_FRAMEWORK_APPLE_FRAMEWORK "Build CF libraries as Apple Framework" OFF)

# Make sure all libraries are placed into the same output folder.
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif ()

include(cmake/CFPlatform.cmake)

# Disable annoying MSVC warnings. Intentionally directory-scoped (not just on
# `cute`) so it also reaches vendored deps like SDL3/PhysFS built in this tree.
if(MSVC)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_SECURE_NO_DEPRECATE)
	add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
	add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
	add_compile_options(/MP)
endif()

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/include/cute_version.h.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/cute_version.h"
	@ONLY
)
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/src/cute_version.cpp.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/cute_version.cpp"
	@ONLY
)

# Cute Framework shared library.
include(cmake/CFSources.cmake)

if(APPLE)
	set(CF_SRCS ${CF_SRCS}
		src/internal/cute_tls.m
	)
endif()

if(CF_FRAMEWORK_STATIC)
	add_library(cute STATIC ${CF_SRCS} ${CF_HDRS})
	target_compile_definitions(cute PUBLIC CF_STATIC)
else()
	add_library(cute SHARED ${CF_SRCS} ${CF_HDRS})
endif()
target_compile_definitions(cute PRIVATE CF_EXPORT)
# Must have at least C++20 and C23. Scoped to the cute target (not the whole
# tree) so vendored deps like SDL3/PhysFS/s2n build with their own standard.
target_compile_features(cute PUBLIC cxx_std_20 c_std_23)

if(EMSCRIPTEN)
	message(STATUS "Configuring CF for Web (Emscripten)")

	# Emscripten forces off a couple options that don't make sense for web builds.
	# (CF_FRAMEWORK_STATIC is already forced ON by cmake/CFPlatform.cmake, which
	# also makes PhysFS build static below.)
	set(CF_CUTE_SHADERC OFF CACHE BOOL "" FORCE)
	set(CF_FRAMEWORK_APPLE_FRAMEWORK OFF CACHE BOOL "" FORCE)

	target_compile_options(cute PUBLIC -sUSE_SDL=3)
	target_compile_options(cute PRIVATE -gsplit-dwarf)

	target_link_options(cute INTERFACE
		-sUSE_SDL=3
		-sUSE_WEBGL2=1 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2
		-sFULL_ES3=1
		-sALLOW_MEMORY_GROWTH=1
		-sFILESYSTEM=1
		-sASYNCIFY=1
		-sASYNCIFY_STACK_SIZE=2040000
		# Skip ASYNCIFY instrumentation for functions proven never to suspend
		# (never reach cf_sleep / a coroutine yield / a fiber swap). This severs
		# the allocator hub (cf_alloc & co, an indirect call) and the cspv shader
		# compiler from the instrumented set, shrinking the wasm-opt link pass and
		# the output module. See cmake/asyncify_remove.txt for the list + contract.
		-sASYNCIFY_REMOVE=@${CMAKE_CURRENT_SOURCE_DIR}/cmake/asyncify_remove.txt
	)

	target_compile_options(cute PRIVATE -O1 -fno-rtti -fno-exceptions)
endif()

if(ANDROID)
	message(STATUS "Configuring CF for Android")

	# Android-specific options.
	# (CF_FRAMEWORK_STATIC is already forced ON by cmake/CFPlatform.cmake, which
	# also makes PhysFS build static below.)
	set(CF_CUTE_SHADERC OFF CACHE BOOL "" FORCE)
	set(CF_FRAMEWORK_APPLE_FRAMEWORK OFF CACHE BOOL "" FORCE)

	# Link Android-specific libraries.
	# GLESv3 needed for imgui OpenGL3 backend (VAO functions).
	target_link_libraries(cute PUBLIC android log GLESv3 EGL)
endif()

if(APPLE)
	find_library(IOKIT IOKit)
	find_library(FOUNDATION Foundation)
	find_library(SECURITY Security)
	find_library(QUARTZCORE QuartzCore)
	find_library(METAL Metal)
	find_library(METALKIT MetalKit)
	find_library(NETWORK Network)
	target_link_libraries(cute PUBLIC ${IOKIT} ${FOUNDATION} ${SECURITY} ${QUARTZCORE} ${METAL} ${METALKIT} ${NETWORK})
endif()

if(LINUX)
	find_package(OpenGL REQUIRED)
	target_link_libraries(cute PUBLIC ${OPENGL_LIBRARIES})
endif()

# MinGW needs these since gcc ignores pragma links in C/C++.
if(WINDOWS AND NOT MSVC)
	target_link_libraries(cute PUBLIC ws2_32 secur32 shlwapi)
endif()

# Deprecated no-op, kept so existing user CMake scripts don't break. CF no longer
# ships or uses dxcompiler.dll/dxil.dll -- D3D12 shaders go through the system's
# FXC (d3dcompiler_47.dll) fed by cute_spirv's HLSL transpiler backend.
function(target_dxc_copy TARGET)
endfunction()

# PhysFS (always), s2n (Linux only), and SDL3 (everywhere but Emscripten).
include(cmake/CFDependencies.cmake)

add_subdirectory(tools)

# Some platform specific settings.
if(MINGW)
	target_link_libraries(cute PUBLIC d3d11 crypt32)
elseif(WINDOWS)
	target_link_libraries(cute PUBLIC crypt32)
elseif(LINUX)
	target_compile_options(cute PRIVATE -msse4.1)
elseif(APPLE)
	if(CF_FRAMEWORK_APPLE_FRAMEWORK)
		set_target_properties(cute PROPERTIES FRAMEWORK TRUE)
		if (CF_FRAMEWORK_STATIC)
		set_target_properties(cute PROPERTIES
			FRAMEWORK_VERSION "${PROJECT_VERSION}"
			MACOSX_FRAMEWORK_IDENTIFIER "com.cuteframework.static"
		)
		else()
		set_target_properties(cute PROPERTIES
			FRAMEWORK_VERSION "${PROJECT_VERSION}"
			MACOSX_FRAMEWORK_IDENTIFIER "com.cuteframework.shared"
		)
		endif()
	endif()
endif()

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	option(CF_FRAMEWORK_BUILD_TESTS "Build the cute framework unit tests." ON)
	option(CF_FRAMEWORK_BUILD_SAMPLES "Build the cute framework sample programs." ON)
	option(CF_BUILD_DOCSPARSER "Build the docsparser documentation generator." ON)

	# Cute unit tests executable (optional, defaulted to also build).
	if (CF_FRAMEWORK_BUILD_TESTS)
		enable_testing()
		add_subdirectory(test)
	endif()

	# Cute sample programs (optional, defaulted to also build).
	if (CF_FRAMEWORK_BUILD_SAMPLES)
		add_subdirectory(samples)
	endif()
endif()

# Propogate public headers to other cmake scripts including this subdirectory.
target_include_directories(cute PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_include_directories(cute PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libraries>)
target_include_directories(cute PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libraries/imgui>)
target_include_directories(cute PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(cute PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tools)

# Install target
install(TARGETS cute
	BUNDLE DESTINATION .
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
