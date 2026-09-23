cmake_minimum_required(VERSION 4.0)
project(idl)

set(CMAKE_C_STANDARD 23)
set(BUILD_SHARED_LIBS OFF)
set(BUILD_TESTING OFF)

file(GLOB_RECURSE SOURCES src/*.c)

if(WIN32)
	set(EXCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/core/platform_unix.c")
else()
	set(EXCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/core/platform_win.c")
endif()

list(REMOVE_ITEM SOURCES ${EXCLUDE_FILES})

include(FetchContent)

# Make the dependencies' option() calls honor variables already set here (e.g. BUILD_TESTING).
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# libuv: async I/O and platform utilities, downloaded when the build is configured
FetchContent_Declare(
	libuv
	URL https://github.com/libuv/libuv/archive/refs/tags/v1.52.1.tar.gz
	URL_HASH SHA256=478baf2599bfbc882c355288c9cb6f92e0e7dda435fa04031fa5b607cf3f414c
	DOWNLOAD_EXTRACT_TIMESTAMP TRUE
	EXCLUDE_FROM_ALL
)
set(LIBUV_BUILD_SHARED OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(libuv)

# libyaml: YAML reading and writing, downloaded when the build is configured
FetchContent_Declare(
	libyaml
	URL https://github.com/yaml/libyaml/archive/refs/tags/0.2.5.tar.gz
	URL_HASH SHA256=fa240dbf262be053f3898006d502d514936c818e422afdcf33921c63bed9bf2e
	DOWNLOAD_EXTRACT_TIMESTAMP TRUE
	EXCLUDE_FROM_ALL
)
# libyaml's CMakeLists.txt requires CMake 3.0, which CMake 4 no longer accepts.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
FetchContent_MakeAvailable(libyaml)
unset(CMAKE_POLICY_VERSION_MINIMUM)

# All the code lives in the idl_core library, which the tests also use.
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c")
add_library(idl_core STATIC ${SOURCES})
target_include_directories(idl_core PUBLIC src)
target_link_libraries(idl_core PUBLIC uv_a yaml)
target_compile_definitions(idl_core PUBLIC DEBUG=0)

add_executable(idl src/main.c)
target_link_libraries(idl idl_core)

option(IDL_BUILD_TESTS "Build the idl tests (ctest)" ON)
if(IDL_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()