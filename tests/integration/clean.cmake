# idl clean: removes build/ only inside a project.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

use_fixture(c_basic)

idl("${PROJECT_DIR}" ARGS build)
expect_exists("${PROJECT_DIR}/build")

idl("${PROJECT_DIR}" ARGS clean)
expect_contains("${IDL_OUTPUT}" "Removed build/")
expect_not_exists("${PROJECT_DIR}/build")
expect_exists("${PROJECT_DIR}/src/main.c")

idl("${PROJECT_DIR}" ARGS clean)
expect_contains("${IDL_OUTPUT}" "Nothing to clean")

# Outside a project, it does not delete some random build/.
file(WRITE "${WORK_DIR}/other/build/important.txt" "do not delete\n")
idl("${WORK_DIR}/other" EXPECT 1 ARGS clean)
expect_exists("${WORK_DIR}/other/build/important.txt")

# --project
idl("${PROJECT_DIR}" ARGS build)
idl("${WORK_DIR}" ARGS clean --project c_basic)
expect_not_exists("${PROJECT_DIR}/build")
