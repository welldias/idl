# idl test: builds and runs each file in tests/.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

use_fixture(with_tests)

idl("${PROJECT_DIR}" EXPECT 1 ARGS test)
expect_contains("${IDL_OUTPUT}" "Compiling tests/test_sum.c")
expect_contains("${IDL_OUTPUT}" "Test test_sum: ok")
expect_contains("${IDL_OUTPUT}" "Test test_broken: FAILED")
expect_contains("${IDL_OUTPUT}" "1 passed, 1 failed")
expect_exists("${PROJECT_DIR}/build/debug/tests/test_sum${EXE}")

# Tests link with the project sources, but not with main.
file(READ "${PROJECT_DIR}/build/debug/obj/.link/test-test_sum.cmd" link_cmd)
expect_contains("${link_cmd}" "sum.c.o")
expect_not_contains("${link_cmd}" "main.c.o")

file(REMOVE "${PROJECT_DIR}/tests/test_broken.c")
idl("${PROJECT_DIR}" ARGS test)
expect_contains("${IDL_OUTPUT}" "1 passed, 0 failed")

# idl build does not compile the tests.
file(REMOVE_RECURSE "${PROJECT_DIR}/build")
idl("${PROJECT_DIR}" ARGS build)
expect_not_contains("${IDL_OUTPUT}" "tests/")

# No tests/: nothing to test, and that is not an error.
file(REMOVE_RECURSE "${PROJECT_DIR}/tests")
idl("${PROJECT_DIR}" ARGS test)
expect_contains("${IDL_OUTPUT}" "No tests found")
