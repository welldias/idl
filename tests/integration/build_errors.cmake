# Erros de build devolvem código diferente de 0 e mensagens claras.
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

# Erro de compilação.
use_fixture(c_basic)
file(APPEND "${PROJECT_DIR}/src/util.c" "int quebrado( {\n")
idl("${PROJECT_DIR}" EXPECT 1 ARGS build)
expect_contains("${IDL_OUTPUT}" "Compilando src/util.c: falhou")
expect_contains("${IDL_OUTPUT}" "Falha na compilação")
expect_not_contains("${IDL_OUTPUT}" "Linkando")

# Erro de linkagem (função declarada e nunca definida).
set(link_dir "${WORK_DIR}/link_error")
file(WRITE "${link_dir}/src/main.c" "int nao_existe(void);\nint main(void) { return nao_existe(); }\n")
idl("${link_dir}" EXPECT 1 ARGS build)
expect_contains("${IDL_OUTPUT}" "Falha na linkagem")

# Sem src/.
file(MAKE_DIRECTORY "${WORK_DIR}/sem_src")
idl("${WORK_DIR}/sem_src" EXPECT 1 ARGS build)
expect_contains("${IDL_OUTPUT}" "src/ não encontrado")

# src/ sem fontes.
file(WRITE "${WORK_DIR}/vazio/src/leia.txt" "nada aqui\n")
idl("${WORK_DIR}/vazio" EXPECT 1 ARGS build)
expect_contains("${IDL_OUTPUT}" "Nenhum fonte C/C++")

# Dois mains.
file(WRITE "${WORK_DIR}/dois_mains/src/main.c" "int main(void) { return 0; }\n")
file(WRITE "${WORK_DIR}/dois_mains/src/main.cpp" "int main() { return 0; }\n")
idl("${WORK_DIR}/dois_mains" EXPECT 1 ARGS build)
expect_contains("${IDL_OUTPUT}" "Mais de um main")

# project.yml inválido.
file(WRITE "${WORK_DIR}/yml_ruim/src/main.c" "int main(void) { return 0; }\n")
file(WRITE "${WORK_DIR}/yml_ruim/project.yml" "project: [\n")
idl("${WORK_DIR}/yml_ruim" EXPECT 1 ARGS build)

# Diretório de projeto inexistente.
idl("${WORK_DIR}" EXPECT 1 ARGS build --project nao-existe)
expect_contains("${IDL_OUTPUT}" "nao-existe")

# Fontes fora da convenção geram aviso, mas não impedem o build.
file(WRITE "${WORK_DIR}/avisos/src/main.c" "int main(void) { return 0; }\n")
file(WRITE "${WORK_DIR}/avisos/src/bin/sub/x.c" "int main(void) { return 0; }\n")
idl("${WORK_DIR}/avisos" ARGS build)
expect_contains("${IDL_OUTPUT}" "src/bin/sub/x.c ignorado")

# requires-c desconhecido: aviso e build sem -std.
file(WRITE "${WORK_DIR}/avisos/project.yml" "project:\n  name: avisos\n  requires-c: K&R\n")
idl("${WORK_DIR}/avisos" ARGS build)
expect_contains("${IDL_OUTPUT}" "requires-c 'K&R' não reconhecido")
