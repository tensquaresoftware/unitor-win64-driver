#!/usr/bin/env python3
"""Prepare and publish Unitor MT4 Bridge dual-flavor GitHub releases.

Inspired by sibling Luthier publish/prepare-release.py — adapted for two Inno
Setup flavors (win11-wms + win10-virtualmidi), not a PyInstaller multi-OS matrix.

Stages under gitignored _local/releases/{version}/:
  pack → finalize → verify → publish-ci (CI) | publish (local tag; Ask First / --yes)

Semver tags without v prefix (X.Y.Z), matching CMake project(VERSION).
"""

from __future__ import annotations

import argparse
import hashlib
import re
import shutil
import subprocess
import sys
import zipfile
from dataclasses import dataclass
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[2]
PACKAGING_DIR = Path(__file__).resolve().parent
TEMPLATE_DIR = PACKAGING_DIR / "templates"
RELEASES_ROOT = PROJECT_ROOT / "_local" / "releases"
GITHUB_REPO = "tensquaresoftware/unitor-win64-driver"
CHECKSUM_FILE = "SHA256SUMS.txt"
NOTES_FILE = "RELEASE_NOTES.md"

FLAVORS = ("win11-wms", "win10-virtualmidi")
VERSION_RE = re.compile(r"^\d+\.\d+\.\d+(?:-[0-9A-Za-z.]+)?$")


@dataclass(frozen=True)
class ReleasePaths:
    version: str
    artifact_version: str
    release_dir: Path

    def setup_name(self, flavor: str) -> str:
        # Setup EXE names always use CMake project(VERSION) (no prerelease suffix).
        return f"UnitorMt4Bridge-Setup-{flavor}-{self.artifact_version}.exe"

    def setup_path(self, flavor: str) -> Path:
        return self.release_dir / self.setup_name(flavor)

    @property
    def docs_archive(self) -> Path:
        return self.release_dir / f"UnitorMt4Bridge-{self.version}-docs.zip"

    @property
    def checksums(self) -> Path:
        return self.release_dir / CHECKSUM_FILE

    @property
    def notes(self) -> Path:
        return self.release_dir / NOTES_FILE

    def distributable_files(self) -> tuple[Path, ...]:
        setups = tuple(self.setup_path(flavor) for flavor in FLAVORS)
        return (*setups, self.docs_archive)


def read_cmake_version(root: Path = PROJECT_ROOT) -> str:
    text = (root / "CMakeLists.txt").read_text(encoding="utf-8")
    match = re.search(
        r"project\s*\(\s*unitor-win64-driver\s+VERSION\s+(\d+\.\d+\.\d+)",
        text,
    )
    if not match:
        raise SystemExit("Could not read project(VERSION) from CMakeLists.txt")
    return match.group(1)


def render_template(name: str, version: str) -> str:
    path = TEMPLATE_DIR / name
    if not path.is_file():
        raise SystemExit(f"Missing template: {path}")
    return path.read_text(encoding="utf-8").replace("{{VERSION}}", version)


def ensure_release_dir(paths: ReleasePaths) -> None:
    paths.release_dir.mkdir(parents=True, exist_ok=True)


def default_installer_dir(root: Path = PROJECT_ROOT) -> Path:
    return root / "builds" / "installer"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def pack_setups(
    paths: ReleasePaths,
    *,
    source_dir: Path,
    force: bool,
) -> None:
    """Copy both Setup EXEs. All-or-nothing: refuse if either flavor is missing."""
    ensure_release_dir(paths)
    pending: list[tuple[str, Path, Path]] = []
    missing: list[str] = []
    for flavor in FLAVORS:
        name = paths.setup_name(flavor)
        source = source_dir / name
        target = paths.setup_path(flavor)
        if not source.is_file():
            missing.append(str(source))
            continue
        if target.is_file() and not force:
            raise SystemExit(f"Already exists: {target}\nUse --force to overwrite.")
        pending.append((name, source, target))

    if missing:
        raise SystemExit(
            "Missing Setup artifact(s) — nothing packed:\n  "
            + "\n  ".join(missing)
            + "\nBuild both flavors first:\n"
            "  .\\scripts\\packaging\\build-public-installer.ps1 -Flavor both"
        )
    if len(pending) != len(FLAVORS):
        raise SystemExit("Internal pack error: expected both flavors before copy")

    copied: list[Path] = []
    try:
        for name, source, target in pending:
            shutil.copy2(source, target)
            copied.append(target)
            size_mb = target.stat().st_size / (1024 * 1024)
            print(f"Packed {name} ({size_mb:.1f} MiB)")
    except Exception:
        for path in copied:
            if path.is_file():
                path.unlink()
        raise


def create_docs_archive(paths: ReleasePaths, *, force: bool) -> None:
    archive = paths.docs_archive
    if archive.is_file() and not force:
        print(f"Docs archive already exists: {archive.name} (use --force to overwrite)")
        return

    manuals: tuple[tuple[str, Path], ...] = (
        ("README.md", PROJECT_ROOT / "docs" / "user" / "README.md"),
        ("unitor-mt4-bridge-win11-wms-user-guide.md", PROJECT_ROOT / "docs" / "user" / "unitor-mt4-bridge-win11-wms-user-guide.md"),
        (
            "unitor-mt4-bridge-win11-wms-guide-utilisateur.md",
            PROJECT_ROOT / "docs" / "user" / "unitor-mt4-bridge-win11-wms-guide-utilisateur.md",
        ),
        (
            "unitor-mt4-bridge-win10-virtualmidi-user-guide.md",
            PROJECT_ROOT / "docs" / "user" / "unitor-mt4-bridge-win10-virtualmidi-user-guide.md",
        ),
        (
            "unitor-mt4-bridge-win10-virtualmidi-guide-utilisateur.md",
            PROJECT_ROOT / "docs" / "user" / "unitor-mt4-bridge-win10-virtualmidi-guide-utilisateur.md",
        ),
        (
            "license-and-backends.md",
            PROJECT_ROOT / "docs" / "dev" / "license-and-backends.md",
        ),
        (
            "authenticode-and-smartscreen.md",
            PROJECT_ROOT / "docs" / "dev" / "authenticode-and-smartscreen.md",
        ),
    )
    for _, path in manuals:
        if not path.is_file():
            raise SystemExit(f"Missing release doc: {path}")

    ensure_release_dir(paths)
    with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for arcname, path in manuals:
            zf.write(path, arcname=arcname)
        readme_txt = render_template("README-docs.template.txt", paths.version)
        zf.writestr("README.txt", readme_txt)
    print(f"Created {archive.name}")


def write_release_notes(paths: ReleasePaths, *, force: bool) -> None:
    if paths.notes.is_file() and not force:
        print(f"{NOTES_FILE} already exists (use --force to overwrite)")
        return
    ensure_release_dir(paths)
    paths.notes.write_text(
        render_template("RELEASE_NOTES.template.md", paths.version),
        encoding="utf-8",
    )
    print(f"Created {paths.notes.name}")


def write_checksums(paths: ReleasePaths) -> None:
    missing = [p.name for p in paths.distributable_files() if not p.is_file()]
    if missing:
        raise SystemExit(f"Cannot write checksums — missing: {', '.join(missing)}")
    lines = [f"{sha256_file(path)}  {path.name}" for path in paths.distributable_files()]
    ensure_release_dir(paths)
    paths.checksums.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Created {paths.checksums.name}")


def _checksum_errors(paths: ReleasePaths) -> list[str]:
    errors: list[str] = []
    checksum_names: set[str] = set()
    for line in paths.checksums.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        parts = line.split(maxsplit=1)
        if len(parts) != 2:
            errors.append(f"Invalid checksum line: {line!r}")
            continue
        expected, name = parts
        checksum_names.add(name)
        file_path = paths.release_dir / name
        if not file_path.is_file():
            errors.append(f"Checksum references missing file: {name}")
        elif sha256_file(file_path) != expected:
            errors.append(f"Checksum mismatch: {name}")
    for path in paths.distributable_files():
        if path.name not in checksum_names:
            errors.append(f"Distributable missing from {CHECKSUM_FILE}: {path.name}")
    return errors


def verify_release(paths: ReleasePaths) -> None:
    errors: list[str] = []
    for path in paths.distributable_files():
        if not path.is_file():
            errors.append(f"Missing: {path.name}")
        elif path.stat().st_size == 0:
            errors.append(f"Empty: {path.name}")

    if not paths.notes.is_file():
        errors.append(f"Missing {NOTES_FILE}")
    if not paths.checksums.is_file():
        errors.append(f"Missing {CHECKSUM_FILE}")
    else:
        errors.extend(_checksum_errors(paths))

    if errors:
        print("Verification FAILED:")
        for err in errors:
            print(f"  * {err}")
        raise SystemExit(1)
    print("Verification OK — both flavors, docs, notes, and checksums match.")


def status(paths: ReleasePaths) -> None:
    ensure_release_dir(paths)
    print(f"Version      : {paths.version}")
    print(f"Release dir  : {paths.release_dir}")
    print()
    rows = [(f"Setup {flavor}", paths.setup_path(flavor)) for flavor in FLAVORS]
    rows.extend(
        [
            ("Docs", paths.docs_archive),
            ("Checksums", paths.checksums),
            ("Notes", paths.notes),
        ]
    )
    for label, path in rows:
        if path.is_file():
            size = path.stat().st_size / (1024 * 1024)
            print(f"  [OK] {label:20} {path.name} ({size:.1f} MiB)")
        else:
            print(f"  [ ] {label:20} {path.name}")


def _run(cmd: list[str], *, cwd: Path | None = None) -> None:
    print("==>", " ".join(str(part) for part in cmd))
    subprocess.run(cmd, cwd=cwd or PROJECT_ROOT, check=True)


def _git_output(*args: str) -> str:
    result = subprocess.run(
        ["git", *args],
        cwd=PROJECT_ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    return result.stdout.strip()


def is_prerelease_version(version: str) -> bool:
    return bool(re.match(r"^\d+\.\d+\.\d+-.+", version))


def _gh_release_exists(tag: str) -> bool:
    result = subprocess.run(
        ["gh", "release", "view", tag, "--repo", GITHUB_REPO],
        cwd=PROJECT_ROOT,
        capture_output=True,
    )
    return result.returncode == 0


def confirm_or_abort(*, yes: bool, prompt: str) -> None:
    print()
    print(prompt)
    print()
    if yes:
        return
    answer = input("Continue? [y/N] ").strip().lower()
    if answer not in {"y", "yes"}:
        raise SystemExit("Aborted (publish requires explicit confirmation or --yes).")


def _remote_tag_exists(tag: str) -> bool:
    remote_tag = subprocess.run(
        ["git", "ls-remote", "--tags", "origin", f"refs/tags/{tag}"],
        cwd=PROJECT_ROOT,
        capture_output=True,
        text=True,
    )
    return bool(remote_tag.stdout.strip())


def gh_release_upload_only(paths: ReleasePaths, *, prerelease: bool) -> None:
    """CI path: upload assets to an existing GitHub Release. Never create tag/release."""
    verify_release(paths)
    if not paths.notes.is_file():
        raise SystemExit(f"Missing {NOTES_FILE}. Run: prepare-release.py finalize")

    tag = paths.version
    if not _remote_tag_exists(tag):
        raise SystemExit(
            f"publish-ci refused: remote tag {tag} is missing. "
            "Push the tag first (or use local publish), then re-run publish-ci."
        )
    if not _gh_release_exists(tag):
        raise SystemExit(
            f"publish-ci refused: GitHub Release for {tag} is missing "
            "(upload-only — will not create a Release). "
            "Create the Release for the existing tag first, or use local publish."
        )

    assets = [*[str(p) for p in paths.distributable_files()], str(paths.checksums)]
    print(f"Release {tag} exists — uploading assets (publish-ci upload-only)")
    _run(
        ["gh", "release", "upload", tag, "--repo", GITHUB_REPO, *assets, "--clobber"],
        cwd=paths.release_dir,
    )
    edit_cmd = ["gh", "release", "edit", tag, "--repo", GITHUB_REPO]
    edit_cmd.append("--prerelease" if prerelease else "--prerelease=false")
    _run(edit_cmd, cwd=paths.release_dir)
    print()
    print(f"Uploaded: https://github.com/{GITHUB_REPO}/releases/tag/{tag}")


def gh_release_create(paths: ReleasePaths, *, prerelease: bool) -> None:
    verify_release(paths)
    if not paths.notes.is_file():
        raise SystemExit(f"Missing {NOTES_FILE}. Run: prepare-release.py finalize")

    tag = paths.version
    assets = [*[str(p) for p in paths.distributable_files()], str(paths.checksums)]

    if _gh_release_exists(tag):
        print(f"Release {tag} already exists — uploading assets")
        _run(
            ["gh", "release", "upload", tag, "--repo", GITHUB_REPO, *assets, "--clobber"],
            cwd=paths.release_dir,
        )
        edit_cmd = ["gh", "release", "edit", tag, "--repo", GITHUB_REPO]
        edit_cmd.append("--prerelease" if prerelease else "--prerelease=false")
        _run(edit_cmd, cwd=paths.release_dir)
    else:
        cmd = [
            "gh",
            "release",
            "create",
            tag,
            "--repo",
            GITHUB_REPO,
            "--title",
            f"Unitor MT4 Bridge {paths.version}",
            "--notes-file",
            str(paths.notes),
            *assets,
        ]
        if prerelease:
            cmd.append("--prerelease")
        _run(cmd, cwd=paths.release_dir)

    print()
    print(f"Published: https://github.com/{GITHUB_REPO}/releases/tag/{tag}")


def publish_ci(paths: ReleasePaths, *, yes: bool, prerelease: bool | None) -> None:
    resolved = prerelease if prerelease is not None else is_prerelease_version(paths.version)
    confirm_or_abort(
        yes=yes,
        prompt=(
            f"Ready to publish-ci Unitor MT4 Bridge {paths.version}\n"
            f"  Upload-only: remote tag AND GitHub Release must already exist.\n"
            f"  Assets: {paths.release_dir}"
        ),
    )
    gh_release_upload_only(paths, prerelease=resolved)


def publish_release(
    paths: ReleasePaths,
    *,
    yes: bool,
    prerelease: bool,
    skip_tag_push: bool,
) -> None:
    verify_release(paths)
    if _git_output("status", "--porcelain"):
        raise SystemExit("Git working tree is not clean. Commit or stash changes first.")

    tag = paths.version
    if subprocess.run(["git", "rev-parse", tag], cwd=PROJECT_ROOT, capture_output=True).returncode == 0:
        raise SystemExit(f"Git tag already exists: {tag}")

    if _remote_tag_exists(tag):
        raise SystemExit(f"Remote tag already exists: {tag}")

    confirm_or_abort(
        yes=yes,
        prompt=(
            f"Ready to publish Unitor MT4 Bridge {paths.version}\n"
            f"  Creates annotated tag {tag}"
            + ("" if skip_tag_push else " and pushes it to origin")
            + f"\n  Then uploads both Setup flavors + docs from {paths.release_dir}"
        ),
    )

    _run(["git", "tag", "-a", tag, "-m", f"Unitor MT4 Bridge {paths.version}"])
    if not skip_tag_push:
        _run(["git", "push", "origin", tag])
    gh_release_create(paths, prerelease=prerelease)


def build_paths(version: str | None) -> ReleasePaths:
    cmake_version = read_cmake_version()
    resolved = version or cmake_version
    if not VERSION_RE.match(resolved):
        raise SystemExit(f"Invalid version {resolved!r} (expected X.Y.Z)")
    tag_base = resolved.split("-", 1)[0]
    if tag_base != cmake_version:
        raise SystemExit(
            f"Tag/version base {tag_base!r} must match CMakeLists.txt VERSION "
            f"({cmake_version})"
        )
    if version and version != cmake_version:
        print(
            f"Note: release folder/tag {version}; Setup artifacts use CMake "
            f"{cmake_version}",
            file=sys.stderr,
        )
    return ReleasePaths(
        version=resolved,
        artifact_version=cmake_version,
        release_dir=RELEASES_ROOT / resolved,
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Prepare Unitor MT4 Bridge release assets under _local/releases/X.Y.Z/ "
            "(dual win11-wms + win10-virtualmidi Setups; semver without v prefix)."
        ),
    )
    parser.add_argument("--version", help="Release version X.Y.Z (default: CMakeLists.txt)")
    parser.add_argument("--force", action="store_true", help="Overwrite existing staged files")

    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("status", help="Show which release assets exist")

    pack = sub.add_parser(
        "pack",
        help="Copy both Setup EXEs from builds/installer into the release folder",
    )
    pack.add_argument(
        "--source-dir",
        type=Path,
        default=None,
        help="Directory containing UnitorMt4Bridge-Setup-*-{version}.exe",
    )

    sub.add_parser(
        "finalize",
        help="Create docs zip, RELEASE_NOTES.md, and SHA256SUMS.txt",
    )
    sub.add_parser("verify", help="Verify staged assets and checksums (no GitHub publish)")

    pub = sub.add_parser(
        "publish",
        help="Create git tag, push, and GitHub release (Ask First — requires --yes to skip prompt)",
    )
    pub.add_argument("-y", "--yes", action="store_true", help="Skip confirmation prompt")
    pub.add_argument("--prerelease", action="store_true", help="Mark as pre-release on GitHub")
    pub.add_argument(
        "--skip-tag-push",
        action="store_true",
        help="Create tag locally but do not push (dry-run style)",
    )

    pub_ci = sub.add_parser(
        "publish-ci",
        help="Upload GitHub release assets (CI: tag already exists); requires --yes in automation",
    )
    pub_ci.add_argument("-y", "--yes", action="store_true", help="Skip confirmation prompt")
    pub_ci.add_argument("--prerelease", action="store_true", help="Force pre-release on GitHub")
    pub_ci.add_argument(
        "--stable",
        action="store_true",
        help="Force stable release (override auto prerelease detection)",
    )

    args = parser.parse_args()
    paths = build_paths(args.version)

    if args.command == "status":
        status(paths)
    elif args.command == "pack":
        source = args.source_dir or default_installer_dir()
        pack_setups(paths, source_dir=source.resolve(), force=args.force)
        status(paths)
    elif args.command == "finalize":
        create_docs_archive(paths, force=args.force)
        write_release_notes(paths, force=args.force)
        write_checksums(paths)
        print()
        status(paths)
    elif args.command == "verify":
        verify_release(paths)
    elif args.command == "publish":
        publish_release(
            paths,
            yes=args.yes,
            prerelease=args.prerelease,
            skip_tag_push=args.skip_tag_push,
        )
    elif args.command == "publish-ci":
        if args.prerelease and args.stable:
            parser.error("Use only one of --prerelease or --stable")
        prerelease_override = None
        if args.prerelease:
            prerelease_override = True
        elif args.stable:
            prerelease_override = False
        publish_ci(paths, yes=args.yes, prerelease=prerelease_override)
    else:
        parser.error(f"Unknown command: {args.command}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
