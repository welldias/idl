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

Without `src/main.*`, the project becomes the static library `lib<name>.a`. C and C++ can be mixed. Sources are compiled in parallel, and only what changed is rebuilt (including when a header is modified).

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
