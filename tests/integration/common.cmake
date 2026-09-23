# Funções comuns dos testes de integração.
# Cada script recebe: IDL (binário), FIXTURES_DIR (projetos de exemplo) e WORK_DIR (área de trabalho).

foreach(var IDL FIXTURES_DIR WORK_DIR)
	if(NOT DEFINED ${var})
		message(FATAL_ERROR "Variável ${var} não definida (rode pelo ctest).")
	endif()
endforeach()

# Garante que CC/CXX/AR do ambiente não interfiram nos testes.
set(ENV{CC} "")
set(ENV{CXX} "")
set(ENV{AR} "")

if(CMAKE_HOST_WIN32)
	set(EXE ".exe")
else()
	set(EXE "")
endif()

file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}")

# Pula o teste (o ctest trata o código 77 como "skipped").
function(skip_test reason)
	message(STATUS "SKIP: ${reason}")
	cmake_language(EXIT 77)
endfunction()

# Copia tests/fixtures/<name> para WORK_DIR/<name> e define PROJECT_DIR.
function(use_fixture name)
	file(COPY "${FIXTURES_DIR}/${name}" DESTINATION "${WORK_DIR}")
	set(PROJECT_DIR "${WORK_DIR}/${name}" PARENT_SCOPE)
endfunction()

# idl(<dir> EXPECT <código> ARGS <args...>): executa o idl e guarda a saída (stdout+stderr) em IDL_OUTPUT.
function(idl dir)
	cmake_parse_arguments(ARG "" "EXPECT" "ARGS" ${ARGN})
	if(NOT DEFINED ARG_EXPECT)
		set(ARG_EXPECT 0)
	endif()

	execute_process(
		COMMAND "${IDL}" ${ARG_ARGS}
		WORKING_DIRECTORY "${dir}"
		OUTPUT_VARIABLE output
		ERROR_VARIABLE output
		RESULT_VARIABLE result
	)
	message(STATUS "$ idl ${ARG_ARGS}  (em ${dir}) -> ${result}\n${output}")

	if(NOT "${result}" STREQUAL "${ARG_EXPECT}")
		message(FATAL_ERROR "idl ${ARG_ARGS}: esperado código ${ARG_EXPECT}, obtido ${result}")
	endif()
	set(IDL_OUTPUT "${output}" PARENT_SCOPE)
endfunction()

# run_program(<programa> EXPECT <código> ARGS <args...>): executa um binário gerado; saída em PROGRAM_OUTPUT.
function(run_program program)
	cmake_parse_arguments(ARG "" "EXPECT" "ARGS" ${ARGN})
	if(NOT DEFINED ARG_EXPECT)
		set(ARG_EXPECT 0)
	endif()

	if(NOT EXISTS "${program}")
		message(FATAL_ERROR "Programa não encontrado: ${program}")
	endif()

	execute_process(
		COMMAND "${program}" ${ARG_ARGS}
		OUTPUT_VARIABLE output
		ERROR_VARIABLE output
		RESULT_VARIABLE result
	)
	if(NOT "${result}" STREQUAL "${ARG_EXPECT}")
		message(FATAL_ERROR "${program}: esperado código ${ARG_EXPECT}, obtido ${result}\n${output}")
	endif()
	set(PROGRAM_OUTPUT "${output}" PARENT_SCOPE)
endfunction()

function(expect_contains text needle)
	string(FIND "${text}" "${needle}" pos)
	if(pos EQUAL -1)
		message(FATAL_ERROR "Esperado encontrar \"${needle}\" em:\n${text}")
	endif()
endfunction()

function(expect_not_contains text needle)
	string(FIND "${text}" "${needle}" pos)
	if(NOT pos EQUAL -1)
		message(FATAL_ERROR "Não esperado encontrar \"${needle}\" em:\n${text}")
	endif()
endfunction()

function(expect_exists path)
	if(NOT EXISTS "${path}")
		message(FATAL_ERROR "Esperado existir: ${path}")
	endif()
endfunction()

function(expect_not_exists path)
	if(EXISTS "${path}")
		message(FATAL_ERROR "Não esperado existir: ${path}")
	endif()
endfunction()

# Conta quantas vezes <needle> aparece em <text>.
function(count_occurrences text needle out_var)
	string(REPLACE "${needle}" "\n" replaced "${text}")
	string(LENGTH "${text}" len_before)
	string(LENGTH "${replaced}" len_after)
	string(LENGTH "${needle}" len_needle)
	math(EXPR count "(${len_before} - ${len_after}) / (${len_needle} - 1)")
	set(${out_var} ${count} PARENT_SCOPE)
endfunction()

# Avança o mtime do arquivo, para o build incremental enxergar a mudança.
function(touch_later path)
	file(TOUCH "${path}")
endfunction()
