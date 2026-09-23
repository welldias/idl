# Testes do idl. Rode com: cmake --build build && ctest --test-dir build --output-on-failure
#
#   unit.*         testes em C de cada módulo (tests/unit/test_<módulo>.c)
#   integration.*  scripts CMake que executam o binário idl sobre projetos de exemplo (tests/fixtures)

# Programa auxiliar usado no lugar de comandos do shell (portável entre Unix e Windows).
add_executable(fake_tool helpers/fake_tool.c)

set(IDL_UNIT_TESTS
	list
	strutils
	stream
	cmd_args
	compiler_command
	platform
	process_runner
	project_file
	build_layout
	build_toolchain
)

foreach(name ${IDL_UNIT_TESTS})
	add_executable(test_${name} unit/test_${name}.c)
	target_link_libraries(test_${name} idl_core)
	target_compile_definitions(test_${name} PRIVATE
		IDL_TEST_WORK_DIR="${CMAKE_CURRENT_BINARY_DIR}/work/unit_${name}"
		IDL_TEST_FAKE_TOOL="$<TARGET_FILE:fake_tool>"
	)
	add_dependencies(test_${name} fake_tool)
	add_test(NAME unit.${name} COMMAND test_${name})
	set_tests_properties(unit.${name} PROPERTIES LABELS unit)
endforeach()

set(IDL_INTEGRATION_TESTS
	cli
	init
	build_c
	build_errors
	profiles
	cxx_mixed
	library_bins
	tests_cmd
	deps
	clean
	toolchain_env
)

foreach(name ${IDL_INTEGRATION_TESTS})
	add_test(NAME integration.${name}
		COMMAND ${CMAKE_COMMAND}
			-DIDL=$<TARGET_FILE:idl>
			-DFIXTURES_DIR=${CMAKE_CURRENT_SOURCE_DIR}/fixtures
			-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/work/integration_${name}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/integration/${name}.cmake
	)
	# Código 77: teste pulado (ex.: não há compilador C++ ou clang na máquina).
	set_tests_properties(integration.${name} PROPERTIES LABELS integration SKIP_RETURN_CODE 77)
endforeach()
