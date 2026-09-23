# idl init: creates project.yml, README.md and src/main.c without overwriting anything; idl add.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

set(dir "${WORK_DIR}/my_app")
file(MAKE_DIRECTORY "${dir}")
file(WRITE "${dir}/README.md" "# already here\n")

idl("${dir}" ARGS init)
expect_contains("${IDL_OUTPUT}" "Project 'my_app' initialized successfully")
expect_exists("${dir}/project.yml")
expect_exists("${dir}/src/main.c")

file(READ "${dir}/README.md" readme)
expect_contains("${readme}" "# already here")

file(READ "${dir}/project.yml" yml)
expect_contains("${yml}" "name: my_app")
expect_contains("${yml}" "requires-c: C23")

file(READ "${dir}/src/main.c" main_c)
expect_contains("${main_c}" "Hello from my_app!")

# The freshly created project already builds and runs.
idl("${dir}" ARGS run)
expect_contains("${IDL_OUTPUT}" "Hello from my_app!")

# Does not re-initialize an existing project.
idl("${dir}" EXPECT 1 ARGS init)
expect_contains("${IDL_OUTPUT}" "already initialized")

# idl add
idl("${dir}" ARGS add m)
idl("${dir}" ARGS add m)
file(READ "${dir}/project.yml" yml)
count_occurrences("${yml}" "- m" deps)
if(NOT deps EQUAL 1)
	message(FATAL_ERROR "Duplicated dependency:\n${yml}")
endif()
idl("${dir}" EXPECT 1 ARGS add)

# An existing main.cpp does not get a main.c next to it.
set(cpp_dir "${WORK_DIR}/app_cpp")
file(WRITE "${cpp_dir}/src/main.cpp" "int main() { return 0; }\n")
idl("${cpp_dir}" ARGS init)
expect_not_exists("${cpp_dir}/src/main.c")

# A name with spaces and quotes becomes a valid C string.
set(odd_dir "${WORK_DIR}/my \"app\"")
file(MAKE_DIRECTORY "${odd_dir}")
idl("${odd_dir}" ARGS init)
idl("${odd_dir}" ARGS run)
expect_contains("${IDL_OUTPUT}" "Hello from my \"app\"!")
