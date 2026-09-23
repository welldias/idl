# Projeto sem main: biblioteca estática + executáveis de src/bin/.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

use_fixture(lib_bins)
set(out "${PROJECT_DIR}/build/debug")

idl("${PROJECT_DIR}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Arquivando build/debug/liblib_bins.a")
expect_contains("${IDL_OUTPUT}" "Linkando build/debug/calc_cli")
expect_contains("${IDL_OUTPUT}" "Linkando build/debug/sai_com_3")
expect_exists("${out}/liblib_bins.a")

run_program("${out}/calc_cli${EXE}" ARGS 1 2 3)
expect_contains("${PROGRAM_OUTPUT}" "calc total=6")

# Com vários executáveis, o run exige --bin.
idl("${PROJECT_DIR}" EXPECT 1 ARGS run)
expect_contains("${IDL_OUTPUT}" "--bin")

idl("${PROJECT_DIR}" ARGS run --bin calc_cli -- 10 20)
expect_contains("${IDL_OUTPUT}" "calc total=30")

# O código de saída do programa é repassado pelo idl run.
idl("${PROJECT_DIR}" EXPECT 3 ARGS run --bin sai_com_3)
expect_contains("${IDL_OUTPUT}" "saindo com 3")

idl("${PROJECT_DIR}" EXPECT 1 ARGS run --bin nao_existe)
expect_contains("${IDL_OUTPUT}" "'nao_existe' não encontrado")

# A biblioteca contém só os fontes de src/ (sem os de src/bin/).
find_program(AR_TOOL ar)
if(AR_TOOL)
	execute_process(COMMAND "${AR_TOOL}" t "${out}/liblib_bins.a" OUTPUT_VARIABLE members)
	expect_contains("${members}" "calc.c.o")
	expect_contains("${members}" "nome.c.o")
	expect_not_contains("${members}" "calc_cli")
endif()

# Fonte removido sai da biblioteca.
file(REMOVE "${PROJECT_DIR}/src/core/nome.c")
file(WRITE "${PROJECT_DIR}/src/core/nome2.c" "const char *calc_nome(void) { return \"calc2\"; }\n")
idl("${PROJECT_DIR}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Arquivando")
if(AR_TOOL)
	execute_process(COMMAND "${AR_TOOL}" t "${out}/liblib_bins.a" OUTPUT_VARIABLE members)
	expect_not_contains("${members}" "nome.c.o")
	expect_contains("${members}" "nome2.c.o")
endif()

# Com um único executável, o run não precisa de --bin.
file(REMOVE "${PROJECT_DIR}/src/bin/sai_com_3.c")
idl("${PROJECT_DIR}" ARGS run -- 5)
expect_contains("${IDL_OUTPUT}" "calc2 total=5")
