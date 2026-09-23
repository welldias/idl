cmake_minimum_required(VERSION 4.0)
project(idl)

set(CMAKE_C_STANDARD 23)
set(BUILD_SHARED_LIBS OFF)
set(BUILD_TESTING OFF)

file(GLOB_RECURSE SOURCES src/*.c)

if(WIN32)
	set(EXCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/core/platform_unix.c")
else()
	set(EXCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/core/platform_win.c")
endif()

list(REMOVE_ITEM SOURCES ${EXCLUDE_FILES})

include(FetchContent)

# Faz option() das dependências respeitar variáveis já definidas aqui (ex.: BUILD_TESTING).
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# libuv: I/O assíncrono e utilitários de plataforma, baixada durante a configuração do build
FetchContent_Declare(
	libuv
	URL https://github.com/libuv/libuv/archive/refs/tags/v1.52.1.tar.gz
	URL_HASH SHA256=478baf2599bfbc882c355288c9cb6f92e0e7dda435fa04031fa5b607cf3f414c
	DOWNLOAD_EXTRACT_TIMESTAMP TRUE
	EXCLUDE_FROM_ALL
)
set(LIBUV_BUILD_SHARED OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(libuv)

# libyaml: leitura e escrita de YAML, baixada durante a configuração do build
FetchContent_Declare(
	libyaml
	URL https://github.com/yaml/libyaml/archive/refs/tags/0.2.5.tar.gz
	URL_HASH SHA256=fa240dbf262be053f3898006d502d514936c818e422afdcf33921c63bed9bf2e
	DOWNLOAD_EXTRACT_TIMESTAMP TRUE
	EXCLUDE_FROM_ALL
)
# O CMakeLists.txt da libyaml exige CMake 3.0, versão que o CMake 4 não aceita mais.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
FetchContent_MakeAvailable(libyaml)
unset(CMAKE_POLICY_VERSION_MINIMUM)

# Todo o código fica na biblioteca idl_core, que também é usada pelos testes.
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c")
add_library(idl_core STATIC ${SOURCES})
target_include_directories(idl_core PUBLIC src)
target_link_libraries(idl_core PUBLIC uv_a yaml)
target_compile_definitions(idl_core PUBLIC DEBUG=0)

add_executable(idl src/main.c)
target_link_libraries(idl idl_core)

option(IDL_BUILD_TESTS "Compila os testes do idl (ctest)" ON)
if(IDL_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()