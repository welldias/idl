# Debug and release profiles in separate directories.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

use_fixture(c_basic)

idl("${PROJECT_DIR}" ARGS build --release)
expect_contains("${IDL_OUTPUT}" "Building c_basic (release)")
run_program("${PROJECT_DIR}/build/release/c_basic${EXE}")
expect_contains("${PROGRAM_OUTPUT}" "profile=release")
expect_not_exists("${PROJECT_DIR}/build/debug")

file(READ "${PROJECT_DIR}/build/release/obj/src/main.c.cmd" flags)
expect_contains("${flags}" "-O2")
expect_contains("${flags}" "-DNDEBUG")
expect_not_contains("${flags}" "-g\n")

idl("${PROJECT_DIR}" ARGS build)
run_program("${PROJECT_DIR}/build/debug/c_basic${EXE}")
expect_contains("${PROGRAM_OUTPUT}" "profile=debug")

file(READ "${PROJECT_DIR}/build/debug/obj/src/main.c.cmd" flags)
expect_contains("${flags}" "-g\n")
expect_contains("${flags}" "-O0")

# Each profile has its own incremental state.
idl("${PROJECT_DIR}" ARGS build --release)
expect_contains("${IDL_OUTPUT}" "Nothing to do")

idl("${PROJECT_DIR}" ARGS run --release)
expect_contains("${IDL_OUTPUT}" "profile=release")
