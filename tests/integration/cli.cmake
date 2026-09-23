# Command line: help, unknown command and exit codes.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

idl("${WORK_DIR}")
expect_contains("${IDL_OUTPUT}" "Usage:")

idl("${WORK_DIR}" ARGS help)
expect_contains("${IDL_OUTPUT}" "build    Build the project")
expect_contains("${IDL_OUTPUT}" "test     Build and run the project's tests")
expect_contains("${IDL_OUTPUT}" "clean    Remove the build directory")
expect_not_contains("${IDL_OUTPUT}" "pip")
expect_contains("${IDL_OUTPUT}" "Usage: idl <COMMAND> [ARGS]")
expect_contains("${IDL_OUTPUT}" "cache    Manage idl's cache")
expect_contains("${IDL_OUTPUT}" "self     Manage the idl executable")
expect_contains("${IDL_OUTPUT}" "venv     Create a virtual environment")
# Nothing copied from uv is left in the help.
expect_not_contains("${IDL_OUTPUT}" "uv")
expect_not_contains("${IDL_OUTPUT}" "UV_")
expect_not_contains("${IDL_OUTPUT}" "box.exe")

idl("${WORK_DIR}" ARGS --help)
expect_contains("${IDL_OUTPUT}" "Usage: idl")
idl("${WORK_DIR}" ARGS -h)
expect_contains("${IDL_OUTPUT}" "Usage: idl")

idl("${WORK_DIR}" ARGS help)
expect_contains("${IDL_OUTPUT}" "release  Build the project optimized")
expect_contains("${IDL_OUTPUT}" "idl help <COMMAND>")

# The help of a command comes before it: idl help <command>.
idl("${WORK_DIR}" ARGS help build)
expect_contains("${IDL_OUTPUT}" "Usage: idl build [TARGET]...")
expect_contains("${IDL_OUTPUT}" "--project <DIR>")
idl("${WORK_DIR}" ARGS help run)
expect_contains("${IDL_OUTPUT}" "-- ARGS")
idl("${WORK_DIR}" ARGS help sync)
expect_contains("${IDL_OUTPUT}" "Not implemented yet.")
idl("${WORK_DIR}" EXPECT 1 ARGS help nope)
expect_contains("${IDL_OUTPUT}" "Unknown command 'nope'")
idl("${WORK_DIR}" EXPECT 1 ARGS help build run)
expect_contains("${IDL_OUTPUT}" "Unexpected argument 'run'")

# After a command, --help is an option like any other: unknown.
idl("${WORK_DIR}" EXPECT 1 ARGS build --help)
expect_contains("${IDL_OUTPUT}" "Unknown option '--help'")
idl("${WORK_DIR}" EXPECT 1 ARGS run -h)
expect_contains("${IDL_OUTPUT}" "Unknown option '-h'")
idl("${WORK_DIR}" EXPECT 1 ARGS add --help)
expect_contains("${IDL_OUTPUT}" "Unknown option '--help'")
idl("${WORK_DIR}" EXPECT 1 ARGS init --help)
expect_contains("${IDL_OUTPUT}" "Unknown option '--help'")

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
