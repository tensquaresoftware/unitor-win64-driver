# Personal Instructions

**Author:** Guillaume DUPONT  
**Organization:** Ten Square Software  
**Context:** Windows 64-bit usermode MIDI USB bridge for Emagic Unitor-family interfaces (MT4 / AMT8 / Unitor8) — Cursor + BMad Method  
**Revision date:** 2026-08-04

---

## Purpose of this document

This file defines my personal instructions for the AI coding agent. It guides code generation, documentation, advice, and responses with rules, standards, and conventions for this project.

**Authoritative for style disputes.** Condensed Cursor rules live under `.cursor/rules/`.

---

## 1. Communication

### 1.1 Interactions with me

- My name is Guillaume — call me by my first name
- Always use informal tone ("tu") in French
- Reply exclusively in French in our conversations
- **Plain French, no anglicisms or management jargon** — Avoid "trade-off", "ledger", "workflow", "pain point", "scope", "deliverable", etc. Prefer: "compromis", "fichier source de vérité", "découplage", "formule", "garde-fou à la compilation", "déroulement", "point de friction"
- **Complete sentences** — No telegraphic prose or chat abbreviations (fn, impl, req/res, etc.)
- Keep **English only for project identifiers**: class names, file names, symbols, APIs, error messages
- Be frank, honest, and factual — contradict me when the better solution requires it
- Absolute priority: help me find the most relevant solution

### 1.2 Structure of responses

- Clear, proportionate answers — not essays
- Prefer longevity and maintainability of generated code
- **Default tone: plain, natural French** — Technical terms only when needed; short gloss on first use
- When introducing an abstract concept, give one short concrete example
- **No redundant repetition** — One well-written explanation per turn. No filler openers ("Bien sûr !", "Excellente question !")
- **Response length proportional to the question**
- **DO NOT** paste full code blocks in the chat when modifying — diffs from the tool are enough
- Prefer summaries in natural language only

### 1.3 Domain / product names (keep in English)

Do **not** force-translate hardware, protocol, or product terms in French chat:

- **MT4**, **AMT8**, **Unitor8**, **WinUSB**, **virtualMIDI**, **Windows MIDI Services**, **Zadig**, **DAW**, **MIDI**
- Project/repo name: **unitor-win64-driver**

Vendor spelling (Tobias Erichsen): the product name is **virtualMIDI** (not `VirtualMIDI`). Keep technical identifiers unchanged: `teVirtualMIDI.dll`, C++ symbols such as `VirtualMidiBackend`.

---

## 2. Process & Workflow

> Guidance for judgment. **Mechanical thresholds** are enforced by  
> `scripts/quality/lint-touched.py` (+ optional `.clang-tidy`) — see §3.  
> There is **no** mandatory DESIGN → AUTO-REVIEW redesign loop on every task.

### 2.1 Lightweight coding loop

Before and while coding:

- Prefer the **simplest design that works (KISS)**; implement **only the current need (YAGNI)**
- Ask: **Does this choice make future changes easier (ETC)?**
- Detect duplication with WET nuance (§3.1 / §6.10) — duplicate once OK; factorize when stable
- Use explicit, intention-revealing names; one clear responsibility per function/class (soft SRP). Do **not** use a descriptive comment as a substitute for a better name — rename or extract; comments explain *why*, not *what*
- **Boy Scout limited to ticket scope** — improve the touched zone; do not rewrite an entire large file for one line
- End of task: **compile** (when a target exists) + relevant tests + **`python scripts/quality/lint-touched.py`** on the diff

If the analyser fails: fix the finding. Do not redesign unrelated code.

### 2.2 Exception: Rapid prototyping

If Guillaume explicitly requests a "rapid prototype", "POC", or "spike", the quality gate on the diff may be deferred until after validation — then bring new code under §3 before merge.

### 2.3 Document metadata

When modifying a file that contains a **revision date** or **version number** in its header, update them.

### 2.4 BMad — Cursor Agents chat titles

When Guillaume starts a conversation with a **BMad skill**, the agent may **rename the current chat** — **at most once per conversation**.

Use MCP `rename_chat` (`cursor-app-control`). Authoritative detail: `.cursor/rules/bmad-agent-chat-titles.mdc`.

**Rename-once:** if the sidebar title already starts with `BMad —`, never call `rename_chat` again.

**No date in the title.**

**Format A — story ID compact:** `BMad — Create Story 4-5`, `BMad — Dev Story 4-5`, `BMad — Code Review 4-5`, `BMad — Quick Dev 8-4`

**Format B — other commands:** `BMad — {Workflow Label} : {Topic}` — label in English, topic in French (2–5 words). Omit colon/topic when useless (`BMad — Help`).

**Duplicates:** before `rename_chat`, run `python scripts/dev/resolve-bmad-chat-title.py "BASE TITLE"` (macOS + Windows) and use the printed line.

**Persistence:** only `rename_chat` writes the title durably. Editing Cursor’s sidebar cache / `state.vscdb` does not stick after reopening a conversation.

**Amorce** for create / dev / quick-dev / code-review: first user-facing content must be `## Contexte de la Story` + one plain-French sentence — `.cursor/rules/bmad-story-context-amorce.mdc`.

---

## 3. Quantifiable Limits (quality gate)

> Realistic thresholds for a C++ usermode Windows MIDI/USB bridge (§3.1) and for project Python under `scripts/` (§3.4).  
> **Enforced on touched files** by `scripts/quality/lint-touched.py` (and `.clang-tidy` when used for C++).  
> Historical debt outside the ticket diff is a **separate chore**.

### 3.1 Code metrics — MAXIMUM limits

| Metric | Maximum | Notes |
|---|---|---|
| Function / method (protocol / core logic) | **~40 lines** | lizard `nloc` (non-comment) |
| Function / method (platform / WinUSB / OS glue) | **~50 lines** | Paths under `Usb/`, `Platform/`, `WinUsb/`, `Interop/`, `Install/` |
| Function parameters (**our** code) | **4** | Beyond → options struct; do not rewrite third-party / Win32 APIs |
| Cyclomatic complexity | **10** (glue up to **12** if justified) | Extract named helpers when exceeded |
| Nesting depth | **4** | Prefer guards / early returns / RAII |
| Useful `.cpp` file | **~400 lines** | Large inherited or generated files → dedicated cleanup |

#### Code duplication: extract when stable — WET before premature abstraction

- **First occurrence (WET):** duplicating once is acceptable
- **Second stable occurrence:** evaluate whether extraction improves clarity (ETC)
- **Third similar occurrence** or confirmed stable duplication → **mandatory extraction**

#### Hot MIDI / USB path (overrides metrics)

In USB transfer completion callbacks, MIDI encode/decode loops, and equivalents on the latency-sensitive path: **no** unbounded allocation, dangerous locks, or logging that risks jitter.  
If a §3 metric conflicts with timing safety or clarity of the critical path: **safety wins**; document a dated, motivated exception (`NOLINT` or comment).

### 3.2 Procedure when the gate fails on the diff

1. Read the analyser finding (length / params / complexity / nesting / file size)
2. Fix **that** signal — extract a helper, simplify branches, or split a file if the ticket owns that work
3. Re-run `python scripts/quality/lint-touched.py`
4. Do **not** refactor untouched historical code “to green the whole tree” in the same ticket

### 3.3 Warning signs

- A **new** method at ~70+ lines with deep nesting → simplify before merge
- Same block copied a third stable time → factorize
- Abstraction “for later” with no current need → YAGNI, remove
- Cannot find a good function name → likely too many responsibilities
- Comment like "// Part 1", "// Part 2" → each part = separate function

### 3.4 Scripts quality gate (Python)

> Anti-drift for **project Python** under `scripts/` (labs, packaging CLI, quality helpers).  
> Same tool as C++: `python scripts/quality/lint-touched.py`.  
> Historical oversized labs are a **separate chore** — `--all` reports them; it does **not** fail the process.

#### Scope

| Included | Excluded |
|---|---|
| `scripts/**/*.py` | `_bmad/`, `.agents/`, `tools/` (no project `.py` there today), third-party trees |

#### Metrics — MAXIMUM limits (looser than §3.1 C++)

| Metric | Maximum | Notes |
|---|---|---|
| Function (scripts core, e.g. `quality/`, `dev/`) | **~70** nloc | lizard |
| Function (lab / packaging glue — argparse + I/O) | **~90** nloc | Paths with `/lab/` or `/packaging/` |
| Function parameters | **4** | Prefer dataclass / `Namespace`; lizard `self`/`cls` are excluded |
| Cyclomatic complexity | **12** (glue up to **14**) | Extract helpers when exceeded |
| Nesting depth | **5** | Indent-based estimate (heuristic; not AST) |
| Useful `.py` file | **~700** lines | Non-blank, non-`#` lines |

WET→DRY for labs follows §3.1 duplication rules: duplicate once OK; factorize when the third stable copy appears (dedicated chore — not this gate’s job to rewrite the tree).

#### Touched vs `--all` (same spirit as C++)

- **Touched (default):** analyse changed hunks under `scripts/**/*.py`; **exit 1** on findings. File-size findings only if the file is **new**, or useful-line count **grew** vs `--base` while still over ~700.
- **`--all`:** diagnostic scan of tracked `scripts/**/*.py`; print findings; **exit 0** (backlog / debt visibility).

#### Finish criteria

End of task: `python scripts/quality/lint-touched.py` must be green on the ticket diff (C++ **and** scripts). Do not mass-refactor untouched labs to green `--all`.

---

## 4. Self-Critique (light)

> Short sanity pass. **Finish criteria:** compile + tests + `scripts/quality/lint-touched.py` on touched files.

### 4.1 Quick design check

- Clear responsibility for new/changed types? (soft SRP)
- Dependencies flow inward: platform/OS glue at the edges; protocol/core does not depend on UI or installers
- Prefer declarative `DeviceProfile` (per PID) over model-specific branches — see project brief
- Names reveal intent? Magic numbers named?
- WET/DRY balanced (§3.1)?

### 4.2 Design principles (see §6.10)

- **KISS / YAGNI / ETC** applied to this ticket
- **Boy Scout** limited to the modified zone
- **CQS** when natural — do not force it onto Win32 APIs that combine query and mutation

> Full acronym reference: `docs/dev/software-development-quality-principles.md`

### 4.3 Golden rule

> The **analyser and the compiler** decide the mechanical bar.  
> If `lint-touched.py` fails on the diff → fix those findings before calling the task done.  
> Untouched historical debt → note or separate chore.

---

## 5. Development Environment

### 5.1 System & Tools

- **Primary machine:** MacBook Pro M5, macOS Tahoe — Cursor for design and most editing
- **Target / validation machine:** Windows 10/11 64-bit PC — build, USB hardware tests, DAW checks
- **IDE / AI agent:** Cursor Pro+ (project rules in `.cursor/rules/`)
- **Language:** C++17 minimum (raise only if architecture explicitly requires it)
- **Build system:** CMake (expected); build trees under `builds/` only (e.g. `builds/debug`, `builds/ci`). Use `CMakePresets.json` / `cmake --preset debug`. Cursor pins CMake Tools to that preset via `.vscode/settings.json`.
- **Directory / file names:** **kebab-case** for project folders and non-C++ files (`scripts/quality/lint-touched.py`, `docs/dev/…`, `conventions.md`). Exception: C++ sources under `src/` use **PascalCase** filenames matching types (`DeviceProfile.h`, `EmagicMidiMapper.cpp`). Never a top-level `Documentation/`
- **Stack (to be confirmed in architecture):** WinUSB usermode + protocol layer + virtual MIDI exposure (virtualMIDI SDK and/or Windows MIDI Services). JUCE may appear later for shared tooling — do not assume it until decided.

### 5.2 Project knowledge

- Kickoff brief: `docs/dev/prompt-demarrage-projet-bmad.md`
- BMad config: `_bmad/` — chat French, generated docs English
- Quality gate SSOT: this file §3 + `scripts/quality/lint-touched.py`

---

## 6. C++ Standards & Quality

### 6.1 C++ generalities

- **Standard:** C++17 minimum
- **Source code language:** English only (names and comments — no French in sources)

### 6.2 Git

#### Commits

- Summary/description in **English only**
- **Create commits only on explicit request**
- Format: imperative summary line, then bullets for significant changes

#### GitHub Issues

- Title and description in English only

### 6.3 Naming conventions

#### Directories

- **kebab-case** for all project directories: `scripts/quality/`, `docs/dev/`, `src/protocol/`, …
- Do **not** use `PascalCase` (`Scripts/`, `Documentation/`) or `snake_case` (`my_module/`) for folders
- Layout: kickoff brief and development process docs under `docs/dev/`
- C++ sources live under `src/` (subfolders still kebab-case)

#### Files (non-C++)

- **kebab-case** for markdown, scripts, configs owned by this project: `conventions.md`, `contributing.md`, `lint-touched.py`, `prompt-demarrage-projet-bmad.md`
- Keep standard tool filenames when required by the tool (`.gitignore`, `.clang-tidy`, `CMakeLists.txt`)

#### Files (C++ under `src/`)

- **PascalCase** for headers and translation units, matching the primary type: `DeviceProfile.h`, `EmagicMidiMapper.cpp`
- One primary type per pair of `.h` / `.cpp` when practical
- Do **not** use kebab-case for C++ source filenames in `src/`

#### Symbols

| Kind | Style | Examples |
|---|---|---|
| Variables / methods | `lowerCamelCase` | `openDevice()`, `cableMask` |
| Classes / namespaces | `PascalCase` | `DeviceProfile`, `EmagicMidiMapper` |
| Enum class values | `k` prefix | `DeviceModel::kMt4` |
| Public constants | `kPascalCase` | `kDefaultInterfaceNumber` |
| Private members | trailing `_` | `deviceHandle_`, `kMaxPackets_` |
| Avoid | `snake_case` (except rare FFI), `m_` + `_` | — |

### 6.4 Magic numbers — forbidden

Always name meaningful literals (`kNoCable`, `kUsbInterfaceIndex`). Acceptable bare literals: `0`, `1`, `nullptr`, `true`, `false`, and test expected values.

### 6.5 Includes

1. C++ system headers  
2. Third-party / OS headers (WinUSB, Windows MIDI, …)  
3. Project headers (paths from project root configured in CMake — never deep `../../../`)

Separate groups with a blank line. Prefer forward declarations in `.h`, full includes in `.cpp`. Use `#pragma once`.

### 6.6 .h / .cpp separation

Separate declaration and definition except templates, explicit `inline`, and trivial getters/setters (&lt; ~5 lines).

### 6.7 Class organization

- Public API first, members private after methods, private helpers last
- `explicit` single-arg constructors; Rule of 0/3/5; `= default` / `= delete` when intentional
- Virtual destructor when polymorphic

### 6.8 Error handling & RAII

- Exceptions for exceptional failures; `std::optional` / result types for expected absence/failure
- Never silently ignore errors; log with context at the right layer
- RAII for handles (WinUSB, file, threads); prefer `unique_ptr` for exclusive ownership

### 6.9 SOLID & Clean Code (pragmatic)

- Readable, intention-revealing names; soft SRP; single level of abstraction per function
- Prefer helpers over narrative comments when §3 thresholds are exceeded
- Prefer delegation over deep inheritance; Law of Demeter

### 6.10 Design principles (agent priorities)

> Full reference: `docs/dev/software-development-quality-principles.md`

| Principle | Rule |
|---|---|
| **KISS** | Simplest design that meets the current need and §3 |
| **YAGNI** | No hooks or abstractions for hypothetical future devices/features |
| **ETC** | Prefer choices that make the next likely change easier |
| **Boy Scout** | Clean only the ticket-touched zone |
| **WET → DRY** | Duplicate once OK; extract on stable 2nd–3rd occurrence |
| **CQS** | Prefer command/query separation when natural |
| **Fail-fast** | Guard invalid USB/MIDI preconditions early |

**When principles conflict:** (1) correctness & MIDI/USB timing safety → (2) KISS + YAGNI + ETC → (3) SOLID + §3 → (4) DRY after confirmed duplication.

### 6.11 Formatting (Allman)

- 4 spaces, no tabs
- Opening brace on its own line
- `const` before the type: `const Thing&`
- Pointers: `SomeObject* p` (space after type)
- `nullptr` only — never `NULL` / `0` as null pointer
- Space around binary operators; no space after `!`
- Prefer early return over `else` after `return`
- Prefer explicit lambda captures over `[=]` / `[&]`

---

## 7. Architecture notes (project-specific)

Until BMad architecture locks the stack:

- Prefer a **declarative `DeviceProfile` per USB PID** and a single Emagic cable-mapping implementation over per-model code forks (MT4 first; AMT8 / Unitor8 later as validation)
- Keep **protocol logic** independent of WinUSB / virtual-MIDI backends so either virtualMIDI SDK or Windows MIDI Services can be swapped behind an interface
- Usermode WinUSB only for V1 — no custom kernel driver unless a later decision overturns this brief

---

## 8. Testing

- Prefer TDD for protocol/mapping pure logic (F.I.R.S.T., AAA)
- Hardware-dependent USB/DAW checks are integration / manual validation stories — do not pretend unit tests cover them
- Do not unit-test thin Win32 wrappers with no logic

---

## 9. Languages summary

| Surface | Language |
|---|---|
| Chat with Guillaume | French (plain) |
| BMad generated docs / stories / specs | English |
| Source code & comments | English |
| Git commits / issues | English |
