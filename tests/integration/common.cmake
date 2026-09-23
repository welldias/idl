# Common functions for the integration tests.
# Each script receives: IDL (binary), FIXTURES_DIR (sample projects) and WORK_DIR (scratch area).

foreach(var IDL FIXTURES_DIR WORK_DIR)
	if(NOT DEFINED ${var})
		message(FATAL_ERROR "Variable ${var} is not defined (run through ctest).")
	endif()
endforeach()

# Make sure CC/CXX/AR from the environment do not affect the tests.
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

# Skips the test (ctest treats exit code 77 as "skipped").
function(skip_test reason)
	message(STATUS "SKIP: ${reason}")
	cmake_language(EXIT 77)
endfunction()

# Copies tests/fixtures/<name> to WORK_DIR/<name> and sets PROJECT_DIR.
function(use_fixture name)
	file(COPY "${FIXTURES_DIR}/${name}" DESTINATION "${WORK_DIR}")
	set(PROJECT_DIR "${WORK_DIR}/${name}" PARENT_SCOPE)
endfunction()

# idl(<dir> EXPECT <code> ARGS <args...>): runs idl and stores its output (stdout+stderr) in IDL_OUTPUT.
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
	message(STATUS "$ idl ${ARG_ARGS}  (in ${dir}) -> ${result}\n${output}")

	if(NOT "${result}" STREQUAL "${ARG_EXPECT}")
		message(FATAL_ERROR "idl ${ARG_ARGS}: expected exit code ${ARG_EXPECT}, got ${result}")
	endif()
	set(IDL_OUTPUT "${output}" PARENT_SCOPE)
endfunction()

# run_program(<program> EXPECT <code> ARGS <args...>): runs a built binary; output in PROGRAM_OUTPUT.
function(run_program program)
	cmake_parse_arguments(ARG "" "EXPECT" "ARGS" ${ARGN})
	if(NOT DEFINED ARG_EXPECT)
		set(ARG_EXPECT 0)
	endif()

	if(NOT EXISTS "${program}")
		message(FATAL_ERROR "Program not found: ${program}")
	endif()

	execute_process(
		COMMAND "${program}" ${ARG_ARGS}
		OUTPUT_VARIABLE output
		ERROR_VARIABLE output
		RESULT_VARIABLE result
	)
	if(NOT "${result}" STREQUAL "${ARG_EXPECT}")
		message(FATAL_ERROR "${program}: expected exit code ${ARG_EXPECT}, got ${result}\n${output}")
	endif()
	set(PROGRAM_OUTPUT "${output}" PARENT_SCOPE)
endfunction()

function(expect_contains text needle)
	string(FIND "${text}" "${needle}" pos)
	if(pos EQUAL -1)
		message(FATAL_ERROR "Expected to find \"${needle}\" in:\n${text}")
	endif()
endfunction()

function(expect_not_contains text needle)
	string(FIND "${text}" "${needle}" pos)
	if(NOT pos EQUAL -1)
		message(FATAL_ERROR "Did not expect to find \"${needle}\" in:\n${text}")
	endif()
endfunction()

function(expect_exists path)
	if(NOT EXISTS "${path}")
		message(FATAL_ERROR "Expected to exist: ${path}")
	endif()
endfunction()

function(expect_not_exists path)
	if(EXISTS "${path}")
		message(FATAL_ERROR "Did not expect to exist: ${path}")
	endif()
endfunction()

# Counts how many times <needle> appears in <text>.
function(count_occurrences text needle out_var)
	string(REPLACE "${needle}" "\n" replaced "${text}")
	string(LENGTH "${text}" len_before)
	string(LENGTH "${replaced}" len_after)
	string(LENGTH "${needle}" len_needle)
	math(EXPR count "(${len_before} - ${len_after}) / (${len_needle} - 1)")
	set(${out_var} ${count} PARENT_SCOPE)
endfunction()

# Bumps the file mtime so the incremental build sees the change.
function(touch_later path)
	file(TOUCH "${path}")
endfunction()
