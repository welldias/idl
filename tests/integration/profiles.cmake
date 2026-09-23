# Perfis debug e release em diretórios separados.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

use_fixture(c_basic)

idl("${PROJECT_DIR}" ARGS build --release)
expect_contains("${IDL_OUTPUT}" "Construindo c_basic (release)")
run_program("${PROJECT_DIR}/build/release/c_basic${EXE}")
expect_contains("${PROGRAM_OUTPUT}" "perfil=release")
expect_not_exists("${PROJECT_DIR}/build/debug")

file(READ "${PROJECT_DIR}/build/release/obj/src/main.c.cmd" flags)
expect_contains("${flags}" "-O2")
expect_contains("${flags}" "-DNDEBUG")
expect_not_contains("${flags}" "-g\n")

idl("${PROJECT_DIR}" ARGS build)
run_program("${PROJECT_DIR}/build/debug/c_basic${EXE}")
expect_contains("${PROGRAM_OUTPUT}" "perfil=debug")

file(READ "${PROJECT_DIR}/build/debug/obj/src/main.c.cmd" flags)
expect_contains("${flags}" "-g\n")
expect_contains("${flags}" "-O0")

# Cada perfil tem seu próprio estado incremental.
idl("${PROJECT_DIR}" ARGS build --release)
expect_contains("${IDL_OUTPUT}" "Nada a fazer")

idl("${PROJECT_DIR}" ARGS run --release)
expect_contains("${IDL_OUTPUT}" "perfil=release")
