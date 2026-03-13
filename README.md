
# C++ Starter (CMake + CI)

A minimal, cross-platform C++ starter with CMake, GitHub Actions CI, and a basic test placeholder.

## Build (local)

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j
./app  # Windows: .\Debug\app.exe (or .\app.exe if single-config)
```

## Run (args)

```bash
./app hello world
```

## Run tests (placeholder)

Add your test framework (e.g., Catch2, GTest) and a test target in CMake.

## Format (clang-format)

```bash
clang-format -i src/*.cpp src/*.hpp
```
