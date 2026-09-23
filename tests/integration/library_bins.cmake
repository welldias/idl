# Project without main: static and shared libraries + executables from src/bin/.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

use_fixture(lib_bins)
set(out "${PROJECT_DIR}/build/debug")
set(shared_name "${SHARED_PREFIX}lib_bins${SHARED_EXT}")

idl("${PROJECT_DIR}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Archiving build/debug/liblib_bins.a")
expect_contains("${IDL_OUTPUT}" "Linking build/debug/${shared_name}")
expect_contains("${IDL_OUTPUT}" "Linking build/debug/calc_cli")
expect_contains("${IDL_OUTPUT}" "Linking build/debug/exit_with_3")
expect_exists("${out}/liblib_bins.a")
expect_exists("${out}/${shared_name}")

# The library sources are compiled as position independent code; the executables are not.
if(NOT CMAKE_HOST_WIN32)
	file(READ "${out}/obj/src/calc.c.cmd" lib_cmd)
	file(READ "${out}/obj/src/bin/calc_cli.c.cmd" bin_cmd)
	expect_contains("${lib_cmd}" "-fPIC")
	expect_not_contains("${bin_cmd}" "-fPIC")
endif()

# On Linux: a real shared object, with soname and the exported functions,
# that an external program can link against and run.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
	find_program(READELF readelf)
	if(READELF)
		execute_process(COMMAND "${READELF}" -h -d "${out}/${shared_name}" OUTPUT_VARIABLE elf)
		expect_contains("${elf}" "DYN (Shared object file)")
		expect_contains("${elf}" "Library soname: [${shared_name}]")
	endif()

	find_program(NM nm)
	if(NM)
		execute_process(COMMAND "${NM}" -D --defined-only "${out}/${shared_name}" OUTPUT_VARIABLE symbols)
		expect_contains("${symbols}" "calc_add")
		expect_contains("${symbols}" "calc_name")
	endif()

	find_program(C_COMPILER NAMES gcc clang cc)
	if(C_COMPILER)
		set(consumer "${WORK_DIR}/consumer")
		file(WRITE "${consumer}.c" "#include <stdio.h>\n#include <calc/calc.h>\nint main(void) { printf(\"%s %d\\n\", calc_name(), calc_add(40, 2)); return 0; }\n")
		execute_process(
			COMMAND "${C_COMPILER}" "${consumer}.c" "-I${PROJECT_DIR}/include" "-L${out}" -llib_bins "-Wl,-rpath,${out}" -o "${consumer}"
			RESULT_VARIABLE result
			OUTPUT_VARIABLE output
			ERROR_VARIABLE output
		)
		if(NOT result EQUAL 0)
			message(FATAL_ERROR "Could not link against ${shared_name}:\n${output}")
		endif()
		run_program("${consumer}")
		expect_contains("${PROGRAM_OUTPUT}" "calc 42")
	endif()
endif()

run_program("${out}/calc_cli${EXE}" ARGS 1 2 3)
expect_contains("${PROGRAM_OUTPUT}" "calc total=6")

# Choosing a target in the convention: an executable of src/bin/ gets the library's objects, not its archives.
idl("${PROJECT_DIR}" ARGS clean)
idl("${PROJECT_DIR}" ARGS build calc_cli)
expect_contains("${IDL_OUTPUT}" "Compiling src/calc.c")
expect_contains("${IDL_OUTPUT}" "Linking build/debug/calc_cli")
expect_not_contains("${IDL_OUTPUT}" "exit_with_3")
expect_not_contains("${IDL_OUTPUT}" "Archiving")
idl("${PROJECT_DIR}" ARGS build lib_bins)
expect_contains("${IDL_OUTPUT}" "Archiving build/debug/liblib_bins.a")
expect_contains("${IDL_OUTPUT}" "Linking build/debug/${shared_name}")
idl("${PROJECT_DIR}" ARGS build)

# With several executables, run needs the name of one.
idl("${PROJECT_DIR}" EXPECT 1 ARGS run)
expect_contains("${IDL_OUTPUT}" "choose one: idl run <name>")

idl("${PROJECT_DIR}" ARGS run calc_cli -- 10 20)
expect_contains("${IDL_OUTPUT}" "calc total=30")

# idl run forwards the program's exit code.
idl("${PROJECT_DIR}" EXPECT 3 ARGS run exit_with_3)
expect_contains("${IDL_OUTPUT}" "exiting with 3")

idl("${PROJECT_DIR}" EXPECT 1 ARGS run does_not_exist)
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
expect_contains("${IDL_OUTPUT}" "Linking build/debug/${shared_name}")
if(AR_TOOL)
	execute_process(COMMAND "${AR_TOOL}" t "${out}/liblib_bins.a" OUTPUT_VARIABLE members)
	expect_not_contains("${members}" "name.c.o")
	expect_contains("${members}" "name2.c.o")
endif()

# With a single executable, run does not need its name.
file(REMOVE "${PROJECT_DIR}/src/bin/exit_with_3.c")
idl("${PROJECT_DIR}" ARGS run -- 5)
expect_contains("${IDL_OUTPUT}" "calc2 total=5")

# A C++ library: the shared library is linked with the C++ compiler.
find_program(CXX_COMPILER NAMES g++ clang++)
if(CXX_COMPILER)
	set(cxx_dir "${WORK_DIR}/cxx_lib")
	file(WRITE "${cxx_dir}/src/greet.cpp" "#include <string>\nextern \"C\" int greet_len(void) { return (int)std::string(\"hello\").size(); }\n")
	idl("${cxx_dir}" ARGS build)
	expect_exists("${cxx_dir}/build/debug/libcxx_lib.a")
	expect_exists("${cxx_dir}/build/debug/${SHARED_PREFIX}cxx_lib${SHARED_EXT}")
	file(READ "${cxx_dir}/build/debug/obj/.link/shared-cxx_lib.cmd" link_cmd)
	string(REGEX MATCH "^[^\n]*" linker "${link_cmd}")
	expect_contains("${linker}" "++")
endif()
