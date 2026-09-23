#ifndef IDL_PLATFORM_H
#define IDL_PLATFORM_H

#include "commons.h"
#include "list.h"
#include <uv.h>

typedef void (*platform_scan_directory_cb)(const char*, void* arg);

bool platform_file_exists(const char* full_path);
bool platform_file_is_binary(const char* full_path);
bool platform_make_dirs(const char* path);
bool platform_dir_exists(const char* path);
/* Retorna o instante de modificação em nanossegundos, ou -1 se o arquivo não existir. */
int64 platform_file_mtime(const char* path);
/* Remove o arquivo ou o diretório com todo o seu conteúdo (como rm -rf). */
bool platform_remove_tree(const char* path);
/* Executa um programa com stdin/stdout/stderr herdados e retorna o código de saída (-1 se não iniciar). */
int platform_exec(char* const* argv);
/* extension == NULL chama add_file_cb para todos os arquivos. */
bool platform_scan_directory(const char *dir_path, const char *extension, platform_scan_directory_cb add_file_cb, void* arg);
bool platform_processes_wait_async(void * proc, int ms);
uint32 platform_num_cores(void);

#endif //IDL_PLATFORM_H