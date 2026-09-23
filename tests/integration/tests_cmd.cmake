# idl test: compila e executa cada arquivo de tests/.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

use_fixture(with_tests)

idl("${PROJECT_DIR}" EXPECT 1 ARGS test)
expect_contains("${IDL_OUTPUT}" "Compilando tests/test_soma.c")
expect_contains("${IDL_OUTPUT}" "Teste test_soma: ok")
expect_contains("${IDL_OUTPUT}" "Teste test_quebrado: FALHOU")
expect_contains("${IDL_OUTPUT}" "1 passaram, 1 falharam")
expect_exists("${PROJECT_DIR}/build/debug/tests/test_soma${EXE}")

# Os testes linkam com os fontes do projeto, mas não com o main.
file(READ "${PROJECT_DIR}/build/debug/obj/.link/test-test_soma.cmd" link_cmd)
expect_contains("${link_cmd}" "soma.c.o")
expect_not_contains("${link_cmd}" "main.c.o")

file(REMOVE "${PROJECT_DIR}/tests/test_quebrado.c")
idl("${PROJECT_DIR}" ARGS test)
expect_contains("${IDL_OUTPUT}" "1 passaram, 0 falharam")

# idl build não compila os testes.
file(REMOVE_RECURSE "${PROJECT_DIR}/build")
idl("${PROJECT_DIR}" ARGS build)
expect_not_contains("${IDL_OUTPUT}" "tests/")

# Sem tests/: nada a testar, e não é erro.
file(REMOVE_RECURSE "${PROJECT_DIR}/tests")
idl("${PROJECT_DIR}" ARGS test)
expect_contains("${IDL_OUTPUT}" "Nenhum teste encontrado")
