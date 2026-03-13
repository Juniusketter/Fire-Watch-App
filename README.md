# Fire Extinguisher Tracking – C++ Starter

This repository contains a single-file C++17 **zip generator** that writes `fire-extinguisher-tracking-starter.zip` containing your **Assignment JSON Schema**, **flags.yml**, and a **valid example**. No external libraries are required.

## Build & Run
```bash
cmake -S . -B build && cmake --build build
./build/zipgen
```
Or:
```bash
g++ -std=c++17 -O2 -o zipgen src/main.cpp && ./zipgen
```

## Contents
- `src/main.cpp` – minimal ZIP writer (store/no compression) + embedded artifacts
- `schemas/assignment.schema.json` – provided also as a plain file for reference
- `flags/flags.yml`
- `examples/assignment.example.json`
- `prebuilt/fire-extinguisher-tracking-starter.zip` – pre-generated for convenience
