---
organization: Ten Square Software
project: unitor-win64-driver
title: Authenticode and SmartScreen — lab vs public trust policy
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-10
---

# Authenticode and SmartScreen

This page is the contributor / releaser runbook for **code-signing trust** on Unitor MT4 Bridge. It is **not** the musician-facing install guide — SmartScreen steps for downloaders live in the [user guide](../user/unitor-mt4-bridge-user-guide.md#windows-smartscreen-unsigned-or-unrecognized-setup).

## Product policy (FR-15 / NFR-S1 / AD-19)

| Rule | Meaning |
|---|---|
| Authenticode strongly recommended | Prefer signing `Bridge.exe` and `UnitorMt4Bridge-Setup.exe` under the **Ten Square Software** / chosen certificate path before a tagged public community release |
| Not a hard V1 gate | Missing certificate secrets must **not** fail the Public Installer build or merge CI |
| Unsigned public builds | Allowed for V1 **only** when user SmartScreen docs are present (linked above) |
| OQ-3 deferred | Personal vs organization certificate purchase / cost / timing stays **Deferred — Guillaume before first tagged public community release**. This page documents the *policy*; it does **not** decide the choice |
| No kernel / WHQL | Usermode Bridge + WinUSB INF catalog only — never Microsoft Partner Center attestation / kernel signing |

## Two signing domains (do not conflate)

| Domain | What it protects | Typical artifacts |
|---|---|---|
| **A) Authenticode (binaries)** | Publisher trust / SmartScreen reputation for the Setup and Bridge EXEs | `UnitorMt4Bridge-Setup.exe`, `Bridge.exe` |
| **B) WinUSB INF catalog** | Driver-package trust for clean-machine association (`CatalogFile=mt4-winusb.cat` in `installer/mt4-winusb.inf`) | `mt4-winusb.cat` next to the INF |

A valid binary signature does **not** automatically produce a production INF catalog, and a lab self-signed `.cat` is **not** public Authenticode.

## Lab vs public

### Lab only — `installer/sign-lab-package.ps1`

- Builds a **self-signed** `mt4-winusb.cat` and stages it into LocalMachine **Root** / **TrustedPublisher** for contributor machines
- Useful when a clean lab PC rejects an unsigned INF during bind experiments
- **Not** community trust. Never describe this script as public Authenticode. Never ship its self-signed `.cat` as if it were Trusted Root community trust
- Lab `.cat` files under `installer/` are gitignored / not production artifacts

### Public Installer honesty today

- The Public Installer ships `mt4-winusb.inf` **without** a production `.cat` in the payload
- Clean-machine Driver Store association may fail until a real public catalog is produced under the chosen certificate
- Lab mitigation remains `installer/sign-lab-package.ps1` (contributors / lab only)
- When a public catalog signing path exists under the same chosen certificate, document how `.cat` is produced and whether it is packaged into the installer — do **not** promote the lab self-signed catalog

### Optional public binary signing — `scripts/packaging/sign-public-artifacts.ps1`

- Signs Setup / Bridge with a **real** code-signing certificate when environment / cert material is present
- Invoked optionally from `scripts/packaging/build-public-installer.ps1` only when the gate is satisfied
- Unsigned packaging must still succeed (Authenticode is not a hard gate)
- Distinct from the lab catalog script — do not merge the two workflows

## When a certificate exists (chosen path)

Document the release path under **Ten Square Software** (or the org Subject Guillaume selects under OQ-3):

1. **Subject** — match the public facade (**Ten Square Software**); do not invent a different publisher name in user prose
2. **Tooling** — Windows SDK **SignTool** (`signtool.exe`), same Kit discovery pattern as the lab helper
3. **Timestamping** — always timestamp signed public artifacts (RFC 3161 / Microsoft timestamp server as configured for the release)
4. **Artifacts** — at minimum `UnitorMt4Bridge-Setup.exe`; prefer also signing installed `Bridge.exe` before packaging when practical
5. **Secrets** — never commit PFX files, private keys, or CI secrets into this repository

Exact personal-vs-org purchase remains **OQ-3 — Deferred — Guillaume before first tagged public community release**.

### Example SignTool shape (when cert is available)

```powershell
# Illustrative only — wire through scripts/packaging/sign-public-artifacts.ps1
signtool sign /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 `
  /n "Ten Square Software" `
  path\to\UnitorMt4Bridge-Setup.exe
```

Adjust `/n`, `/f`+`/p`, or cloud KMS flags to the chosen certificate store. Prefer the gated helper over ad-hoc flags in musician docs.

## When the certificate lags

1. Ship unsigned public builds **only** with [SmartScreen user guidance](../user/unitor-mt4-bridge-user-guide.md#windows-smartscreen-unsigned-or-unrecognized-setup) present
2. Keep Authenticode **strongly recommended** for the next tagged release once a cert exists
3. Do **not** claim “unsigned = malware” or “signed = never SmartScreen”

Microsoft Defender SmartScreen is **reputation-based**. Even a valid OV/EV signature can warn until download reputation accumulates; EV no longer buys an instant bypass. Self-signed binaries behave like unsigned for SmartScreen. Reference: [SmartScreen reputation](https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/smartscreen-reputation).

## CI honesty (AD-13)

| Surface | Policy |
|---|---|
| Merge CI (`.github/workflows/windows-build.yml`) | Compile + tests only |
| `release.yml` / Authenticode packaging | **Omitted on purpose** unless Guillaume adds a separate release workflow with secrets |
| Merge gate | Must **not** fail solely because SignTool secrets are missing |

See also: [`windows-ci-toolchain.md`](windows-ci-toolchain.md) “omitted on purpose” table.

## Related

- User SmartScreen section: [English user guide](../user/unitor-mt4-bridge-user-guide.md#windows-smartscreen-unsigned-or-unrecognized-setup) · [guide français](../user/unitor-mt4-bridge-guide-utilisateur.md#windows-smartscreen-setup-non-signe-ou-non-reconnu)
- WinUSB bind (contributor): [`winusb-bind.md`](winusb-bind.md)
- License / backends fence: [`license-and-backends.md`](license-and-backends.md)
- Operator smoke: [`docs/tests/smoke-epic4-authenticode-smartscreen-mt4.md`](../tests/smoke-epic4-authenticode-smartscreen-mt4.md)
- Lab catalog helper: [`installer/sign-lab-package.ps1`](../../installer/sign-lab-package.ps1)
- Optional public sign helper: [`scripts/packaging/sign-public-artifacts.ps1`](../../scripts/packaging/sign-public-artifacts.ps1)
