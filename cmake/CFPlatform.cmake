# Platform detection.
# Put Emscripten first so it doesn't fall into the generic UNIX/Linux path.
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten" OR DEFINED EMSCRIPTEN)
	set(EMSCRIPTEN TRUE)

elseif(CMAKE_SYSTEM_NAME STREQUAL "Android" OR DEFINED ANDROID)
	set(ANDROID TRUE)

elseif(WIN32)
	set(WINDOWS TRUE)

elseif(APPLE)
	enable_language(OBJC)
	if(CMAKE_SYSTEM_NAME MATCHES ".*MacOS.*" OR CMAKE_SYSTEM_NAME MATCHES ".*Darwin.*")
		set(MACOSX TRUE)
	elseif(CMAKE_SYSTEM_NAME MATCHES ".*iOS.*")
		set(IOS TRUE)
	else()
		message(FATAL_ERROR "No supported Apple platform detected.")
	endif()

elseif(UNIX AND NOT APPLE)
	if(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
		set(LINUX TRUE)
	else()
		message(FATAL_ERROR "No supported UNIX platform detected.")
	endif()

else()
	message(FATAL_ERROR "No supported platform detected.")
endif()

# Emscripten and Android only ever ship a statically-linked cute (this is what
# CI exercises, and neither the Emscripten SIDE_MODULE nor Android .so path is
# wired up here). Force it before add_library() picks STATIC vs SHARED so the
# rest of the dependency setup (PhysFS in particular) doesn't need its own
# copy of this override.
if(EMSCRIPTEN OR ANDROID)
	set(CF_FRAMEWORK_STATIC ON CACHE BOOL "Build static library for Cute Framework." FORCE)
endif()
