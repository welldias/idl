#ifndef IDL_BUILD_GLOB_H
#define IDL_BUILD_GLOB_H

#include "idl.h"
#include "build_source.h"

// Matches a relative path ('/' separators) against a pattern:
// '*' any characters except '/', '?' one character except '/',
// '**' any characters including '/' ("a/**/b.c" also matches "a/b.c").
bool build_glob_match(const char *pattern, const char *path);

/* Adds to <out> the C/C++ sources matched by <patterns> (files or globs), minus those
   matched by <excludes>, skipping sources of other systems (see build_source_for_os).
   <owner> names who asked for the sources in the messages. A missing file or a path
   outside the project is an error; a glob that matches nothing is a warning. */
bool build_glob_expand(list_t *patterns, list_t *excludes, const char *owner, build_sources_t *out);

#endif // IDL_BUILD_GLOB_H
