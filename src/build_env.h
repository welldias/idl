#ifndef IDL_BUILD_ENV_H
#define IDL_BUILD_ENV_H

#include "idl.h"

/* Environment variable names accepted in "envs:": [A-Z_][A-Z0-9_]*. */
bool build_env_valid_name(const char *name);

/* Replaces ${NAME} with the current value of the variable (empty if it is not set) and
   $$ with $. Returns NULL when a ${ is not closed. Release with free. */
char *build_env_expand(const char *value);

/* Sets the variables of the "envs:" section (project_env_t items) in idl's own
   environment, in order, so every process idl starts from now on inherits them.
   <applied> receives "NAME=value\n" for each variable set, with the value as written in
   project.yml (not expanded, so a different shell PATH does not look like a change), or "".
   Release with free. */
bool build_env_apply(list_t *envs, char **applied);

#endif // IDL_BUILD_ENV_H
