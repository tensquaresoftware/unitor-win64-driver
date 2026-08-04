# Contributing

## Solo developer workflow

- Commits only when explicitly requested
- Commit messages and GitHub issues in English
- Chat with the coding agent in French; generated BMad docs in English — see `conventions.md`

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
