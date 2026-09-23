# Build errors return a non-zero exit code and clear messages.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

# Compilation error.
use_fixture(c_basic)
file(APPEND "${PROJECT_DIR}/src/util.c" "int broken( {\n")
idl("${PROJECT_DIR}" EXPECT 1 ARGS build)
expect_contains("${IDL_OUTPUT}" "Compiling src/util.c: failed")
expect_contains("${IDL_OUTPUT}" "Compilation failed")
expect_not_contains("${IDL_OUTPUT}" "Linking")

# Link error (function declared but never defined).
set(link_dir "${WORK_DIR}/link_error")
file(WRITE "${link_dir}/src/main.c" "int does_not_exist(void);\nint main(void) { return does_not_exist(); }\n")
idl("${link_dir}" EXPECT 1 ARGS build)
expect_contains("${IDL_OUTPUT}" "Linking failed")

# No src/.
file(MAKE_DIRECTORY "${WORK_DIR}/no_src")
idl("${WORK_DIR}/no_src" EXPECT 1 ARGS build)
expect_contains("${IDL_OUTPUT}" "src/ not found")

# src/ without sources.
file(WRITE "${WORK_DIR}/empty/src/readme.txt" "nothing here\n")
idl("${WORK_DIR}/empty" EXPECT 1 ARGS build)
expect_contains("${IDL_OUTPUT}" "No C/C++ sources")

# Two mains.
file(WRITE "${WORK_DIR}/two_mains/src/main.c" "int main(void) { return 0; }\n")
file(WRITE "${WORK_DIR}/two_mains/src/main.cpp" "int main() { return 0; }\n")
idl("${WORK_DIR}/two_mains" EXPECT 1 ARGS build)
expect_contains("${IDL_OUTPUT}" "More than one main")

# Invalid project.yml.
file(WRITE "${WORK_DIR}/bad_yml/src/main.c" "int main(void) { return 0; }\n")
file(WRITE "${WORK_DIR}/bad_yml/project.yml" "project: [\n")
idl("${WORK_DIR}/bad_yml" EXPECT 1 ARGS build)

# Project directory does not exist.
idl("${WORK_DIR}" EXPECT 1 ARGS build --project does-not-exist)
expect_contains("${IDL_OUTPUT}" "does-not-exist")

# Sources outside the convention produce a warning but do not stop the build.
file(WRITE "${WORK_DIR}/warnings/src/main.c" "int main(void) { return 0; }\n")
file(WRITE "${WORK_DIR}/warnings/src/bin/sub/x.c" "int main(void) { return 0; }\n")
idl("${WORK_DIR}/warnings" ARGS build)
expect_contains("${IDL_OUTPUT}" "src/bin/sub/x.c ignored")

# Unknown requires-c: warning and build without -std.
file(WRITE "${WORK_DIR}/warnings/project.yml" "project:\n  name: warnings\n  requires-c: K&R\n")
idl("${WORK_DIR}/warnings" ARGS build)
expect_contains("${IDL_OUTPUT}" "requires-c 'K&R' not recognized")
