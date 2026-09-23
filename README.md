# idl

idl is a project manager for C and C++. It builds your project without a Makefile, CMake or any configuration file: just organize the code following a directory convention.

```
my_project/
  project.yml      optional
  include/         public headers (added to -I)
  src/
    main.c         becomes the executable build/debug/my_project
    *.c, *.cpp     other sources, compiled and linked together
    bin/<name>.c   each file becomes an extra executable
  tests/           each file becomes a test
```

Sources whose name ends in a platform suffix are only built on that platform: `*_win.c` on Windows, `*_linux.c` on Linux, `*_macos.c` on macOS and `*_unix.c` on any Unix-like system (Linux and macOS included). This applies to `src/`, `src/bin/` and `tests/`, in C and C++.

Without `src/main.*`, the project is a library: idl builds both the static `lib<name>.a` and the shared `lib<name>.so` (`.dylib` on macOS, `.dll` on Windows). C and C++ can be mixed. Sources are compiled in parallel, and only what changed is rebuilt (including when a header is modified).

See [examples/executable](examples/executable) and [examples/library](examples/library) for complete projects built without any configuration file.

## Usage

```sh
idl init              # creates project.yml, README.md and src/main.c
idl build             # builds into build/debug/ (--release: build/release/)
idl run -- args       # builds and runs
idl test              # builds and runs the tests in tests/
idl add zlib          # adds a system library (via pkg-config)
idl clean             # removes build/
```

`project.yml` is optional and adjusts what the convention doesn't cover:

```yaml
project:
  name: my_project
  requires-c: C23        # becomes -std=c23
  dependencies: [m, zlib]
build:
  defines: [FOO=1]
  cflags: [-Wshadow]
```

## Building idl

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

The dependencies (libuv and libyaml) are downloaded automatically by CMake.
