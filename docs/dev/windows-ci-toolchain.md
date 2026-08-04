# Windows CI toolchain (Story 1.1)

Pinned compile gate for the Bridge usermode host.

## Gate

| Item | Value |
|---|---|
| Workflow | `.github/workflows/windows-build.yml` |
| Runner | `windows-2022` (not `windows-latest`) |
| Configure | `cmake -S . -B builds/ci -A x64` |
| Build | `cmake --build builds/ci --config Release` |
| CMake minimum (project) | 3.20 |

Generator: left to CMake auto-detect on the runner (Visual Studio 2022 on `windows-2022`). Do not hard-pin VS 18 / 2026 generators for V1.

## Observed versions

The workflow prints CMake and Visual Studio versions on every green run (`Record toolchain versions` step). Copy the values from a successful Actions log into the table below when first verified on `main` / a PR.

| Tool | Observed on green run | Date |
|---|---|---|
| CMake | _(fill from Actions log)_ | |
| Visual Studio | _(fill from Actions log)_ | |

## Local (Windows x64)

```text
cmake -S . -B builds/debug -A x64
cmake --build builds/debug --config Debug
```

Outputs must stay under `builds/`. Never use repo-root `build/` as the documented out directory.
