#ifndef IDL_HANDLERS_H
#define IDL_HANDLERS_H

#include "idl.h"

int handle_param_run(int argc, char *argv[]);
int handle_param_init(int argc, char *argv[]);
int handle_param_add(int argc, char *argv[]);
int handle_param_remove(int argc, char *argv[]);
int handle_param_version(int argc, char *argv[]);
int handle_param_sync(int argc, char *argv[]);
int handle_param_lock(int argc, char *argv[]);
int handle_param_export(int argc, char *argv[]);
int handle_param_tree(int argc, char *argv[]);
int handle_param_tool(int argc, char *argv[]);
int handle_param_c(int argc, char *argv[]);
int handle_param_pip(int argc, char *argv[]);
int handle_param_venv(int argc, char *argv[]);
int handle_param_build(int argc, char *argv[]);
int handle_param_test(int argc, char *argv[]);
int handle_param_clean(int argc, char *argv[]);
int handle_param_publish(int argc, char *argv[]);
int handle_param_cache(int argc, char *argv[]);
int handle_param_self(int argc, char *argv[]);
int handle_param_help(int argc, char *argv[]);

#endif
