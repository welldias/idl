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

## Projects that don't follow the convention

When the sources are spread over other directories, or the project produces several
libraries and executables, declare them in the `targets:` section of `project.yml`. With
`targets:`, idl builds exactly what is declared and the directory convention is not used.

```yaml
project:
  name: app
targets:
  core:
    type: static-library       # executable | static-library | shared-library | library (static + shared) | test
    sources: [lib/core/**/*.c, lib/common.c]
    exclude: [lib/core/legacy/**]
    include-dirs: [lib/core/private]       # this target only
    public-include-dirs: [lib/core/include] # this target and whoever links it
    defines: [CORE=1]
    cflags: []
    cxxflags: []
    libs: [m]
  app:
    type: executable
    sources: [apps/*.c]
    link: [core]               # other targets of the project
    ldflags: []
```

- `sources` takes files and globs (`*`, `?`, `**`). `exclude` removes sources that match its globs.
- `link` handles the link order, `-fPIC` for code that goes into shared libraries, and the rpath for the project's own shared libraries.
- The `build:` section and `project.dependencies` apply to every target.
- An executable and a library may have the same name (`lua` and `liblua.a`); `link: [lua]` always refers to the library.
- Targets of `type: test` are built and run by `idl test` only: each source becomes a test program named after the file (like `tests/` in the convention), or, with `single: true`, all the sources make one program named after the target. A test passes when it exits with code 0.
- `idl run <target>` runs one of the executables.

See [examples/configured](examples/configured).

## Environment variables

The `envs:` section sets environment variables for every process idl starts in the project: the compiler, the linker, `pkg-config`, and the programs run by `idl run` and `idl test`.

```yaml
envs:
  PKG_CONFIG_PATH: /opt/openssl-3/lib/pkgconfig:${PKG_CONFIG_PATH}
  LD_LIBRARY_PATH: /opt/openssl-3/lib   # so idl run/test find the library at run time
  TZ: UTC
```

- Only uppercase names (`A-Z`, `0-9`, `_`) become variables; other keys are ignored with a warning.
- `${NAME}` is replaced by the current value of the variable, including one set earlier in the section; `$$` is a literal `$`.
- The variables also affect how the compiler is found: `PATH`, `CC`, `CXX` and `AR` set here are used.
- Changing the section rebuilds the project.

## Usage

```sh
idl init              # creates project.yml, README.md and src/main.c
idl build             # builds the debug profile (-g -O0) into build/debug/
idl build a b         # builds only targets a and b, and what they link
idl release [a b]     # the same, optimized (-O2 -DNDEBUG), into build/release/
idl run [exe] -- args # builds (debug) and runs; the name is needed with several executables
idl test [names]      # builds (debug) and runs the tests (tests/, or targets of type test)
idl add zlib m        # adds libraries installed on the machine (see below)
idl clean             # removes build/
idl help build        # how to use a command
```

`idl add` looks for each library before adding it to `project.dependencies`, in this order: the known ones (`pthread`, `m`, `dl`, `rt`), `pkg-config`, the directories of `LD_LIBRARY_PATH`, and the system library directories (`/usr/lib`, `/usr/local/lib`...). A library that is not found is refused, and the build does the same search, failing with a clear message if a dependency is missing. Only the name is saved, so `project.yml` stays portable.

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
