#!/usr/bin/env python3
"""
Resolve a free BMad Agents sidebar title (duplicate → ' (n)' suffix).

Usage:
  python scripts/dev/resolve-bmad-chat-title.py "BMad — Quick Dev 8-4"

Reads Cursor's state DB read-only (macOS + Windows). Prints one line: the title
to pass to MCP rename_chat. Does NOT mutate the database — only rename_chat
persists titles durably.
"""

from __future__ import annotations

import json
import re
import sqlite3
import sys
from pathlib import Path


def cursor_state_db_candidates() -> list[Path]:
    home = Path.home()
    return [
        home / "Library/Application Support/Cursor/User/globalStorage/state.vscdb",
        home / "AppData/Roaming/Cursor/User/globalStorage/state.vscdb",
    ]


def load_composer_names(db_path: Path) -> list[str]:
    uri = f"file:{db_path.as_posix()}?mode=ro"
    conn = sqlite3.connect(uri, uri=True)
    try:
        row = conn.execute(
            "SELECT value FROM ItemTable WHERE key='composer.composerHeaders'"
        ).fetchone()
    finally:
        conn.close()
    if not row or not row[0]:
        return []
    payload = json.loads(row[0])
    return [c.get("name") or "" for c in payload.get("allComposers", [])]


def resolve_title(base: str, names: list[str]) -> str:
    pat = re.compile(rf"^{re.escape(base)}(?: \((\d+)\))?$")
    taken: set[int] = set()
    for name in names:
        match = pat.match(name)
        if not match:
            continue
        taken.add(int(match.group(1)) if match.group(1) else 0)
    if not taken:
        return base
    n = 1
    while n in taken:
        n += 1
    return f"{base} ({n})"


def main() -> int:
    if len(sys.argv) != 2 or not sys.argv[1].strip():
        print(
            "Usage: python scripts/dev/resolve-bmad-chat-title.py "
            '"BMad — Create Story 4-5"',
            file=sys.stderr,
        )
        return 2

    base = sys.argv[1].strip()
    names: list[str] = []
    for candidate in cursor_state_db_candidates():
        if not candidate.is_file():
            continue
        try:
            names = load_composer_names(candidate)
            break
        except (OSError, sqlite3.Error, json.JSONDecodeError) as exc:
            print(f"Warning: could not read {candidate}: {exc}", file=sys.stderr)

    print(resolve_title(base, names))
    return 0


if __name__ == "__main__":
    sys.exit(main())
