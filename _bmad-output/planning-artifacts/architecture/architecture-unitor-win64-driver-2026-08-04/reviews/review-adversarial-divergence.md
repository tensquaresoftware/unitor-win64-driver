# Adversarial Divergence Review — Architecture Spine

**Target:** `ARCHITECTURE-SPINE.md` (unitor-win64-driver V1, draft 2026-08-04)  
**Method:** For each finding, two epic/story teams each obey every cited AD literally yet produce incompatible runtime or integration behavior.  
**Reviewer stance:** Cynical — assume builders will optimize for their slice and cite the AD sentence that backs their choice.

---

## Verdict

**FAIL (divergence-prone draft).** The spine’s invariants are directionally sound but leave **shared-entity ownership**, **data-shape contracts**, and **lifecycle edge semantics** implicit. Six concrete incompatible pairs were found where both sides cite valid AD text. Five require new or tightened ADs before epic/story split; one is partially covered by AQ-1 but still needs a hard rule.

---

## Finding 1 — Unit ordinal `K`: two owners, two assignment algorithms

**Governed by (both teams cite):** AD-5, AD-6; capability map row “Port names | `Midi/` + session”

### Team A — Epic **Device / Multi-MT4 sessions** (`DeviceSessionManager`)

- Owns `UnitIdentityRegistry` persisted under `%ProgramData%/TenSquare/Bridge/unit-identity.json`.
- On first sight of a unit: assign smallest free `K`; key = USB topology path (`bus/port` chain) when serial absent.
- Passes `unitOrdinal` into session construction; **does not** expose registry to `Midi/`.

**AD compliance:** AD-6 (“durable local map keyed by USB topology path … persisted by the Bridge”; “Ordinal K must not reshuffle”).

### Team B — Epic **Midi / VirtualMIDI backend** (`VirtualMidiBackend`)

- Owns display-name formatting (AD-5 strings).
- At port creation, reads `iSerialNumber` from WinUSB device descriptor; derives stable `K` from **sorted serial lexicographic order** among currently connected MT4s (serial `A` → K=1, serial `B` → K=2).
- Ignores any ordinal passed from session layer — “naming lives in `Midi/` per capability map.”

**AD compliance:** AD-6 (“Prefer USB serial string when present”); AD-5 (exact display strings from `K` and `N`).

### Incompatible outcome

Two MT4s connected: Team A assigns K by plug-order + topology map; Team B assigns K by serial sort. DAW recall files reference `MT4 #2 Port 3` for unit A’s K=2 while Team B labeled that physical unit `MT4 Port 3` (K=1). **Both obey AD-5 string grammar and AD-6 stability wording; names diverge.**

### Hole to close

**New AD (recommended AD-17):** Single owner for `K` assignment — `DeviceSessionManager` / `UnitIdentityRegistry` only. `VirtualMidiBackend` receives `(unitOrdinal, portIndex, direction)` and **must not** re-derive K from USB descriptors. Persisted identity record schema is normative (one JSON file, one key precedence: serial → topology path).

---

## Finding 2 — `DeviceProfile` cable masks: same field names, incompatible shapes

**Governed by (both teams cite):** AD-3, AD-14; consistency row “DeviceProfiles as declarative data”

### Team A — Epic **Profile / DeviceProfile table**

- Defines `DeviceProfile` as:

```cpp
struct DeviceProfile {
  uint16_t vid, pid;
  uint8_t ifnum;
  uint16_t in_cables;   // bitmask, bit i => Emagic cable i
  uint16_t out_cables;
  uint32_t capability_flags;
};
```

- MT4 row: `in_cables=0x8003`, `out_cables=0x800f` (Linux quirk-table literals).

**AD compliance:** AD-3 (“declares at least: vid, pid, in_cables mask, out_cables mask, ifnum, capability flags”).

### Team B — Epic **Protocol / Emagic cable mapper**

- Expects profile adapter:

```cpp
struct CableMap {
  std::array<uint8_t, 4> outLogicalToEmagic;  // Virtual Port N → cable id
  std::array<uint8_t, 2> inLogicalToEmagic;
};
```

- Builds map from **1-based cable indices** documented in `docs/dev/` after USB capture — treats `0x8003` as opaque until interpreted; their table maps Port 1 → cable 0x01, not bit 0.

**AD compliance:** AD-14 (reimplement under MIT; reference Linux quirks); AD-3 (reads profile, no hard-coded MT4 masks in mapper **logic** — table lives in Profile).

### Incompatible outcome

Mapper compiles against Profile headers but **misroutes cable 3 ↔ Port 4** because bit position ≠ Emagic cable ID. Single-MT4 “works on port 1” in isolation tests; dual-unit + SysEx stress fails. Both teams passed AD-3 field checklist.

### Hole to close

**Tighten AD-3:** Normative mask semantics — “bit *n* (0-based) corresponds to Emagic cable *n* as in `QUIRK_MIDI_EMAGIC` / addendum cable table; MT4 validated row must round-trip through `EmagicCableMapper` unit tests.” Optionally: shared `DeviceProfile.h` owns **both** raw masks and precomputed `CableMap` to eliminate dual interpretation.

---

## Finding 3 — Virtual Port lifecycle on unplug: destroy vs mark-unavailable

**Governed by (both teams cite):** AD-9, AD-10, FR-11 (via binds)

### Team A — Epic **Device / session lifecycle**

- On USB disconnect: **immediately** tear down `DeviceSession`, call VirtualMIDI SDK destroy for all six endpoints.
- Rationale: AD-9 first clause — “destroying/tearing down the session destroys those ports.”

**AD compliance:** AD-9 (no orphan ports after unplug).

### Team B — Epic **App / hot-plug & Auto-Start**

- On USB disconnect: transition ports to **Unavailable** state, keep SDK handles for **60 s recovery window**; if same unit identity replugs within window, **reactivate same endpoints** without DAW rescan.
- Rationale: AD-9 parenthetical — “or marks them unavailable per documented hot-plug behavior”; AD-10 allows documented recovery paths.

**AD compliance:** AD-9 (alternate branch); AD-10 (replug happy path without reboot).

### Incompatible outcome

Team A: DAW sees ports vanish instantly — user must re-select inputs. Team B: DAW sees grayed/stale ports that resume — Matrix-Control session stays bound to stale handles. **Both satisfy “no long-lived ports for disconnected units beyond the documented recovery window” with different window definitions (0 s vs 60 s).**

### Hole to close

**Tighten AD-9 (+ AD-10 cross-ref):** Exactly one hot-unplug policy — pick **destroy** or **mark-unavailable + max window**; if latter, state max seconds, identity match rules, and mandatory DAW behavior doc. AQ-2 must resolve to a single normative paragraph, not “Architecture revisit.”

---

## Finding 4 — Port creation authority: `DeviceSession` vs `VirtualMidiBackend` init

**Governed by (both teams cite):** AD-2 (dependency diagram), AD-7, AD-9

### Team A — Epic **Device / DeviceSession**

- `DeviceSession::Start()` sequence: WinUSB open → mapper init → `midiBackend->CreatePortSet(sessionId, names)`.
- Ports exist only while `DeviceSession` alive object holds `shared_ptr`.

**AD compliance:** AD-9 (“Creating a DeviceSession creates that unit’s Virtual Port set”); AD-2 (`Sess --> Midi`).

### Team B — Epic **Midi / VirtualMidiBackend**

- `VirtualMidiBackend::OnDeviceArrival(unitId)` registered from `App` hot-plug watcher **before** session fully constructed; creates ports when WinUSB interface first detected.
- `DeviceSession` assumes ports already exist and only binds read/write callbacks.

**AD compliance:** AD-7 (“Bridge creates and destroys Virtual Ports via the VirtualMIDI SDK”); AD-2 (`Sess --> Vm` direct edge).

### Incompatible outcome

Double port creation (name collision in VirtualMIDI driver) **or** session starts with null handles because ports were created under a provisional `unitId` that `DeviceSessionManager` later replaces. Hot-plug race: Team B creates ports for phantom arrival; Team A never attaches mapper. Both graphs match AD-2 mermaid.

### Hole to close

**Tighten AD-7 + AD-9:** Normative call graph — only `DeviceSession` (or exactly one method it delegates to) may call VirtualMIDI create/destroy; hot-plug notifications **must not** create ports outside session factory. Sequence diagram: `Arrival → SessionManager → DeviceSession::Start → MidiBackend::CreatePortSet`.

---

## Finding 5 — WinUSB open path: GUID vs VID/PID enumeration

**Governed by (both teams cite):** AD-12 (installer), AD-1/structural `Usb/`

### Team A — Epic **Installer / WinUSB association**

- INF registers `DeviceInterfaceGUID {aa209017-cf8a-49ad-a0e7-701187ff7e05}` on MT4 interface 2.
- Documents: “Bridge must open via registered GUID.”

**AD compliance:** AD-12 (primary community path, exact GUID).

### Team B — Epic **Usb / WinUsbTransport**

- Enumerates `SetupDiGetClassDevs` with `USB\VID_086A&PID_0003`, opens first matching WinUSB interface via `WinUsb_Initialize` on path from device interface detail — **no GUID filter** (Zadig-style dev fallback generalized).

**AD compliance:** AD-12 (“Developer fallback: Zadig — documented for contributors only” — Team B treats GUID as installer-only hint); AD-1 usermode WinUSB transport.

### Incompatible outcome

Clean INF install: Team B opens wrong composite interface or first MT4 when two present → silent bulk pipe mismatch. Zadig dev machine: Team A’s GUID never registered → Team A’s Bridge build fails open. **Both “use WinUSB”; production vs dev matrices diverge without CI catching it.**

### Hole to close

**Tighten AD-12 + new Usb AD:** Bridge **must** open devices via the registered `DeviceInterfaceGUID` on the profile’s `ifnum`; VID/PID walk is **contributor-only** behind explicit compile flag or runtime `--dev-zadig` switch. CI Windows build tests GUID path with mocked enumeration fixture.

---

## Finding 6 — 2 IN + 4 OUT topology vs AD-5 `Port N` numbering (1..4 only)

**Governed by (both teams cite):** AD-5, FR-4 (via binds)

### Team A — Epic **Midi / port factory**

- Creates **6** SDK endpoints: 4 OUT named `MT4 Port 1..4`, 2 IN named `MT4 Port 1..2` (same N range as OUT for “cable N” on IN side per AD-5 bullet).

**AD compliance:** AD-5 (“N in 1..4”; “same label on both sides for cable N”); FR-4 2 IN + 4 OUT.

### Team B — Epic **Protocol / cable routing**

- Maps Emagic **IN cables 1 and 2** to Virtual **IN endpoints 3 and 4** (matching hardware silkscreen “Port 3/4 IN” on some MT4 docs); OUT remains 1..4.
- Names IN endpoints `MT4 Port 3` and `MT4 Port 4` only (OUT keeps 1..4).

**AD compliance:** AD-5 (macOS-like naming — cites legacy macOS driver port numbering); AD-14 (protocol fidelity over UI assumption).

### Incompatible outcome

User wires DAW to `MT4 Port 1` IN (Team A) while Team B’s mapper feeds cable 1 from `MT4 Port 3` IN. OUT works; IN silent. Validation Matrix “2 IN / 4 OUT round-trip per port” fails on **different** ports depending on team. AD-5 never states IN logical index ↔ cable id ↔ display `N`.

### Hole to close

**Tighten AD-5 (+ AD-3 cable table):** Normative mapping table — for MT4 validated profile, list each of 6 endpoints: `{direction, displayName, emagicCableId, mask bit}`. One row per endpoint; no team-local reinterpretation.

---

## Secondary pairs (shorter)

| # | Teams | Clash | AD gap |
| --- | --- | --- | --- |
| 7 | **Device** (topology key = hub port chain) vs **App** (topology key = full instance path string) | Same unit new K after hub change | AD-6 lacks canonical path serialization |
| 8 | **tools/harness** (timestamps at `MidiBackend` API) vs **Usb** (timestamps at bulk URB completion) | Incomparable p99 tables, both “MIDI Path” | AD-11 lacks injection/observation plane |
| 9 | **Device** (`DeviceSessionManager` owns hot-plug) vs **App** (Windows service registers `WM_DEVICECHANGE` and mutates sessions) | Double session create / missed teardown | AD-10 lacks single hot-plug owner |
| 10 | **Profile** (capability flags enum in header) vs **Protocol** (ignores flags, always parses Patch/LTC fields) | Future stub row enables dead code paths | AD-3 “off for V1 MT4” doesn’t forbid read of flags |

---

## Recommended AD patch set (summary)

| Priority | Action | Closes |
| --- | --- | --- |
| P0 | **AD-17** — Single owner + schema for unit identity and ordinal `K` | Finding 1 |
| P0 | **AD-3 amend** — Bit semantics + MT4 round-trip test obligation | Finding 2 |
| P0 | **AD-5 amend** — Six-row endpoint table (name, direction, cable id) | Finding 6 |
| P1 | **AD-9 amend** — One hot-unplug policy; bind AQ-2 to decision | Finding 3 |
| P1 | **AD-7/9 amend** — Port create/destroy only from `DeviceSession` | Finding 4 |
| P1 | **AD-12 + Usb rule** — GUID-first open; Zadig dev flag only | Finding 5 |
| P2 | **AD-6 amend** — Canonical topology path format | Secondary 7 |
| P2 | **AD-11 amend** — Harness timestamp planes | Secondary 8 |
| P2 | **AD-10 amend** — Hot-plug notification owner | Secondary 9 |

---

## Self-check

- **≥10 issues:** 6 primary + 4 secondary = 10.  
- **Each primary pair:** two teams, literal AD obedience, concrete incompatible behavior.  
- **Actionable:** each maps to new or tightened AD text.

---

*Review generated: 2026-08-04 — adversarial divergence pass on Architecture Spine draft.*
