# Command line: help, unknown command and exit codes.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

idl("${WORK_DIR}")
expect_contains("${IDL_OUTPUT}" "Usage:")

idl("${WORK_DIR}" ARGS help)
expect_contains("${IDL_OUTPUT}" "build    Build the project")
expect_contains("${IDL_OUTPUT}" "test     Build and run the project's tests")
expect_contains("${IDL_OUTPUT}" "clean    Remove the build directory")
expect_not_contains("${IDL_OUTPUT}" "pip")
expect_contains("${IDL_OUTPUT}" "Usage: idl <COMMAND> [OPTIONS]")
expect_contains("${IDL_OUTPUT}" "cache    Manage idl's cache")
expect_contains("${IDL_OUTPUT}" "self     Manage the idl executable")
expect_contains("${IDL_OUTPUT}" "venv     Create a virtual environment")
expect_contains("${IDL_OUTPUT}" "--project <DIR>")
# Nothing copied from uv is left in the help.
expect_not_contains("${IDL_OUTPUT}" "uv")
expect_not_contains("${IDL_OUTPUT}" "UV_")
expect_not_contains("${IDL_OUTPUT}" "box.exe")

idl("${WORK_DIR}" ARGS --help)
expect_contains("${IDL_OUTPUT}" "Usage: idl")
idl("${WORK_DIR}" ARGS -h)
expect_contains("${IDL_OUTPUT}" "Usage: idl")

# pip is Python-only and does not exist in idl.
idl("${WORK_DIR}" EXPECT 1 ARGS pip)
expect_contains("${IDL_OUTPUT}" "Unknown command: pip")

idl("${WORK_DIR}" EXPECT 1 ARGS comando-inexistente)
expect_contains("${IDL_OUTPUT}" "Unknown command: comando-inexistente")

# Project commands fail outside a project.
idl("${WORK_DIR}" EXPECT 1 ARGS add zlib)
expect_contains("${IDL_OUTPUT}" "Project file not found")
idl("${WORK_DIR}" EXPECT 1 ARGS build)
expect_contains("${IDL_OUTPUT}" "src/")
idl("${WORK_DIR}" EXPECT 1 ARGS run)
idl("${WORK_DIR}" EXPECT 1 ARGS test)
