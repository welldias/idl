# Targets declared in project.yml: sources spread in several directories, libraries and executables.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

use_fixture(targets)
set(out "${PROJECT_DIR}/build/debug")
set(obj "${out}/obj")
set(plugin_name "${SHARED_PREFIX}plugin${SHARED_EXT}")

idl("${PROJECT_DIR}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Compiling modules/base/base.c (base)")
expect_contains("${IDL_OUTPUT}" "Compiling modules/core/util/extra.c (core)") # "**" goes into subdirectories
expect_contains("${IDL_OUTPUT}" "Compiling apps/main/args.c (targets)")
expect_not_contains("${IDL_OUTPUT}" "legacy")                                 # exclude
expect_contains("${IDL_OUTPUT}" "Archiving build/debug/libbase.a")
expect_contains("${IDL_OUTPUT}" "Archiving build/debug/libcore.a")
expect_contains("${IDL_OUTPUT}" "Linking build/debug/${plugin_name}")
expect_contains("${IDL_OUTPUT}" "Linking build/debug/targets")
expect_contains("${IDL_OUTPUT}" "Linking build/debug/host")

# Only the source of the current system is compiled.
if(CMAKE_HOST_WIN32)
	expect_contains("${IDL_OUTPUT}" "modules/core/os_win.c")
	expect_not_contains("${IDL_OUTPUT}" "modules/core/os_unix.c")
else()
	expect_contains("${IDL_OUTPUT}" "modules/core/os_unix.c")
	expect_not_contains("${IDL_OUTPUT}" "modules/core/os_win.c")
endif()

run_program("${out}/targets${EXE}" ARGS a b)
expect_contains("${PROGRAM_OUTPUT}" "core=19 extra=5")
expect_contains("${PROGRAM_OUTPUT}" "args=2")
run_program("${out}/host${EXE}")
expect_contains("${PROGRAM_OUTPUT}" "plugin=38 version=3")

# Each target is compiled with the global settings, its own and the public include dirs of what it links.
file(READ "${obj}/lib/core/modules/core/core.c.cmd" core_cmd)
expect_contains("${core_cmd}" "-std=c17")
expect_contains("${core_cmd}" "-DGLOBAL=1")
expect_contains("${core_cmd}" "-DCORE_LEVEL=2")
expect_contains("${core_cmd}" "-Imodules/core/private")
expect_contains("${core_cmd}" "-Imodules/core/include")
expect_contains("${core_cmd}" "-Imodules/base/include")

file(READ "${obj}/exe/targets/apps/main/main.c.cmd" main_cmd)
expect_contains("${main_cmd}" "-Imodules/core/include")
expect_contains("${main_cmd}" "-Imodules/base/include")   # transitive
expect_not_contains("${main_cmd}" "modules/core/private") # private to core
expect_not_contains("${main_cmd}" "CORE_LEVEL")
expect_not_contains("${main_cmd}" "-fPIC")

file(READ "${obj}/lib/base/modules/base/base.c.cmd" base_cmd)
expect_not_contains("${base_cmd}" "modules/core")

# The static libraries that go into the shared library are position independent code.
if(NOT CMAKE_HOST_WIN32)
	expect_contains("${core_cmd}" "-fPIC")
	expect_contains("${base_cmd}" "-fPIC")
endif()

# Link order: dependents before their dependencies; the shared library is not linked again statically.
file(READ "${obj}/.link/exe-targets.cmd" link_cmd)
string(FIND "${link_cmd}" "libcore.a" core_pos)
string(FIND "${link_cmd}" "libbase.a" base_pos)
if(core_pos EQUAL -1 OR base_pos EQUAL -1 OR core_pos GREATER base_pos)
	message(FATAL_ERROR "Expected libcore.a before libbase.a in:\n${link_cmd}")
endif()
# The system libraries of a static library go to whoever links it.
expect_contains("${link_cmd}" "-lm")

file(READ "${obj}/.link/exe-host.cmd" host_cmd)
expect_contains("${host_cmd}" "${plugin_name}")
expect_not_contains("${host_cmd}" "libcore.a")
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
	expect_contains("${host_cmd}" "-Wl,-rpath,$ORIGIN")
endif()

# idl build does not build the tests.
expect_not_contains("${IDL_OUTPUT}" "checks/")
expect_not_exists("${out}/tests")

# idl test: one executable per source of "checks", one with every source of "suite".
idl("${PROJECT_DIR}" ARGS test)
expect_contains("${IDL_OUTPUT}" "Compiling checks/test_core.c (test_core)")
expect_contains("${IDL_OUTPUT}" "Linking build/debug/tests/test_core")
expect_contains("${IDL_OUTPUT}" "Linking build/debug/tests/test_plugin")
expect_contains("${IDL_OUTPUT}" "Linking build/debug/tests/suite")
expect_contains("${IDL_OUTPUT}" "core: ok")
expect_contains("${IDL_OUTPUT}" "plugin: ok")
expect_contains("${IDL_OUTPUT}" "suite: 2 of 2 cases passed")
expect_contains("${IDL_OUTPUT}" "Result: 3 passed, 0 failed.")
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
	file(READ "${obj}/.link/test-test_plugin.cmd" test_link)
	expect_contains("${test_link}" "-Wl,-rpath,$ORIGIN/..")
endif()

# Tests are not executables for idl run.
idl("${PROJECT_DIR}" EXPECT 1 ARGS run suite)
expect_contains("${IDL_OUTPUT}" "'suite' not found")

# Nothing changed: nothing is rebuilt.
idl("${PROJECT_DIR}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Nothing to do")

# A change in base: only base.c is compiled; what uses libbase.a is linked again.
touch_later("${PROJECT_DIR}/modules/base/base.c")
idl("${PROJECT_DIR}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Compiling modules/base/base.c (base)")
expect_not_contains("${IDL_OUTPUT}" "Compiling modules/core")
expect_contains("${IDL_OUTPUT}" "Archiving build/debug/libbase.a")
expect_not_contains("${IDL_OUTPUT}" "Archiving build/debug/libcore.a")
expect_contains("${IDL_OUTPUT}" "Linking build/debug/targets")
expect_contains("${IDL_OUTPUT}" "Linking build/debug/${plugin_name}")
expect_contains("${IDL_OUTPUT}" "Linking build/debug/host")

# run: without a name, the executable named after the project.
idl("${PROJECT_DIR}" ARGS run -- x)
expect_contains("${IDL_OUTPUT}" "core=19 extra=5")
idl("${PROJECT_DIR}" ARGS run host)
expect_contains("${IDL_OUTPUT}" "plugin=38")

# build <target>...: only the targets asked for and what they link.
idl("${PROJECT_DIR}" ARGS clean)
idl("${PROJECT_DIR}" ARGS build)
file(READ "${PROJECT_DIR}/build/compile_commands.json" all_commands)
idl("${PROJECT_DIR}" ARGS clean)
file(MAKE_DIRECTORY "${PROJECT_DIR}/build")
file(WRITE "${PROJECT_DIR}/build/compile_commands.json" "${all_commands}")

idl("${PROJECT_DIR}" ARGS build targets)
expect_contains("${IDL_OUTPUT}" "Compiling modules/base/base.c (base)")
expect_contains("${IDL_OUTPUT}" "Compiling apps/main/main.c (targets)")
expect_not_contains("${IDL_OUTPUT}" "plugin")
expect_not_contains("${IDL_OUTPUT}" "host")
expect_contains("${IDL_OUTPUT}" "Archiving build/debug/libcore.a")
expect_contains("${IDL_OUTPUT}" "Linking build/debug/targets")
expect_not_exists("${out}/host${EXE}")
run_program("${out}/targets${EXE}")
expect_contains("${PROGRAM_OUTPUT}" "core=19")

# compile_commands.json keeps describing the whole project.
file(READ "${PROJECT_DIR}/build/compile_commands.json" commands)
expect_contains("${commands}" "apps/host.c")

# Several targets; one already built is not rebuilt.
idl("${PROJECT_DIR}" ARGS build targets host)
expect_contains("${IDL_OUTPUT}" "Compiling apps/host.c (host)")
expect_contains("${IDL_OUTPUT}" "Linking build/debug/${plugin_name}")
expect_not_contains("${IDL_OUTPUT}" "Compiling apps/main")

# A library alone: its static archive, not the executables.
idl("${PROJECT_DIR}" ARGS release core)
expect_contains("${IDL_OUTPUT}" "Building targets (release)")
expect_contains("${IDL_OUTPUT}" "Archiving build/release/libbase.a")
expect_contains("${IDL_OUTPUT}" "Archiving build/release/libcore.a")
expect_not_contains("${IDL_OUTPUT}" "Linking")

idl("${PROJECT_DIR}" EXPECT 1 ARGS build nothing)
expect_contains("${IDL_OUTPUT}" "Target 'nothing' not found. Targets of the project: base, core, plugin, targets, host")
idl("${PROJECT_DIR}" EXPECT 1 ARGS run targets host)
expect_contains("${IDL_OUTPUT}" "Unexpected argument 'host'")

# run <name> builds only that executable.
idl("${PROJECT_DIR}" ARGS clean)
idl("${PROJECT_DIR}" ARGS run host)
expect_contains("${IDL_OUTPUT}" "plugin=38")
expect_not_contains("${IDL_OUTPUT}" "apps/main")

# test <name> runs only the tests asked for.
idl("${PROJECT_DIR}" ARGS test test_plugin)
expect_contains("${IDL_OUTPUT}" "plugin: ok")
expect_not_contains("${IDL_OUTPUT}" "core: ok")
expect_contains("${IDL_OUTPUT}" "Result: 1 passed, 0 failed.")

# idl add rewrites project.yml and keeps the targets.
idl("${PROJECT_DIR}" ARGS add pthread)
file(READ "${PROJECT_DIR}/project.yml" yml)
expect_contains("${yml}" "targets:")
expect_contains("${yml}" "public-include-dirs:")
idl("${PROJECT_DIR}" ARGS build)
file(READ "${obj}/.link/exe-targets.cmd" link_cmd)
expect_contains("${link_cmd}" "-pthread")

# An executable and a library may share a name; the executable links the library.
set(same "${WORK_DIR}/same_name")
file(WRITE "${same}/src/lapi.c" "int lua_answer(void) { return 42; }\n")
file(WRITE "${same}/src/lua.c" "#include <stdio.h>\nint lua_answer(void);\nint main(void) { printf(\"lua %d\\n\", lua_answer()); return 0; }\n")
file(WRITE "${same}/src/luac.c" "#include <stdio.h>\nint lua_answer(void);\nint main(void) { printf(\"luac %d\\n\", lua_answer()); return 0; }\n")
file(WRITE "${same}/project.yml" "project:\n  name: same_name\ntargets:\n  lua:\n    type: static-library\n    sources: [src/*.c]\n    exclude: [src/lua.c, src/luac.c]\n  luac:\n    type: executable\n    sources: [src/luac.c]\n    link: [lua]\n  lua:\n    type: executable\n    sources: [src/lua.c]\n    link: [lua]\n")
idl("${same}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Archiving build/debug/liblua.a")
expect_contains("${IDL_OUTPUT}" "Linking build/debug/lua")
expect_exists("${same}/build/debug/obj/lib/lua/src/lapi.c.o")
expect_exists("${same}/build/debug/obj/exe/lua/src/lua.c.o")
run_program("${same}/build/debug/lua${EXE}")
expect_contains("${PROGRAM_OUTPUT}" "lua 42")
idl("${same}" ARGS run lua)
expect_contains("${IDL_OUTPUT}" "lua 42")
idl("${same}" ARGS run luac)
expect_contains("${IDL_OUTPUT}" "luac 42")

# A name shared by an executable and a library builds both.
idl("${same}" ARGS clean)
idl("${same}" ARGS build lua)
expect_contains("${IDL_OUTPUT}" "Archiving build/debug/liblua.a")
expect_contains("${IDL_OUTPUT}" "Linking build/debug/lua")
expect_not_contains("${IDL_OUTPUT}" "luac")

# A project with targets but no test targets.
idl("${same}" ARGS test)
expect_contains("${IDL_OUTPUT}" "No tests declared")

# Errors in the targets.
set(bad "${WORK_DIR}/bad")
file(WRITE "${bad}/a.c" "int a(void) { return 0; }\n")
file(WRITE "${bad}/project.yml" "project:\n  name: bad\ntargets:\n  a:\n    type: static-library\n    sources: [a.c]\n    link: [b]\n  b:\n    type: static-library\n    sources: [a.c]\n    link: [a]\n")
idl("${bad}" EXPECT 1 ARGS build)
expect_contains("${IDL_OUTPUT}" "link cycle")

file(WRITE "${bad}/project.yml" "project:\n  name: bad\ntargets:\n  a:\n    type: executable\n    sources: [missing.c]\n")
idl("${bad}" EXPECT 1 ARGS build)
expect_contains("${IDL_OUTPUT}" "source 'missing.c' not found")

file(WRITE "${bad}/project.yml" "project:\n  name: bad\ntargets:\n  a:\n    type: static-library\n    sources: [a.c]\n")
idl("${bad}" EXPECT 1 ARGS run)
expect_contains("${IDL_OUTPUT}" "type: executable")
