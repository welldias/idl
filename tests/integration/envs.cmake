# The envs: section of project.yml: variables for the build and for the programs idl runs.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

set(dir "${WORK_DIR}/envs_app")
file(WRITE "${dir}/hidden/secret.h" "#define SECRET 42\n")
file(WRITE "${dir}/src/main.c" [=[
#include <stdio.h>
#include <stdlib.h>
#include "secret.h" /* only found through CPATH: hidden/ is not in any -I */

int main(void) {
    const char *greeting = getenv("APP_GREETING");
    printf("secret=%d greeting=%s\n", SECRET, greeting ? greeting : "(none)");
    return 0;
}
]=])
file(WRITE "${dir}/tests/test_env.c" [=[
#include <stdlib.h>
#include <string.h>

int main(void) {
    const char *greeting = getenv("APP_GREETING");
    return greeting && strcmp(greeting, "hello hidden") == 0 ? 0 : 1;
}
]=])
file(WRITE "${dir}/project.yml" [=[
project:
  name: envs_app
envs:
  CPATH: hidden
  APP_GREETING: hello ${CPATH}
  APP_PRICE: $$5
  lower_case: ignored
]=])

# The compiler receives CPATH; the program receives the variables too, with ${...} expanded.
idl("${dir}" ARGS run)
expect_contains("${IDL_OUTPUT}" "secret=42 greeting=hello hidden")
expect_contains("${IDL_OUTPUT}" "envs: 'lower_case' ignored")

idl("${dir}" ARGS test)
expect_contains("${IDL_OUTPUT}" "Test test_env: ok")

# The variables are part of what the build depends on: nothing changed, nothing is rebuilt...
idl("${dir}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Nothing to do")

# ...but a changed variable rebuilds. Here, without CPATH, the header is not found.
file(WRITE "${dir}/project.yml" "project:\n  name: envs_app\n")
idl("${dir}" EXPECT 1 ARGS build)
expect_contains("${IDL_OUTPUT}" "secret.h")

# idl add rewrites project.yml and keeps the section.
file(WRITE "${dir}/project.yml" "project:\n  name: envs_app\nenvs:\n  CPATH: hidden\n  APP_GREETING: hello \${CPATH}\n")
idl("${dir}" ARGS add m)
file(READ "${dir}/project.yml" yml)
expect_contains("${yml}" "envs:")
expect_contains("${yml}" "APP_GREETING: hello \${CPATH}")
idl("${dir}" ARGS run)
expect_contains("${IDL_OUTPUT}" "greeting=hello hidden")

# An unclosed ${ is an error.
file(WRITE "${dir}/project.yml" "project:\n  name: envs_app\nenvs:\n  CPATH: \${OPEN\n")
idl("${dir}" EXPECT 1 ARGS build)
expect_contains("${IDL_OUTPUT}" "without the closing")
