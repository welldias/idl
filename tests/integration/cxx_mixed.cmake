# C++ project with a C part and public headers in include/.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

find_program(CXX_COMPILER NAMES g++ clang++)
if(NOT CXX_COMPILER)
	skip_test("no C++ compiler in PATH")
endif()

use_fixture(cxx_mixed)

idl("${PROJECT_DIR}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Compiling src/main.cpp")
expect_contains("${IDL_OUTPUT}" "Compiling src/c_part.c")

run_program("${PROJECT_DIR}/build/debug/cxx_mixed${EXE}")
expect_contains("${PROGRAM_OUTPUT}" "cxx+mix double=42")

# The .cpp is compiled with the C++ compiler and the .c with the C compiler.
file(READ "${PROJECT_DIR}/build/debug/obj/src/main.cpp.cmd" cpp_cmd)
file(READ "${PROJECT_DIR}/build/debug/obj/src/c_part.c.cmd" c_cmd)
expect_contains("${cpp_cmd}" "++")
expect_not_contains("${c_cmd}" "++")
expect_contains("${cpp_cmd}" "-Iinclude")

# Linking uses the C++ compiler.
file(READ "${PROJECT_DIR}/build/debug/obj/.link/exe-cxx_mixed.cmd" link_cmd)
string(REGEX MATCH "^[^\n]*" linker "${link_cmd}")
expect_contains("${linker}" "++")

# requires-cpp becomes -std.
file(WRITE "${PROJECT_DIR}/project.yml" "project:\n  name: cxx_mixed\n  requires-c: C11\n  requires-cpp: C++17\n")
idl("${PROJECT_DIR}" ARGS build)
file(READ "${PROJECT_DIR}/build/debug/obj/src/main.cpp.cmd" cpp_cmd)
file(READ "${PROJECT_DIR}/build/debug/obj/src/c_part.c.cmd" c_cmd)
expect_contains("${cpp_cmd}" "-std=c++17")
expect_contains("${c_cmd}" "-std=c11")
