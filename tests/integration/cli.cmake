# Linha de comando: ajuda, comando desconhecido e códigos de saída.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

idl("${WORK_DIR}")
expect_contains("${IDL_OUTPUT}" "Usage:")

idl("${WORK_DIR}" ARGS help)
expect_contains("${IDL_OUTPUT}" "build    Build the project")
expect_contains("${IDL_OUTPUT}" "test     Build and run the project's tests")
expect_contains("${IDL_OUTPUT}" "clean    Remove the build directory")

idl("${WORK_DIR}" EXPECT 1 ARGS comando-inexistente)
expect_contains("${IDL_OUTPUT}" "Unknown command: comando-inexistente")

# Comandos de projeto fora de um projeto falham.
idl("${WORK_DIR}" EXPECT 1 ARGS add zlib)
expect_contains("${IDL_OUTPUT}" "Project file not found")
idl("${WORK_DIR}" EXPECT 1 ARGS build)
expect_contains("${IDL_OUTPUT}" "src/")
idl("${WORK_DIR}" EXPECT 1 ARGS run)
idl("${WORK_DIR}" EXPECT 1 ARGS test)
