---
organization: Ten Square Software
project: unitor-win64-driver
title: Authenticode and SmartScreen — lab vs public trust policy
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-21
---

# Authenticode and SmartScreen

This page is the contributor / releaser runbook for **code-signing trust** on Unitor MT4 Bridge. It is **not** the musician-facing install guide — SmartScreen steps for downloaders live in the path-specific user guides ([Win11 WMS](../user/unitor-mt4-bridge-win11-wms-user-guide.md#pass-the-windows-smartscreen-warning) · [Win10 virtualMIDI](../user/unitor-mt4-bridge-win10-virtualmidi-user-guide.md#pass-the-windows-smartscreen-warning); start at [`docs/user/README.md`](../user/README.md)).

**Course correction (2026-08-10):** hobby posture — **no certificate purchase**. See [`sprint-change-proposal-2026-08-10.md`](../../_bmad-output/planning-artifacts/sprint-change-proposal-2026-08-10.md).

## Product policy (FR-15 / NFR-S1 / AD-19)

| Rule | Meaning |
|---|---|
| **OQ-3 — no certificate purchase** | No Authenticode or production INF catalog certificate is planned for this hobby project |
| Not a hard packaging gate | Missing certificate secrets must **not** fail the Public Installer build or merge CI |
| Unsigned public builds | Allowed **when** binaries ship — **only** with user SmartScreen docs (linked above) |
| Clean-PC WinUSB | Setup-alone association without a trusted catalog is **not** promised; use the **guided** path (Zadig or documented equivalent) |
| No kernel / WHQL | Usermode Bridge + WinUSB INF only — never Microsoft Partner Center attestation / kernel signing |

## Two signing domains (do not conflate)

| Domain | What it protects | Typical artifacts |
|---|---|---|
| **A) Authenticode (binaries)** | Publisher trust / SmartScreen reputation for the Setup and Bridge EXEs | `UnitorMt4Bridge-Setup-win11-wms-*.exe`, `UnitorMt4Bridge-Setup-win10-virtualmidi-*.exe`, `Bridge.exe` |
| **B) WinUSB INF catalog** | Driver-package trust for clean-machine association (`CatalogFile=mt4-winusb.cat` in `installer/mt4-winusb.inf`) | `mt4-winusb.cat` next to the INF |

A valid binary signature does **not** automatically produce a production INF catalog, and a lab self-signed `.cat` is **not** public Authenticode. Without a shipped certificate, domain B for community trust is replaced by **guided WinUSB association**, not a paid `.cat`.

## Lab vs public

### Lab only — `installer/sign-lab-package.ps1`

- Builds a **self-signed** `mt4-winusb.cat` and stages it into LocalMachine **Root** / **TrustedPublisher** for contributor machines
- Useful when a clean lab PC rejects an unsigned INF during bind experiments
- **Not** community trust. Never describe this script as public Authenticode. Never ship its self-signed `.cat` as if it were Trusted Root community trust
- Lab `.cat` files under `installer/` are gitignored / not production artifacts

### Public Installer honesty today

- The Public Installer ships `mt4-winusb.inf` **without** a production `.cat` in the payload
- Clean-machine Driver Store association **fails** in lab (`0xE000022F`) — expected without a production catalog
- Community mitigation: **guided WinUSB** (see user guide + [`winusb-bind.md`](winusb-bind.md))
- Lab mitigation remains `installer/sign-lab-package.ps1` (contributors / lab only)

### Optional public binary signing — `scripts/packaging/sign-public-artifacts.ps1`

- Helper remains in-tree **if** a certificate is ever introduced later
- Invoked optionally from `scripts/packaging/build-public-installer.ps1` only when the gate is satisfied
- Unsigned packaging must still succeed
- Distinct from the lab catalog script — do not merge the two workflows
- **Default hobby path:** do not expect this helper to run in CI or Releases

## If a certificate ever appears (optional later path)

Only if a certificate is introduced later — document under **Ten Square Software**:

1. **Subject** — match the public facade (**Ten Square Software**)
2. **Tooling** — Windows SDK **SignTool** (`signtool.exe`)
3. **Timestamping** — always timestamp signed public artifacts
4. **Artifacts** — at minimum both flavored Setups (`UnitorMt4Bridge-Setup-win11-wms-*.exe` and `UnitorMt4Bridge-Setup-win10-virtualmidi-*.exe`); prefer also `Bridge.exe`
5. **Secrets** — never commit PFX files, private keys, or CI secrets

### Example SignTool shape (only if a cert exists)

```powershell
# Illustrative only — wire through scripts/packaging/sign-public-artifacts.ps1
signtool sign /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 `
  /n "Ten Square Software" `
  path\to\UnitorMt4Bridge-Setup-win11-wms-0.1.0.exe
# Also sign UnitorMt4Bridge-Setup-win10-virtualmidi-0.1.0.exe when shipping dual flavors
```

## When shipping unsigned (default hobby path)

1. Ship unsigned public builds **only** with SmartScreen user guidance in the path-specific manuals ([Win11](../user/unitor-mt4-bridge-win11-wms-user-guide.md#pass-the-windows-smartscreen-warning) · [Win10](../user/unitor-mt4-bridge-win10-virtualmidi-user-guide.md#pass-the-windows-smartscreen-warning)) present
2. Do **not** claim a certificate is “coming soon” as the community trust plan
3. Do **not** claim “unsigned = malware” or “signed = never SmartScreen”

Microsoft Defender SmartScreen is **reputation-based**. Even a valid OV/EV signature can warn until download reputation accumulates. Self-signed binaries behave like unsigned for SmartScreen. Reference: [SmartScreen reputation](https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/smartscreen-reputation).

## CI honesty (AD-13)

| Surface | Policy |
|---|---|
| Merge CI (`.github/workflows/windows-build.yml`) | Compile + tests only |
| `release.yml` / Authenticode packaging | Tag-push builds dual Setups and uploads unsigned assets by default — **OQ-3** (no certificate secrets expected); optional `sign-public-artifacts.ps1` only when a cert subject env is set |
| Merge gate | Must **not** fail solely because SignTool secrets are missing |

See also: [`windows-ci-toolchain.md`](windows-ci-toolchain.md) “omitted on purpose” table · operator release runbook [`release-guide.md`](release-guide.md).

## Related

- User SmartScreen: [Win11 EN](../user/unitor-mt4-bridge-win11-wms-user-guide.md#pass-the-windows-smartscreen-warning) · [Win11 FR](../user/unitor-mt4-bridge-win11-wms-guide-utilisateur.md#passer-lavertissement-windows-smartscreen) · [Win10 EN](../user/unitor-mt4-bridge-win10-virtualmidi-user-guide.md#pass-the-windows-smartscreen-warning) · [Win10 FR](../user/unitor-mt4-bridge-win10-virtualmidi-guide-utilisateur.md#passer-lavertissement-windows-smartscreen)
- WinUSB bind: [`winusb-bind.md`](winusb-bind.md)
- License / backends fence: [`license-and-backends.md`](license-and-backends.md)
- Operator smoke: [`docs/tests/smoke-epic4-authenticode-smartscreen-mt4.md`](../tests/smoke-epic4-authenticode-smartscreen-mt4.md)
- Lab catalog helper: [`installer/sign-lab-package.ps1`](../../installer/sign-lab-package.ps1)
- Optional public sign helper: [`scripts/packaging/sign-public-artifacts.ps1`](../../scripts/packaging/sign-public-artifacts.ps1)
