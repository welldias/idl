# Example: configured project

A project that does not follow the directory convention: its sources are spread over
several directories and it produces two libraries and two executables. The `targets:`
section of `project.yml` says what to build and how.

```
configured/
  project.yml
  third_party/textutil/        -> static library "textutil"
    textutil.c, textutil.h
  engine/
    include/geometry/          public headers of "geometry"
    src/
      distance.c, polygon.c    -> library "geometry" (static and shared)
      internal/checks.h        private header
      experimental/            excluded from the build
  apps/
    measure/main.c, parse.c    -> executable "measure"
    report.cpp                 -> executable "report" (C++)
```

## What each target says

```yaml
geometry:
  type: library                        # executable | static-library | shared-library | library
  sources: [engine/src/**/*.c]         # files and globs: *, ?, ** (any depth)
  exclude: [engine/src/experimental/**]
  include-dirs: [engine/src]           # -I for this target only
  public-include-dirs: [engine/include] # -I for this target and whoever links it
  libs: [m]                            # -lm
  link: [textutil]                     # other targets of the project
```

- `library` builds both `libgeometry.a` and `libgeometry.so` (`.dylib` on macOS,
  `geometry.dll` on Windows).
- `measure` and `report` link `libgeometry.a`, which in turn needs `libtextutil.a`: idl
  links both, in the right order, and adds `-lm` because a static library cannot carry it.
- The public include dirs are inherited through `link`, so the programs can
  `#include <geometry/geometry.h>` without listing `engine/include` themselves.
  `engine/src` stays private to the library.
- Everything that goes into the shared library is compiled with `-fPIC`, including
  `textutil`.
- Settings under `build:` (here `-Wshadow`) and `project.dependencies` apply to every target.
- The platform suffix rule still holds: a source named `*_win.c`, `*_linux.c`,
  `*_macos.c` or `*_unix.c` matched by a glob is only built on that system.

## Usage

```sh
idl build                                   # all targets
idl run --bin measure -- 0,0 3,0 3,4        # 3 points, perimeter 12.00
idl run --bin report
idl build --release                         # build/release/
```

With several executables, `idl run` needs `--bin`, unless one of them has the project's name.
