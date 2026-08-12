#!/usr/bin/env python3
"""
Quality gate for unitor-win64-driver — anti-drift on the ticket diff.

Usage:
  python scripts/quality/lint-touched.py
  python scripts/quality/lint-touched.py --base origin/main
  python scripts/quality/lint-touched.py --all
  python scripts/quality/lint-touched.py --all --report-worst 20

Without --all:
  - C++ under src|tests|apps|lib|tools and Python under scripts/
    in the git worktree / commits since --base
  - Function findings apply only to functions that intersect **changed lines**
  - File-size findings apply only if the file is new, or useful-line count grew
    while still over the threshold

With --all: full diagnostic of the tree (debt cleanup backlog; exit 0).

Thresholds: conventions.md §3 (C++) and §3.4 (scripts Python).
Historical lab debt is a separate chore — --all does not fail on it.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

try:
    import lizard
except ImportError:
    print(
        "Missing dependency: lizard. Install with:\n"
        "  python -m pip install -r scripts/quality/requirements.txt",
        file=sys.stderr,
    )
    sys.exit(2)

REPO_ROOT = Path(__file__).resolve().parents[2]

# --- C++ (§3.1) — do not change without Ask First ---
MAX_NLOC_CORE = 40
MAX_NLOC_GLUE = 50
MAX_PARAMS = 4
MAX_CCN_CORE = 10
MAX_CCN_GLUE = 12
MAX_NESTING = 4
MAX_CPP_USEFUL_LINES = 400

# --- Scripts Python (§3.4) ---
MAX_PY_NLOC_CORE = 70
MAX_PY_NLOC_GLUE = 90
MAX_PY_PARAMS = 4
MAX_PY_CCN_CORE = 12
MAX_PY_CCN_GLUE = 14
MAX_PY_NESTING = 5
MAX_PY_USEFUL_LINES = 700

CPP_SUFFIXES = {".h", ".hpp", ".hh", ".cpp", ".cc", ".cxx"}
PY_SUFFIXES = {".py"}
CPP_SCOPE_PREFIXES = ("src/", "tests/", "apps/", "lib/", "tools/")
PY_SCOPE_PREFIX = "scripts/"
SKIP_PATH_PARTS = (
    "third_party",
    "third-party",
    "vendor",
    "external",
    "JuceLibraryCode",
    "juce_binarydata",
)
GLUE_PATH_MARKERS = (
    "/Usb/",
    "/USB/",
    "/WinUsb/",
    "/WinUSB/",
    "/Platform/",
    "/Interop/",
    "/Install/",
    "/Installer/",
)
PY_GLUE_MARKERS = ("/lab/", "/packaging/")


@dataclass(frozen=True)
class Finding:
    path: str
    kind: str
    detail: str
    severity: str
    score: float
    lang: str  # "cpp" | "scripts"


@dataclass(frozen=True)
class MetricLimits:
    lang: str
    max_nloc: int
    max_params: int
    max_ccn: int
    max_nesting: int
    max_useful: int
    size_suffix_ok: bool


@dataclass(frozen=True)
class FnSlice:
    name: str
    start: int
    end: int
    nloc: int
    params: int
    ccn: int
    nesting: int


@dataclass(frozen=True)
class SizeCheckCtx:
    base: str | None
    full_tree: bool


def decode_git_bytes(raw: bytes) -> str:
    """Decode git stdout; never crash the gate on a corrupt working-tree encoding."""
    return raw.decode("utf-8", errors="replace")


def git_output(command: list[str]) -> str:
    try:
        raw = subprocess.check_output(
            command,
            cwd=REPO_ROOT,
            stderr=subprocess.DEVNULL,
        )
    except subprocess.CalledProcessError:
        return ""
    return decode_git_bytes(raw)


def git_lines(command: list[str]) -> list[str]:
    out = git_output(command).strip()
    return [line for line in out.splitlines() if line]


def _skip_vendor(path: Path) -> bool:
    return any(skip in path.parts for skip in SKIP_PATH_PARTS)


def is_cpp_gate_target(rel: str) -> bool:
    norm = rel.replace("\\", "/")
    if not any(norm.startswith(p) for p in CPP_SCOPE_PREFIXES):
        return False
    path = Path(norm)
    if path.suffix.lower() not in CPP_SUFFIXES:
        return False
    if _skip_vendor(path):
        return False
    return (REPO_ROOT / norm).is_file()


def is_py_gate_target(rel: str) -> bool:
    norm = rel.replace("\\", "/")
    if not norm.startswith(PY_SCOPE_PREFIX):
        return False
    path = Path(norm)
    if path.suffix.lower() not in PY_SUFFIXES:
        return False
    if _skip_vendor(path):
        return False
    return (REPO_ROOT / norm).is_file()


def is_gate_target(rel: str) -> bool:
    return is_cpp_gate_target(rel) or is_py_gate_target(rel)


def collect_changed_files(base: str) -> list[str]:
    sets: list[list[str]] = [
        git_lines(["git", "diff", "--name-only", "--diff-filter=ACMR", "HEAD"]),
        git_lines(["git", "diff", "--name-only", "--diff-filter=ACMR", "--cached"]),
        git_lines(["git", "ls-files", "--others", "--exclude-standard"]),
    ]
    if git_lines(["git", "rev-parse", "--verify", base]):
        sets.append(
            git_lines(
                ["git", "diff", "--name-only", "--diff-filter=ACMR", f"{base}...HEAD"]
            )
        )
    return sorted({f.replace("\\", "/") for batch in sets for f in batch if is_gate_target(f)})


def collect_all_files() -> list[str]:
    tracked: list[str] = []
    for prefix in ("src", "tests", "apps", "lib", "tools", "scripts"):
        tracked.extend(git_lines(["git", "ls-files", prefix]))
    return sorted({f.replace("\\", "/") for f in tracked if is_gate_target(f)})


def is_glue_path(rel: str) -> bool:
    norm = "/" + rel.replace("\\", "/")
    return any(marker in norm for marker in GLUE_PATH_MARKERS)


def is_py_glue_path(rel: str) -> bool:
    norm = "/" + rel.replace("\\", "/")
    return any(marker in norm for marker in PY_GLUE_MARKERS)


def useful_line_count(text: str, *, lang: str) -> int:
    count = 0
    for raw in text.splitlines():
        line = raw.strip()
        if not line:
            continue
        if lang == "scripts":
            if line.startswith("#"):
                continue
        elif line.startswith("//"):
            continue
        count += 1
    return count


def max_nesting_in_range(lines: list[str], start: int, end: int) -> int:
    depth = 0
    max_depth = 0
    for idx in range(start - 1, min(end, len(lines))):
        line = lines[idx]
        if "//" in line:
            line = line.split("//", 1)[0]
        for ch in line:
            if ch == "{":
                depth += 1
                max_depth = max(max_depth, depth)
            elif ch == "}":
                depth = max(0, depth - 1)
    return max(0, max_depth - 1)


def max_py_nesting_in_range(lines: list[str], start: int, end: int) -> int:
    """Indent-based nesting estimate relative to the function def line."""
    base: int | None = None
    max_rel = 0
    for idx in range(start - 1, min(end, len(lines))):
        raw = lines[idx]
        stripped = raw.strip()
        if not stripped or stripped.startswith("#"):
            continue
        expanded = raw.expandtabs(4)
        width = len(expanded) - len(expanded.lstrip(" "))
        if base is None:
            base = width
            continue
        if width <= base:
            continue
        max_rel = max(max_rel, (width - base) // 4)
    return max_rel


def parse_diff_line_ranges(base: str) -> dict[str, set[int]]:
    """Map path -> set of new-file line numbers touched (unified diff -U0)."""
    ranges: dict[str, set[int]] = defaultdict(set)
    hunk_re = re.compile(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@")

    def ingest(diff_text: str) -> None:
        current: str | None = None
        for line in diff_text.splitlines():
            if line.startswith("+++ b/"):
                current = line[6:].replace("\\", "/")
                continue
            if line.startswith("+++ /dev/null"):
                current = None
                continue
            match = hunk_re.match(line)
            if not match or current is None:
                continue
            start = int(match.group(1))
            count = int(match.group(2) or "1")
            if count == 0:
                continue
            for n in range(start, start + count):
                ranges[current].add(n)

    blobs: list[str] = [
        git_output(["git", "diff", "-U0", "HEAD"]),
        git_output(["git", "diff", "-U0", "--cached"]),
    ]
    if git_lines(["git", "rev-parse", "--verify", base]):
        blobs.append(git_output(["git", "diff", "-U0", f"{base}...HEAD"]))

    for blob in blobs:
        if blob:
            ingest(blob)

    for rel in git_lines(["git", "ls-files", "--others", "--exclude-standard"]):
        norm = rel.replace("\\", "/")
        if not is_gate_target(norm):
            continue
        text = (REPO_ROOT / norm).read_text(encoding="utf-8", errors="replace")
        ranges[norm] = set(range(1, len(text.splitlines()) + 1))

    return ranges


def function_intersects(start: int, end: int, changed: set[int] | None) -> bool:
    if changed is None:
        return True
    if not changed:
        return False
    return any(line in changed for line in range(start, end + 1))


def severity_for_nloc(nloc: int, limit: int) -> str:
    if nloc >= limit * 2:
        return "critical"
    if nloc >= int(limit * 1.4):
        return "serious"
    return "light"


def severity_for_ccn(ccn: int, limit: int) -> str:
    if ccn >= limit + 8:
        return "critical"
    if ccn >= limit + 3:
        return "serious"
    return "light"


def severity_for_file(useful: int, *, lang: str) -> str:
    if lang == "scripts":
        if useful >= 1400:
            return "critical"
        if useful >= 1000:
            return "serious"
        return "light"
    if useful >= 800:
        return "critical"
    if useful >= 550:
        return "serious"
    return "light"


def base_file_useful_lines(rel: str, base: str, *, lang: str) -> int | None:
    if not git_lines(["git", "rev-parse", "--verify", base]):
        return None
    try:
        raw = subprocess.check_output(
            ["git", "show", f"{base}:{rel}"],
            cwd=REPO_ROOT,
            stderr=subprocess.DEVNULL,
        )
    except subprocess.CalledProcessError:
        return None
    return useful_line_count(decode_git_bytes(raw), lang=lang)


def limits_for_path(rel: str) -> MetricLimits:
    path = Path(rel.replace("\\", "/"))
    suffix = path.suffix.lower()
    if suffix in PY_SUFFIXES:
        glue = is_py_glue_path(rel)
        return MetricLimits(
            lang="scripts",
            max_nloc=MAX_PY_NLOC_GLUE if glue else MAX_PY_NLOC_CORE,
            max_params=MAX_PY_PARAMS,
            max_ccn=MAX_PY_CCN_GLUE if glue else MAX_PY_CCN_CORE,
            max_nesting=MAX_PY_NESTING,
            max_useful=MAX_PY_USEFUL_LINES,
            size_suffix_ok=True,
        )
    glue = is_glue_path(rel)
    return MetricLimits(
        lang="cpp",
        max_nloc=MAX_NLOC_GLUE if glue else MAX_NLOC_CORE,
        max_params=MAX_PARAMS,
        max_ccn=MAX_CCN_GLUE if glue else MAX_CCN_CORE,
        max_nesting=MAX_NESTING,
        max_useful=MAX_CPP_USEFUL_LINES,
        size_suffix_ok=suffix == ".cpp",
    )


def maybe_file_size_finding(
    rel: str,
    text: str,
    limits: MetricLimits,
    ctx: SizeCheckCtx,
) -> Finding | None:
    if not limits.size_suffix_ok:
        return None
    useful = useful_line_count(text, lang=limits.lang)
    if useful <= limits.max_useful:
        return None
    report_file = ctx.full_tree
    if not ctx.full_tree and ctx.base is not None:
        before = base_file_useful_lines(rel, ctx.base, lang=limits.lang)
        if before is None or useful > before:
            report_file = True
    if not report_file:
        return None
    return Finding(
        path=rel,
        kind="file-size",
        detail=f"useful lines {useful} > {limits.max_useful}",
        severity=severity_for_file(useful, lang=limits.lang),
        score=float(useful),
        lang=limits.lang,
    )


def append_metric_findings(
    findings: list[Finding],
    rel: str,
    limits: MetricLimits,
    fn: FnSlice,
) -> None:
    if fn.nloc > limits.max_nloc:
        findings.append(
            Finding(
                path=rel,
                kind="function-length",
                detail=f"{fn.name} nloc={fn.nloc} > {limits.max_nloc} (L{fn.start}-{fn.end})",
                severity=severity_for_nloc(fn.nloc, limits.max_nloc),
                score=float(fn.nloc),
                lang=limits.lang,
            )
        )
    if fn.params > limits.max_params:
        findings.append(
            Finding(
                path=rel,
                kind="parameters",
                detail=f"{fn.name} params={fn.params} > {limits.max_params} (L{fn.start})",
                severity="serious" if fn.params >= 7 else "light",
                score=float(fn.params),
                lang=limits.lang,
            )
        )
    if fn.ccn > limits.max_ccn:
        findings.append(
            Finding(
                path=rel,
                kind="complexity",
                detail=f"{fn.name} ccn={fn.ccn} > {limits.max_ccn} (L{fn.start})",
                severity=severity_for_ccn(fn.ccn, limits.max_ccn),
                score=float(fn.ccn),
                lang=limits.lang,
            )
        )
    if fn.nesting > limits.max_nesting:
        findings.append(
            Finding(
                path=rel,
                kind="nesting",
                detail=(
                    f"{fn.name} nesting~={fn.nesting} > {limits.max_nesting} "
                    f"(L{fn.start})"
                ),
                severity="serious" if fn.nesting >= limits.max_nesting + 3 else "light",
                score=float(fn.nesting),
                lang=limits.lang,
            )
        )


def effective_param_count(raw_fn: object, *, lang: str) -> int:
    """Lizard counts self/cls; subtract them for Python methods."""
    count = int(getattr(raw_fn, "parameter_count", 0))
    if lang != "scripts":
        return count
    params = list(getattr(raw_fn, "full_parameters", None) or [])
    if params and str(params[0]) in ("self", "cls"):
        return max(0, count - 1)
    return count


def nesting_for_function(
    lines: list[str],
    start: int,
    end: int,
    *,
    lang: str,
) -> int:
    if lang == "scripts":
        return max_py_nesting_in_range(lines, start, end)
    return max_nesting_in_range(lines, start, end)


def analyse_file(
    rel: str,
    *,
    changed_lines: set[int] | None,
    base: str | None,
    full_tree: bool,
) -> list[Finding]:
    path = REPO_ROOT / rel
    text = path.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()
    findings: list[Finding] = []
    limits = limits_for_path(rel)

    size = maybe_file_size_finding(
        rel, text, limits, SizeCheckCtx(base=base, full_tree=full_tree)
    )
    if size is not None:
        findings.append(size)

    try:
        analysed = lizard.analyze_file(str(path))
    except Exception as exc:  # noqa: BLE001 - gate must not crash on exotic files
        findings.append(
            Finding(
                path=rel,
                kind="parse-error",
                detail=f"lizard failed: {type(exc).__name__}",
                severity="serious",
                score=0.0,
                lang=limits.lang,
            )
        )
        return findings

    for raw_fn in analysed.function_list:
        start = int(raw_fn.start_line)
        end = int(raw_fn.end_line)
        if not function_intersects(start, end, changed_lines):
            continue
        fn = FnSlice(
            name=raw_fn.name,
            start=start,
            end=end,
            nloc=int(raw_fn.nloc),
            params=effective_param_count(raw_fn, lang=limits.lang),
            ccn=int(raw_fn.cyclomatic_complexity),
            nesting=nesting_for_function(lines, start, end, lang=limits.lang),
        )
        append_metric_findings(findings, rel, limits, fn)

    return findings


def maybe_run_clang_tidy(files: list[str]) -> int:
    compile_db = REPO_ROOT / "compile_commands.json"
    if not compile_db.is_file():
        return 0

    tidy = None
    for candidate in (
        "clang-tidy",
        "/opt/homebrew/opt/llvm/bin/clang-tidy",
        "/usr/local/opt/llvm/bin/clang-tidy",
    ):
        try:
            subprocess.check_output(
                [candidate, "-version"],
                stderr=subprocess.DEVNULL,
                text=True,
            )
            tidy = candidate
            break
        except (OSError, subprocess.CalledProcessError):
            continue
    if tidy is None:
        return 0

    cpp_files = [f for f in files if f.endswith((".cpp", ".cc", ".cxx"))]
    if not cpp_files:
        return 0

    print(f"Optional clang-tidy ({tidy}) on {len(cpp_files)} translation unit(s)...")
    header_filter = f"{re.escape(str(REPO_ROOT / 'src'))}/.*"
    result = subprocess.run(
        [
            tidy,
            "-p",
            str(REPO_ROOT),
            f"-header-filter={header_filter}",
            *cpp_files,
        ],
        cwd=REPO_ROOT,
        check=False,
    )
    return result.returncode


def print_report(findings: list[Finding], *, report_worst: int) -> None:
    if not findings:
        print("Quality gate OK - no threshold violations.")
        return

    by_sev: dict[str, list[Finding]] = defaultdict(list)
    for f in findings:
        by_sev[f.severity].append(f)

    for sev in ("critical", "serious", "light"):
        items = by_sev.get(sev, [])
        if not items:
            continue
        print(f"\n=== {sev.upper()} ({len(items)}) ===")
        for item in sorted(items, key=lambda x: (-x.score, x.path, x.detail)):
            print(f"  [{item.lang}:{item.kind}] {item.path}: {item.detail}")

    if report_worst <= 0:
        return
    file_scores: dict[str, float] = defaultdict(float)
    file_hits: dict[str, int] = defaultdict(int)
    for f in findings:
        file_hits[f.path] += 1
        weight = {"critical": 1000, "serious": 100, "light": 10}[f.severity]
        file_scores[f.path] = max(file_scores[f.path], weight + f.score)
    ranked = sorted(file_scores.items(), key=lambda kv: -kv[1])[:report_worst]
    print(f"\n=== WORST FILES (top {len(ranked)}) ===")
    for path, score in ranked:
        langs = sorted({f.lang for f in findings if f.path == path})
        lang_tag = ",".join(langs) if langs else "?"
        print(
            f"  [{lang_tag}] {path}  (score~={score:.0f}, findings={file_hits[path]})"
        )


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="unitor-win64-driver C++ + scripts quality gate"
    )
    parser.add_argument("--base", default="origin/main")
    parser.add_argument(
        "--all",
        action="store_true",
        help="Full-tree diagnostic (historical debt; exit 0 even with findings)",
    )
    parser.add_argument(
        "--report-worst",
        type=int,
        default=0,
        metavar="N",
        help="Also print top-N worst files",
    )
    parser.add_argument(
        "--clang-tidy",
        action="store_true",
        help="Also run clang-tidy when available",
    )
    return parser


def resolve_target_files(full_tree: bool, base: str) -> list[str] | None:
    """Return files to analyse, or None if base ref is missing in touched mode."""
    if not full_tree and not git_lines(["git", "rev-parse", "--verify", base]):
        return None
    if full_tree:
        return collect_all_files()
    return collect_changed_files(base)


def analyse_targets(
    files: list[str],
    *,
    full_tree: bool,
    base: str,
) -> list[Finding]:
    line_map = None if full_tree else parse_diff_line_ranges(base)
    findings: list[Finding] = []
    for rel in files:
        changed = None if full_tree else line_map.get(rel, set())
        findings.extend(
            analyse_file(
                rel,
                changed_lines=changed,
                base=None if full_tree else base,
                full_tree=full_tree,
            )
        )
    return findings


def exit_after_report(
    findings: list[Finding],
    *,
    full_tree: bool,
    tidy_rc: int,
) -> int:
    if findings:
        if full_tree:
            print(
                f"\nDiagnostic complete - {len(findings)} finding(s) in the tree "
                "(exit 0). Touched-file mode still fails CI on new drift.",
            )
            return 0
        print(
            f"\nQuality gate FAILED - {len(findings)} finding(s). "
            "Fix signals on changed functions; do not redesign unrelated code.",
            file=sys.stderr,
        )
        return 1
    if tidy_rc != 0:
        print("clang-tidy reported issues.", file=sys.stderr)
        return tidy_rc
    return 0


def main() -> int:
    args = build_arg_parser().parse_args()
    full_tree = args.all
    files = resolve_target_files(full_tree, args.base)
    if files is None:
        print(
            f"Quality gate FAILED - git base ref not found: {args.base}",
            file=sys.stderr,
        )
        return 1

    if not files:
        print(
            "No gate files under src|tests|apps|lib|tools (C++) or scripts/ (Python) "
            "to analyse."
            if full_tree
            else "No touched gate files (C++ or scripts/) - quality gate OK."
        )
        return 0

    mode = "full tree" if full_tree else "touched files (hunk-aware)"
    print(f"Quality gate - {mode} ({len(files)} file(s))")
    if not full_tree:
        for f in files:
            tag = "scripts" if is_py_gate_target(f) else "cpp"
            print(f"  - [{tag}] {f}")

    findings = analyse_targets(files, full_tree=full_tree, base=args.base)
    worst = args.report_worst if args.report_worst > 0 else (20 if full_tree else 0)
    print_report(findings, report_worst=worst)
    tidy_rc = maybe_run_clang_tidy(files) if args.clang_tidy else 0
    return exit_after_report(findings, full_tree=full_tree, tidy_rc=tidy_rc)


if __name__ == "__main__":
    sys.exit(main())
