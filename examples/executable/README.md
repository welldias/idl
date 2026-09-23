# Example: executable

A small `wc`-like program that counts lines, words and characters. It is built by idl
with no configuration file: the directory layout says everything.

```
executable/
  src/
    main.c               has main(): the project is an executable
    text/counter.c       other sources in src/ (any depth) are compiled and linked in
    text/counter.h
    os/os_name.h
    os/os_name_unix.c    built only on Unix-like systems (Linux, macOS)
    os/os_name_win.c     built only on Windows
  tests/
    test_counter.c       each file is a test program (exit code 0 = passed)
```

- `src/main.c` makes the project an executable, named after the directory:
  `build/debug/executable`.
- Every other source in `src/` is compiled and linked into it. `src/` is on the include
  path, so headers are included relative to it (`#include "text/counter.h"`).
- A file name ending in `_win`, `_linux`, `_macos` or `_unix` is only built on that
  system, so each platform gets its own `os_name()` without any `#ifdef`.
- Each file in `tests/` is linked with the sources of `src/` (except `main.c`) and run by
  `idl test`.

## Usage

```sh
idl build                              # build/debug/executable
idl run -- README.md                   # builds and runs with arguments
echo "one two" | idl run               # reads stdin when no file is given
idl run -- --version                   # shows the system name
idl test                               # builds and runs tests/test_counter.c
idl build --release                    # build/release/executable (-O2 -DNDEBUG)
```
