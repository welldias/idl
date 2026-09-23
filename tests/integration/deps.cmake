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

# Unknown dependency: warning and -l<name>, which fails at link time.
idl("${PROJECT_DIR}" ARGS add idl-missing-lib)
idl("${PROJECT_DIR}" EXPECT 1 ARGS build)
expect_contains("${IDL_OUTPUT}" "Dependency 'idl-missing-lib' not found by pkg-config")
expect_contains("${IDL_OUTPUT}" "Linking failed")

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
	expect_not_contains("${IDL_OUTPUT}" "not found by pkg-config")
else()
	message(STATUS "zlib/pkg-config missing: part of the test skipped")
endif()
