# idl clean: remove build/ somente dentro de um projeto.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

use_fixture(c_basic)

idl("${PROJECT_DIR}" ARGS build)
expect_exists("${PROJECT_DIR}/build")

idl("${PROJECT_DIR}" ARGS clean)
expect_contains("${IDL_OUTPUT}" "Removido build/")
expect_not_exists("${PROJECT_DIR}/build")
expect_exists("${PROJECT_DIR}/src/main.c")

idl("${PROJECT_DIR}" ARGS clean)
expect_contains("${IDL_OUTPUT}" "Nada a limpar")

# Fora de um projeto, não apaga um build/ qualquer.
file(WRITE "${WORK_DIR}/outro/build/importante.txt" "não apagar\n")
idl("${WORK_DIR}/outro" EXPECT 1 ARGS clean)
expect_exists("${WORK_DIR}/outro/build/importante.txt")

# --project
idl("${PROJECT_DIR}" ARGS build)
idl("${WORK_DIR}" ARGS clean --project c_basic)
expect_not_exists("${PROJECT_DIR}/build")
