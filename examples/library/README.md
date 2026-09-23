# Example: library

A small geometry library with two command-line programs that use it. It is built by idl
with no configuration file: the directory layout says everything.

```
library/
  include/
    shapes/shapes.h      public headers (include/ is on the include path)
  src/
    circle.c             no main.* in src/: the project is a library
    rectangle.c
    bin/
      area.c             each file in src/bin/ becomes an executable
      table.cpp          C and C++ can be mixed
  tests/
    test_shapes.c        each file is a test program (exit code 0 = passed)
```

- Without `src/main.*`, the sources of `src/` (except `src/bin/`) form a library named
  after the directory. idl builds both the static and the shared version:
  `build/debug/liblibrary.a` and `build/debug/liblibrary.so` (`.dylib` on macOS,
  `library.dll` on Windows).
- Each file in `src/bin/` becomes an executable with the file's name, linked with the
  library sources: `build/debug/area` and `build/debug/table`.
- `include/` holds the public API, so both the library and its users write
  `#include <shapes/shapes.h>`. The header has `extern "C"` so the C++ program can use it.
- Each file in `tests/` is linked with the library sources and run by `idl test`.

## Usage

```sh
idl build                          # library + the two executables
./build/debug/area circle 2        # area=12.57 perimeter=12.57
./build/debug/area rectangle 3 4   # area=12.00 perimeter=14.00
./build/debug/table                # table of circle areas
idl test                           # builds and runs tests/test_shapes.c
```

Another project can use the built library:

```sh
cc app.c -Iinclude -Lbuild/debug -llibrary -o app
```
