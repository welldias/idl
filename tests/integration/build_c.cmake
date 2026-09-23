# Convention-based build of a C project, incremental build and compile_commands.json.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

use_fixture(c_basic)
set(bin "${PROJECT_DIR}/build/debug/c_basic${EXE}")

idl("${PROJECT_DIR}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Building c_basic (debug)")
expect_contains("${IDL_OUTPUT}" "Compiling src/main.c")
expect_contains("${IDL_OUTPUT}" "Compiling src/util.c")
expect_contains("${IDL_OUTPUT}" "Linking build/debug/c_basic")

run_program("${bin}" ARGS a b)
expect_contains("${PROGRAM_OUTPUT}" "sum=5 profile=debug args=2")
expect_contains("${PROGRAM_OUTPUT}" "arg[2]=b")

# Sources stay clean: object files only in build/.
file(GLOB_RECURSE stray "${PROJECT_DIR}/src/*.o")
if(stray)
	message(FATAL_ERROR "Object files outside build/: ${stray}")
endif()
expect_exists("${PROJECT_DIR}/build/debug/obj/src/main.c.o")

# Nothing changed: nothing is rebuilt.
idl("${PROJECT_DIR}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Nothing to do")
expect_not_contains("${IDL_OUTPUT}" "Compiling")

# Only util.c changed.
touch_later("${PROJECT_DIR}/src/util.c")
idl("${PROJECT_DIR}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Compiling src/util.c")
expect_not_contains("${IDL_OUTPUT}" "Compiling src/main.c")
expect_contains("${IDL_OUTPUT}" "Linking")

# Header changed: rebuild whoever includes it (both).
touch_later("${PROJECT_DIR}/src/util.h")
idl("${PROJECT_DIR}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Compiling src/util.c")
expect_contains("${IDL_OUTPUT}" "Compiling src/main.c")

# Changing flags (project.yml) also rebuilds.
file(WRITE "${PROJECT_DIR}/project.yml" "project:\n  name: c_basic\nbuild:\n  defines: [EXTRA=1]\n")
idl("${PROJECT_DIR}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Compiling src/main.c")
expect_contains("${IDL_OUTPUT}" "Compiling src/util.c")

# Binary deleted: only link again.
file(REMOVE "${bin}")
idl("${PROJECT_DIR}" ARGS build)
expect_not_contains("${IDL_OUTPUT}" "Compiling")
expect_contains("${IDL_OUTPUT}" "Linking")
expect_exists("${bin}")

# A new source joins the build.
file(WRITE "${PROJECT_DIR}/src/novo.c" "int novo(void) { return 1; }\n")
idl("${PROJECT_DIR}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Compiling src/novo.c")

# Valid compile_commands.json, with one entry per source.
set(json_file "${PROJECT_DIR}/build/compile_commands.json")
expect_exists("${json_file}")
file(READ "${json_file}" json)
string(JSON entries LENGTH "${json}")
if(NOT entries EQUAL 3)
	message(FATAL_ERROR "compile_commands.json should have 3 entries, has ${entries}")
endif()
string(JSON first_file GET "${json}" 0 file)
string(JSON first_arg GET "${json}" 0 arguments 0)
string(JSON directory GET "${json}" 0 directory)
expect_contains("${first_file}" "src/")
expect_contains("${directory}" "c_basic")
if(first_arg STREQUAL "")
	message(FATAL_ERROR "compile_commands.json has no compiler")
endif()

# idl run forwards the arguments after --.
idl("${PROJECT_DIR}" ARGS run -- x "with space ç")
expect_contains("${IDL_OUTPUT}" "args=2")
expect_contains("${IDL_OUTPUT}" "arg[2]=with space ç")

# --project (and the old -p) point to another directory.
idl("${WORK_DIR}" ARGS build --project c_basic)
expect_contains("${IDL_OUTPUT}" "Nothing to do")
idl("${WORK_DIR}" ARGS build -p c_basic)
expect_contains("${IDL_OUTPUT}" "Nothing to do")
