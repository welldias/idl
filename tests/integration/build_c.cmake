# Build de um projeto C por convenção, build incremental e compile_commands.json.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

use_fixture(c_basic)
set(bin "${PROJECT_DIR}/build/debug/c_basic${EXE}")

idl("${PROJECT_DIR}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Construindo c_basic (debug)")
expect_contains("${IDL_OUTPUT}" "Compilando src/main.c")
expect_contains("${IDL_OUTPUT}" "Compilando src/util.c")
expect_contains("${IDL_OUTPUT}" "Linkando build/debug/c_basic")

run_program("${bin}" ARGS a b)
expect_contains("${PROGRAM_OUTPUT}" "soma=5 perfil=debug args=2")
expect_contains("${PROGRAM_OUTPUT}" "arg[2]=b")

# Os fontes ficam limpos: objetos só em build/.
file(GLOB_RECURSE stray "${PROJECT_DIR}/src/*.o")
if(stray)
	message(FATAL_ERROR "Objetos fora de build/: ${stray}")
endif()
expect_exists("${PROJECT_DIR}/build/debug/obj/src/main.c.o")

# Nada mudou: nada é recompilado.
idl("${PROJECT_DIR}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Nada a fazer")
expect_not_contains("${IDL_OUTPUT}" "Compilando")

# Só util.c mudou.
touch_later("${PROJECT_DIR}/src/util.c")
idl("${PROJECT_DIR}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Compilando src/util.c")
expect_not_contains("${IDL_OUTPUT}" "Compilando src/main.c")
expect_contains("${IDL_OUTPUT}" "Linkando")

# Header mudou: recompila quem o inclui (os dois).
touch_later("${PROJECT_DIR}/src/util.h")
idl("${PROJECT_DIR}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Compilando src/util.c")
expect_contains("${IDL_OUTPUT}" "Compilando src/main.c")

# Mudança de flags (project.yml) também recompila.
file(WRITE "${PROJECT_DIR}/project.yml" "project:\n  name: c_basic\nbuild:\n  defines: [EXTRA=1]\n")
idl("${PROJECT_DIR}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Compilando src/main.c")
expect_contains("${IDL_OUTPUT}" "Compilando src/util.c")

# Binário apagado: só linka de novo.
file(REMOVE "${bin}")
idl("${PROJECT_DIR}" ARGS build)
expect_not_contains("${IDL_OUTPUT}" "Compilando")
expect_contains("${IDL_OUTPUT}" "Linkando")
expect_exists("${bin}")

# Fonte novo entra no build.
file(WRITE "${PROJECT_DIR}/src/novo.c" "int novo(void) { return 1; }\n")
idl("${PROJECT_DIR}" ARGS build)
expect_contains("${IDL_OUTPUT}" "Compilando src/novo.c")

# compile_commands.json válido, com um item por fonte.
set(json_file "${PROJECT_DIR}/build/compile_commands.json")
expect_exists("${json_file}")
file(READ "${json_file}" json)
string(JSON entries LENGTH "${json}")
if(NOT entries EQUAL 3)
	message(FATAL_ERROR "compile_commands.json deveria ter 3 itens, tem ${entries}")
endif()
string(JSON first_file GET "${json}" 0 file)
string(JSON first_arg GET "${json}" 0 arguments 0)
string(JSON directory GET "${json}" 0 directory)
expect_contains("${first_file}" "src/")
expect_contains("${directory}" "c_basic")
if(first_arg STREQUAL "")
	message(FATAL_ERROR "compile_commands.json sem compilador")
endif()

# idl run repassa os argumentos depois de --.
idl("${PROJECT_DIR}" ARGS run -- x "com espaço")
expect_contains("${IDL_OUTPUT}" "args=2")
expect_contains("${IDL_OUTPUT}" "arg[2]=com espaço")

# --project (e o antigo -p) apontam para outro diretório.
idl("${WORK_DIR}" ARGS build --project c_basic)
expect_contains("${IDL_OUTPUT}" "Nada a fazer")
idl("${WORK_DIR}" ARGS build -p c_basic)
expect_contains("${IDL_OUTPUT}" "Nada a fazer")
