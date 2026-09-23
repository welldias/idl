# CC/CXX choose the compiler; switching compilers rebuilds everything.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

find_program(CLANG clang)
if(NOT CLANG)
	skip_test("clang not found in PATH")
endif()

use_fixture(c_basic)

set(ENV{CC} "clang")
idl("${PROJECT_DIR}" ARGS build)
file(READ "${PROJECT_DIR}/build/debug/obj/src/main.c.cmd" cmd)
string(REGEX MATCH "^[^\n]*" compiler "${cmd}")
if(NOT compiler STREQUAL "clang")
	message(FATAL_ERROR "Expected clang, got ${compiler}")
endif()
run_program("${PROJECT_DIR}/build/debug/c_basic${EXE}")
expect_contains("${PROGRAM_OUTPUT}" "sum=5")

# Without CC: back to the default (gcc, if present) and rebuild.
set(ENV{CC} "")
find_program(GCC gcc)
if(GCC)
	idl("${PROJECT_DIR}" ARGS build)
	expect_contains("${IDL_OUTPUT}" "Compiling src/main.c")
	file(READ "${PROJECT_DIR}/build/debug/obj/src/main.c.cmd" cmd)
	expect_contains("${cmd}" "gcc")
endif()

# Missing compiler: clear error.
set(ENV{CC} "idl-missing-compiler")
file(REMOVE_RECURSE "${PROJECT_DIR}/build")
idl("${PROJECT_DIR}" EXPECT 1 ARGS build)
expect_contains("${IDL_OUTPUT}" "idl-missing-compiler")
