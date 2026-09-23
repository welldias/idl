# CC/CXX escolhem o compilador; trocar de compilador recompila tudo.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

find_program(CLANG clang)
if(NOT CLANG)
	skip_test("clang não encontrado no PATH")
endif()

use_fixture(c_basic)

set(ENV{CC} "clang")
idl("${PROJECT_DIR}" ARGS build)
file(READ "${PROJECT_DIR}/build/debug/obj/src/main.c.cmd" cmd)
string(REGEX MATCH "^[^\n]*" compiler "${cmd}")
if(NOT compiler STREQUAL "clang")
	message(FATAL_ERROR "Esperado clang, obtido ${compiler}")
endif()
run_program("${PROJECT_DIR}/build/debug/c_basic${EXE}")
expect_contains("${PROGRAM_OUTPUT}" "soma=5")

# Sem CC: volta ao padrão (gcc, se houver) e recompila.
set(ENV{CC} "")
find_program(GCC gcc)
if(GCC)
	idl("${PROJECT_DIR}" ARGS build)
	expect_contains("${IDL_OUTPUT}" "Compilando src/main.c")
	file(READ "${PROJECT_DIR}/build/debug/obj/src/main.c.cmd" cmd)
	expect_contains("${cmd}" "gcc")
endif()

# Compilador inexistente: erro claro.
set(ENV{CC} "compilador-que-nao-existe-idl")
file(REMOVE_RECURSE "${PROJECT_DIR}/build")
idl("${PROJECT_DIR}" EXPECT 1 ARGS build)
expect_contains("${IDL_OUTPUT}" "compilador-que-nao-existe-idl")
