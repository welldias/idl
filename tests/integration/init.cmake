# idl init: cria project.yml, README.md e src/main.c sem sobrescrever nada; idl add.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

set(dir "${WORK_DIR}/meu_app")
file(MAKE_DIRECTORY "${dir}")
file(WRITE "${dir}/README.md" "# já existia\n")

idl("${dir}" ARGS init)
expect_contains("${IDL_OUTPUT}" "Project 'meu_app' initialized successfully")
expect_exists("${dir}/project.yml")
expect_exists("${dir}/src/main.c")

file(READ "${dir}/README.md" readme)
expect_contains("${readme}" "# já existia")

file(READ "${dir}/project.yml" yml)
expect_contains("${yml}" "name: meu_app")
expect_contains("${yml}" "requires-c: C23")

file(READ "${dir}/src/main.c" main_c)
expect_contains("${main_c}" "Hello from meu_app!")

# O projeto recém-criado já compila e roda.
idl("${dir}" ARGS run)
expect_contains("${IDL_OUTPUT}" "Hello from meu_app!")

# Não reinicializa um projeto existente.
idl("${dir}" EXPECT 1 ARGS init)
expect_contains("${IDL_OUTPUT}" "already initialized")

# idl add
idl("${dir}" ARGS add m)
idl("${dir}" ARGS add m)
file(READ "${dir}/project.yml" yml)
count_occurrences("${yml}" "- m" deps)
if(NOT deps EQUAL 1)
	message(FATAL_ERROR "Dependência duplicada:\n${yml}")
endif()
idl("${dir}" EXPECT 1 ARGS add)

# Um main.cpp existente não ganha um main.c ao lado.
set(cpp_dir "${WORK_DIR}/app_cpp")
file(WRITE "${cpp_dir}/src/main.cpp" "int main() { return 0; }\n")
idl("${cpp_dir}" ARGS init)
expect_not_exists("${cpp_dir}/src/main.c")

# Nome com espaço e aspas vira uma string C válida.
set(odd_dir "${WORK_DIR}/meu \"app\"")
file(MAKE_DIRECTORY "${odd_dir}")
idl("${odd_dir}" ARGS init)
idl("${odd_dir}" ARGS run)
expect_contains("${IDL_OUTPUT}" "Hello from meu \"app\"!")
