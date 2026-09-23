# Project without main: static library + executables from src/bin/.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

use_fixture(lib_bins)
set(out "${PROJECT_DIR}/build/debug")

idl("${PROJECT_DIR}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Archiving build/debug/liblib_bins.a")
expect_contains("${IDL_OUTPUT}" "Linking build/debug/calc_cli")
expect_contains("${IDL_OUTPUT}" "Linking build/debug/exit_with_3")
expect_exists("${out}/liblib_bins.a")

run_program("${out}/calc_cli${EXE}" ARGS 1 2 3)
expect_contains("${PROGRAM_OUTPUT}" "calc total=6")

# With several executables, run requires --bin.
idl("${PROJECT_DIR}" EXPECT 1 ARGS run)
expect_contains("${IDL_OUTPUT}" "--bin")

idl("${PROJECT_DIR}" ARGS run --bin calc_cli -- 10 20)
expect_contains("${IDL_OUTPUT}" "calc total=30")

# idl run forwards the program's exit code.
idl("${PROJECT_DIR}" EXPECT 3 ARGS run --bin exit_with_3)
expect_contains("${IDL_OUTPUT}" "exiting with 3")

idl("${PROJECT_DIR}" EXPECT 1 ARGS run --bin does_not_exist)
expect_contains("${IDL_OUTPUT}" "'does_not_exist' not found")

# The library holds only the sources of src/ (not those of src/bin/).
find_program(AR_TOOL ar)
if(AR_TOOL)
	execute_process(COMMAND "${AR_TOOL}" t "${out}/liblib_bins.a" OUTPUT_VARIABLE members)
	expect_contains("${members}" "calc.c.o")
	expect_contains("${members}" "name.c.o")
	expect_not_contains("${members}" "calc_cli")
endif()

# A removed source leaves the library.
file(REMOVE "${PROJECT_DIR}/src/core/name.c")
file(WRITE "${PROJECT_DIR}/src/core/name2.c" "const char *calc_name(void) { return \"calc2\"; }\n")
idl("${PROJECT_DIR}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Archiving")
if(AR_TOOL)
	execute_process(COMMAND "${AR_TOOL}" t "${out}/liblib_bins.a" OUTPUT_VARIABLE members)
	expect_not_contains("${members}" "name.c.o")
	expect_contains("${members}" "name2.c.o")
endif()

# With a single executable, run does not need --bin.
file(REMOVE "${PROJECT_DIR}/src/bin/exit_with_3.c")
idl("${PROJECT_DIR}" ARGS run -- 5)
expect_contains("${IDL_OUTPUT}" "calc2 total=5")
