cmake_minimum_required(VERSION 2.8.4)
project(cute_framework)

# Force 64bit builds.
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Only 64-bit build is supported.")
    return()
endif()

# Common directories for compiler/linker path.
include_directories(src libraries test)
link_directories(shared_libs)

# PhysicsFS shared library.
set(PHYSFS_SRCS
	libraries/physfs/physfs_archiver_7z.c
	libraries/physfs/physfs_archiver_dir.c
	libraries/physfs/physfs_archiver_grp.c
	libraries/physfs/physfs_archiver_hog.c
	libraries/physfs/physfs_archiver_iso9660.c
	libraries/physfs/physfs_archiver_mvl.c
	libraries/physfs/physfs_archiver_qpak.c
	libraries/physfs/physfs_archiver_slb.c
	libraries/physfs/physfs_archiver_unpacked.c
	libraries/physfs/physfs_archiver_vdf.c
	libraries/physfs/physfs_archiver_wad.c
	libraries/physfs/physfs_archiver_zip.c
	libraries/physfs/physfs_byteorder.c
	libraries/physfs/physfs_casefolding.h
	libraries/physfs/physfs_internal.h
	libraries/physfs/physfs_lzmasdk.h
	libraries/physfs/physfs_miniz.h
	libraries/physfs/physfs_platform_haiku.cpp
	libraries/physfs/physfs_platform_os2.c
	libraries/physfs/physfs_platform_posix.c
	libraries/physfs/physfs_platform_qnx.c
	libraries/physfs/physfs_platform_unix.c
	libraries/physfs/physfs_platform_windows.c
	libraries/physfs/physfs_platform_winrt.cpp
	libraries/physfs/physfs_platforms.h
	libraries/physfs/physfs_unicode.c
	libraries/physfs/physfs.c
	libraries/physfs/physfs.h
)

if(APPLE)
	set(PHYSFS_SRCS ${PHYSFS_SRCS}
		libraries/physfs/physfs_platform_apple.m
	)
endif()

add_library(physfs STATIC ${PHYSFS_SRCS})

# Cute Framework shared library.
set(CUTE_SRCS
	src/cute_app.cpp
	src/cute_audio.cpp
	src/cute_buffer.cpp
	src/cute_circular_buffer.cpp
	src/cute_client.cpp
	src/cute_clipboard.cpp
	src/cute_concurrency.cpp
	src/cute_crypto.cpp
	src/cute_error.cpp
	src/cute_file_system.cpp
	src/cute_handle_table.cpp
	src/cute_input.cpp
	src/cute_net.cpp
	src/cute_server.cpp
	src/cute_timer.cpp
	src/cute_version.cpp
	src/cute_memory_pool.cpp

	libraries/glad/glad.c
)

set(CUTE_HDRS
	src/cute_alloc.h
	src/cute_app.h
	src/cute_audio.h
	src/cute_buffer.h
	src/cute_c_runtime.h
	src/cute_circular_buffer.h
	src/cute_client.h
	src/cute_clipboard.h
	src/cute_concurrency.h
	src/cute_crypto.h
	src/cute_crypto_utils.h
	src/cute_defines.h
	src/cute_error.h
	src/cute_file_system.h
	src/cute_file_system_utils.h
	src/cute_handle_table.h
	src/cute_input.h
	src/cute_net.h
	src/cute_server.h
	src/cute_timer.h
	src/cute_version.h
	src/cute_memory_pool.h

	src/internal/cute_app_internal.h
	src/internal/cute_audio_internal.h
	src/internal/cute_crypto_internal.h
	src/internal/cute_defines_internal.h
	src/internal/cute_file_system_internal.h
	src/internal/cute_input_internal.h
	src/internal/cute_net_internal.h

	libraries/glad/glad.h
)

add_library(cute SHARED ${CUTE_SRCS} ${CUTE_HDRS})
target_link_libraries(cute PRIVATE SDL2 physfs libsodium)
target_compile_definitions(cute PRIVATE CUTE_EXPORT)

# Cute unit tests executable (optional, defaulted to also build).
option(CUTE_FRAMEWORK_BUILD_TESTS "Build the cute framework unit tests." ON)
if (CUTE_FRAMEWORK_BUILD_TESTS)

	set(CUTE_TEST_SRCS test/main.cpp)
	set(CUTE_TEST_HDRS
		test/test_circular_buffer.h
		test/test_crypto.h
		test/test_handle.h
		test/test_harness.h
		test/test_nonce_buffer.h
		test/test_packet_queue.h
		test/test_socket.h
		test/test_connect_token.h
	)

	add_executable(tests ${CUTE_TEST_SRCS} ${CUTE_TEST_HDRS})
	target_link_libraries(tests PRIVATE cute)

	# Copy over prebuilt binaries to output directory.
	add_custom_command(TARGET tests POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/shared_libs" $<TARGET_FILE_DIR:tests>)

	# For convenience make tests the startup project in Visual Studio.
	if (MSVC)
		set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT tests)
	endif()
endif()