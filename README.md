
# GitHub Actions Workflows

This folder contains a CI workflow that:
1. Builds the C++ ZIP generator with CMake on Ubuntu/Windows/macOS.
2. Runs the program to produce `fire-extinguisher-tracking-starter.zip`.
3. Uploads the ZIP as a build artifact.
4. Validates JSON schemas against their example payloads using `ajv-cli`.

**Default project path** used by the workflow is `fire-extinguisher-tracking-cpp-starter-v2`.
You can override it when triggering `workflow_dispatch`.
