# System dependencies and the build: section of project.yml.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

use_fixture(deps)

idl("${PROJECT_DIR}" ARGS run)
expect_contains("${IDL_OUTPUT}" "sqrt=4.0 version=7 extra=extra")

file(READ "${PROJECT_DIR}/build/debug/obj/src/main.c.cmd" compile_cmd)
expect_contains("${compile_cmd}" "-std=c17")
expect_contains("${compile_cmd}" "-DVERSION=7")
expect_contains("${compile_cmd}" "-Ithird_party")
expect_contains("${compile_cmd}" "-Wshadow")

file(READ "${PROJECT_DIR}/build/debug/obj/.link/exe-deps.cmd" link_cmd)
expect_contains("${link_cmd}" "-lm")

# idl add looks for the library: one that is not on the machine is refused and nothing is saved.
file(READ "${PROJECT_DIR}/project.yml" yml_before)
idl("${PROJECT_DIR}" EXPECT 1 ARGS add idl-missing-lib)
expect_contains("${IDL_OUTPUT}" "Library 'idl-missing-lib' not found")
idl("${PROJECT_DIR}" EXPECT 1 ARGS add pthread idl-missing-lib)
expect_contains("${IDL_OUTPUT}" "Nothing was saved")
file(READ "${PROJECT_DIR}/project.yml" yml_after)
if(NOT yml_before STREQUAL yml_after)
	message(FATAL_ERROR "project.yml changed after a failed idl add:\n${yml_after}")
endif()

idl("${PROJECT_DIR}" ARGS add m)
expect_contains("${IDL_OUTPUT}" "'m' is already a dependency")
idl("${PROJECT_DIR}" ARGS add pthread)
expect_contains("${IDL_OUTPUT}" "Added pthread (system library)")
idl("${PROJECT_DIR}" EXPECT 1 ARGS add)
expect_contains("${IDL_OUTPUT}" "Usage: idl add")

# A dependency written by hand that is not on the machine stops the build before compiling.
file(READ "${PROJECT_DIR}/project.yml" yml)
string(REPLACE "  - m\n" "  - m\n  - idl-missing-lib\n" yml "${yml}")
file(WRITE "${PROJECT_DIR}/project.yml" "${yml}")
idl("${PROJECT_DIR}" EXPECT 1 ARGS build)
expect_contains("${IDL_OUTPUT}" "Dependency 'idl-missing-lib' not found")
expect_not_contains("${IDL_OUTPUT}" "Compiling")

# A library in a directory of LD_LIBRARY_PATH: built by idl itself, then used by another project.
if(NOT CMAKE_HOST_WIN32)
	if(CMAKE_HOST_APPLE)
		set(lib_path_var DYLD_LIBRARY_PATH)
	else()
		set(lib_path_var LD_LIBRARY_PATH)
	endif()

	set(lib_dir "${WORK_DIR}/mylib")
	file(WRITE "${lib_dir}/src/mylib.c" "int mylib_answer(void) { return 42; }\n")
	idl("${lib_dir}" ARGS build)
	expect_exists("${lib_dir}/build/debug/libmylib.a")

	set(app_dir "${WORK_DIR}/uses_mylib")
	file(WRITE "${app_dir}/src/main.c" "#include <stdio.h>\nint mylib_answer(void);\nint main(void) { printf(\"answer=%d\\n\", mylib_answer()); return 0; }\n")
	file(WRITE "${app_dir}/project.yml" "project:\n  name: uses_mylib\n")

	idl("${app_dir}" EXPECT 1 ARGS add mylib)
	expect_contains("${IDL_OUTPUT}" "Library 'mylib' not found")

	set(ENV{${lib_path_var}} "${WORK_DIR}/nowhere:${lib_dir}/build/debug")
	idl("${app_dir}" ARGS add libmylib)
	expect_contains("${IDL_OUTPUT}" "Added mylib (${lib_dir}/build/debug/libmylib")
	file(READ "${app_dir}/project.yml" yml)
	expect_contains("${yml}" "- mylib")
	idl("${app_dir}" ARGS run)
	expect_contains("${IDL_OUTPUT}" "answer=42")
	file(READ "${app_dir}/build/debug/obj/.link/exe-uses_mylib.cmd" link_cmd)
	expect_contains("${link_cmd}" "-L${lib_dir}/build/debug")
	expect_contains("${link_cmd}" "-lmylib")

	# Without the variable, the build does not find it...
	unset(ENV{${lib_path_var}})
	idl("${app_dir}" EXPECT 1 ARGS build)
	expect_contains("${IDL_OUTPUT}" "Dependency 'mylib' not found")

	# ...unless the project declares it in envs:, which also counts for idl add.
	file(WRITE "${app_dir}/project.yml" "project:\n  name: uses_mylib\nenvs:\n  ${lib_path_var}: ${lib_dir}/build/debug\n")
	idl("${app_dir}" ARGS add mylib)
	expect_contains("${IDL_OUTPUT}" "Added mylib")
	idl("${app_dir}" ARGS run)
	expect_contains("${IDL_OUTPUT}" "answer=42")
endif()

# zlib through pkg-config, when installed.
find_program(PKG_CONFIG pkg-config)
if(PKG_CONFIG)
	execute_process(COMMAND "${PKG_CONFIG}" --exists zlib RESULT_VARIABLE has_zlib)
endif()
if(PKG_CONFIG AND has_zlib EQUAL 0)
	set(zdir "${WORK_DIR}/zlib_app")
	file(WRITE "${zdir}/src/main.c" "#include <stdio.h>\n#include <zlib.h>\nint main(void) { printf(\"zlib %s\\n\", zlibVersion()); return 0; }\n")
	file(WRITE "${zdir}/project.yml" "project:\n  name: zlib_app\n  dependencies: [zlib]\n")
	idl("${zdir}" ARGS run)
	expect_contains("${IDL_OUTPUT}" "zlib 1.")
	expect_not_contains("${IDL_OUTPUT}" "not found")

	# idl add shows where the library came from.
	file(WRITE "${zdir}/project.yml" "project:\n  name: zlib_app\n")
	idl("${zdir}" ARGS add zlib)
	expect_contains("${IDL_OUTPUT}" "Added zlib (pkg-config, version 1.")
else()
	message(STATUS "zlib/pkg-config missing: part of the test skipped")
endif()
