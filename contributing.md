# Contributing

## Solo developer workflow

- Commits only when explicitly requested
- Commit messages and GitHub issues in English
- Chat with the coding agent in French; generated BMad docs in English — see `conventions.md`

## Dual-machine loop

Primary edit is on **macOS** (Cursor). Build, USB, DAW, SysEx, installer, and Studio-Done measurements close on **Windows 10/11 x64**. **Windows 10 x64 is mandatory** in the validation matrix (Win11 alone is not enough). Artifacts go under `builds/`. Windows CI compile is the minimum merge gate; hardware Pass rows stay lab-owned.

Full narrative and pointers: [`docs/dev/contributor-dual-machine-loop.md`](docs/dev/contributor-dual-machine-loop.md).

Related contributor docs (do not duplicate here):

- [`docs/dev/windows-ci-toolchain.md`](docs/dev/windows-ci-toolchain.md)
- [`docs/dev/winusb-bind.md`](docs/dev/winusb-bind.md) — Zadig is contributor fallback only
- [`docs/dev/license-and-backends.md`](docs/dev/license-and-backends.md)

Quality limits stay in `conventions.md` §3 — this file summarizes the loop, not those limits.

## Code quality gate

Mechanical Clean Code limits live in `conventions.md` §3 and are enforced on the **ticket diff** by:

```bash
python -m pip install -r scripts/quality/requirements.txt
python scripts/quality/lint-touched.py
```

Useful variants:

```bash
python scripts/quality/lint-touched.py --base origin/main
python scripts/quality/lint-touched.py --all --report-worst 20
python scripts/quality/lint-touched.py --clang-tidy   # when compile_commands.json exists
```

Definition of done for a coding task: compile (when a target exists) + relevant tests + `lint-touched.py` green on touched C++.

Historical debt outside the diff is a separate chore — not a Boy Scout obligation on every ticket.

## Style

Authoritative standards: `conventions.md`. Short C++ rule: `.cursor/rules/cpp-standards.mdc`.
