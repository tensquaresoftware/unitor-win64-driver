# Matrix-Control SysEx Extract — PRD Validation Matrix

**Source repo:** `/Volumes/Guillaume/Dev/Projects/JUCE/Matrix-Control`  
**Purpose:** Concrete SysEx traffic Guillaume uses when validating a MIDI bridge (Unitor MT4 → Matrix-Control).  
**Date:** 2026-08-04  
**Confidence:** High for Matrix-1000 wire formats (encoder/constants + official-style reference doc); medium for Matrix-6/6R member bytes and split-patch paths (provisional / not primary in v1).

---

## 1. Device / protocol family

| Field | Value | Evidence |
|-------|-------|----------|
| Product role | SysEx MIDI editor for **Oberheim Matrix-1000 / Matrix-6 / Matrix-6R** | `PluginDisplayNames.h` tagline |
| Manufacturer ID | `0x10` (Oberheim) | `SysExConstants.h` `kManufacturerIdOberheim` |
| Device ID (SysEx) | `0x06` (Matrix series — shared M-1000 / M-6 / M-6R) | `SysExConstants.h` `kDeviceIdMatrix1000`; reference doc §System Common |
| Device Inquiry family | `0x06 0x00` (Matrix series) | `SysExConstants.h` DeviceInquiry |
| M-1000 member bytes | `0x02 0x00` (memb-lo / memb-hi) | `SysExConstants.h`; `SysExEncoderTests.cpp` golden reply |
| M-6/6R member bytes | `0x01 0x00` (**provisional**, unconfirmed on hardware) | `SysExConstants.h` comment PRD §9 #6 |
| Packed patch payload | **134 bytes** | `PatchModel.h`, `SysExConstants.h` |
| Packed master payload | **172 bytes** | `SysExConstants.h` |
| Nibble encoding | Low nibble first, then high nibble per packed byte | `SysExEncoder.cpp` `unpackBytes`; reference doc §SysEx Data Format |
| Checksum | Sum of packed data bytes, `& 0x7F`, before F7 | `SysExEncoder.cpp` `calculateChecksum` |

**Note:** Matrix-Control speaks **Oberheim Matrix SysEx directly** (manufacturer `10H`, device `06H`). It does not implement Emagic Unitor/MT4 framing; the bridge under test must transparently carry these frames.

---

## 2. Named operations (with file evidence)

### 2.1 Discovery / presence

| Operation | Direction | Shape (paraphrased) | Code |
|-----------|-----------|---------------------|------|
| **Device Inquiry** | Host → all | `F0 7E 7F 06 01 F7` (6 bytes) | `SysExConstants.h` `kRequestMessage`; `SysExEncoder::encodeDeviceInquiry` |
| **Device Inquiry reply** | Synth → host | `F0 7E <chan> 06 02 10 06 00 <memb-lo> <memb-hi> <rev×4 ASCII> F7` (15 bytes) | `SysExEncoder.cpp` `encodeDeviceInquiryReply`; `SysExEncoderTests.cpp` |
| Presence heartbeat | Host → all | Same inquiry, periodic when detected | `MidiManagerDeviceInquiry.cpp`; `MidiRequestTiming.h` |

### 2.2 Patch dump (read from synth)

| Operation | Request | Expected response | Code |
|-----------|---------|-------------------|------|
| **Request single patch** | `F0 10 06 04 01 <patch 0–99> F7` (7 bytes) | `F0 10 06 01 <patch> <268 nibbles> <checksum> F7` (**275 bytes**) | `MidiManager::requestSinglePatch`, `requestSinglePatchAsync`; `SysExEncoder::encodeRequestMessage` |
| **Request edit buffer** | `F0 10 06 04 04 00 F7` | Same 275-byte **0x01-style patch** frame (decoded as patch) | `MidiManager::requestCurrentPatch` |
| **Device nav load** | Set Bank (if M-1000) + Program Change, then single-patch request | Async one-shot capture; ignores non-patch SysEx until timeout | `PatchManagerActionHandlerDeviceLoad.cpp`; `PatchSelectionMidiSync.cpp`; `MidiManagerAsyncPatch.cpp` |

Primary user-facing name in UI/docs: **device dump**, **load current patch from device**, **bank export live-dump**.

### 2.3 Master dump (read global block)

| Operation | Request | Expected response | Code |
|-----------|---------|-------------------|------|
| **Request master parameters** | `F0 10 06 04 03 00 F7` | `F0 10 06 03 03 <344 nibbles> <checksum> F7` (**351 bytes**) | `MidiManager::requestMasterData`; reference doc §03H / §04H |

Matrix-Control v1 **blocks outbound master edits on Matrix-6/6R** (`sendMaster` gated); dump request path exists but master editing is M-1000-focused.

### 2.4 Patch / master push (write to synth)

| Operation | Outbound frame | Size | Code |
|-----------|----------------|------|------|
| **Send patch to slot** (STORE, import write, M-6 audition) | `F0 10 06 01 <patch> <nibbles> <checksum> F7` | 275 | `MidiManager::sendPatch`; `PatchManagerActionHandlerBankImportWrite.cpp` |
| **Send patch to edit buffer** (M-1000 live edit / Mutator audition) | `F0 10 06 0D 00 <nibbles> <checksum> F7` | 275 | `MidiManager::sendPatchToEditBuffer`; `sendFullPatchForAudition` |
| **Send full master** | `F0 10 06 03 03 <nibbles> <checksum> F7` | 351 | `MidiManager::sendMaster(0x03, …)`; `MasterParameterSysExDispatcher` |
| **Remote parameter edit** (live knob) | `F0 10 06 06 <param 0–99> <value> F7` | 7 | `PatchParameterSysExDispatcher` → `enqueueRemoteParameterEdit` |
| **Matrix Mod bus edit** | `F0 10 06 0B <bus> <source> <amount> <dest> F7` | 9 | `MatrixModBusParameterSysExDispatcher`; coalesced ~25 ms | `PluginProcessor.h` `MatrixModSysExCoalesceTimer` |
| **Set bank** | `F0 10 06 0A <bank 0–9> F7` | 6 | `PatchSelectionMidiSync::syncSelection` |
| **Unlock bank** | `F0 10 06 0C F7` | 5 | `MidiManager::sendUnlockBank` |
| **Store edit buffer** | `F0 10 06 0E <patch> <bank> <unitId> F7` | 8 | `MidiManager::sendStoreEditBuffer` |

### 2.5 Bank librarian (import / export / restore)

| Operation | SysEx pattern | Code / spec |
|-----------|---------------|-------------|
| **Bank EXPORT** | For slots 0…99: `requestSinglePatchAsync(slot)` → save 275-byte `.syx` per slot; M-1000 sends **Set Bank** first | `PatchManagerActionHandlerBankExport.cpp`; `spec-bank-utility-import-export.md` |
| **Bank IMPORT write** | For each valid file: `sendPatch(slot, packed)` (0x01) | `PatchManagerActionHandlerBankImportWrite.cpp` |
| **Import cancel RESTORE** | Re-send **snapshot** patches captured before import (same 0x01 writes) | `beginBankImportRestore`; `restoreNextSnapshotSlot`; `spec-bank-utility-import-export.md` AC |
| **Request all bank** (`04H` type 0) | Would stream 100× patch + 50 dummy split frames — **not implemented in Matrix-Control v1** | `SysExConstants.h` `kRequestAllBank` comment; export uses sequential single-patch requests instead |

There is **no dedicated “restore” SysEx opcode**; restore = replay stored 0x01 patch dumps.

### 2.6 Non-SysEx editor traffic (often paired with dumps)

| Message | Use | Code |
|---------|-----|------|
| **Program Change** | Patch selection after bank change | `PatchSelectionMidiSync::syncSelection` |
| **Control Change 120/121/123** | PANIC (all channels or selected) | `MidiManager::sendPanic` |

---

## 3. Typical message shapes (command bytes)

All Oberheim Matrix editor SysEx (except Universal Inquiry) share:

```
F0  10  06  <opcode>  [header-data…]  [payload…]  [checksum if bulk]  F7
     │   │      │
     │   │      └── opcode (see table)
     │   └── device ID 06H
     └── Oberheim manufacturer 10H
```

### Opcode table (from `SysExConstants.h` + reference doc)

| Opcode | Name | Header after opcode | Bulk payload |
|--------|------|---------------------|--------------|
| `01H` | Single patch data | `<patch 0–99>` | 268 nibbles + checksum |
| `02H` | Split patch (M-6) | `<number>` | 36 bytes packed (+ checksum in full M-6 spec) |
| `03H` | Master parameter data | `<version>` — **03H** on M-1000 | 344 nibbles + checksum |
| `04H` | Request data | `<type>` then `<number>` | none (request only) |
| `06H` | Remote parameter edit | `<param> <value>` | none |
| `0AH` | Set bank | `<bank>` | none |
| `0BH` | Matrix Mod remote edit | `<bus> <src> <amt> <dest>` | none |
| `0CH` | Unlock bank | (none) | none |
| `0DH` | Patch to edit buffer | **`00` literal byte required** | 268 nibbles + checksum |
| `0EH` | Store edit buffer | `<patch> <bank> <unitId>` | none |

### Request types (`04H` second byte)

| Type | Value | `<number>` | Response |
|------|-------|------------|----------|
| All bank + master | `00` | `00` | 100× `01H` + 50 dummy `02H` + master (not used in Matrix-Control v1) |
| Single patch | `01` | patch 0–99 | one `01H` 275-byte message |
| Master | `03` | `00` | one `03H` 351-byte message |
| Edit buffer | `04` | `00` | patch-format 275-byte message |

---

## 4. Sizes and burstiness

### Wire sizes (total bytes including F0/F7)

| Frame | Bytes | Notes |
|-------|-------|-------|
| Device Inquiry request | 6 | Fixed |
| Device Inquiry reply | 15 | Fixed |
| Patch dump / send (01H, 0DH) | **275** | 134 packed → 268 nibbles + 5 header + checksum + F7 |
| Master dump / send (03H) | **351** | 172 packed → 344 nibbles + 5 header + checksum + F7 |
| Split patch (02H) | **43** | `36 + 5 + 1 + 1` in constants; M-6 primary path |
| Remote edit (06H) | 7 | High frequency during UI edits |
| Matrix Mod (0BH) | 9 | Coalesced; timer **25 ms** (> 10 ms gate) |
| Set bank (0AH) | 6 | Before dump loops |
| Store (0EH) | 8 | After edit |
| Single-patch request (04H/01H) | 7 | Per dump |
| Default RPC timeout | **2000 ms** | `kDefaultTimeoutMs` |

### Inter-message pacing

| Profile | Delay | Source |
|---------|-------|--------|
| M-1000 stock EPROM | **10 ms** min between SysEx | `SysExDelayProfile::kStockDelayMsMatrix1000`; manual note in reference doc |
| M-6/6R stock | **20 ms** | `SysExDelayProfile::kStockDelayMsMatrix6` |
| Optimised EPROM (TAUNTEK / GLIGLI / NORDCORE substring in inquiry version) | 5 ms (M-1000) / 10 ms (M-6) | `SysExDelayProfile.h` |
| Post Set Bank / PC settle before dump | `max(50 ms, 5 × profileDelay)` | `MidiRequestTiming::deviceSettleMs` |
| Outbound queue idle wait (before dump) | `max(500 ms, 50 × profileDelay)` | `MidiRequestTiming::outboundIdleTimeoutMs` |

### Burst scenarios Guillaume actually runs

1. **Navigate Internal patch:** Set Bank (optional) + PC + **one** patch request/response (~7 + 275 bytes + 10 ms pacing).
2. **Live edit session:** Many **7-byte** 0x06 messages and **9-byte** 0x0B messages; full **275-byte** 0x0D on Mutator audition / INIT (M-1000).
3. **Bank EXPORT:** **100** sequential `(request 7 B → response 275 B)` pairs, **≥10 ms** apart → ~**28.2 KB** inbound patch payload + ~700 B requests per bank (plus one Set Bank at start).
4. **Bank IMPORT cancel restore:** Up to **100** outbound **275-byte** 0x01 messages with inter-SysEx delay.
5. **Master INIT (M-1000):** Single **351-byte** 0x03 burst after confirm dialog.

---

## 5. Recommended minimum “pass” vectors (unitor-win64-driver V1 PRD)

Guillaume can confirm these as **must-pass** bridge scenarios when Matrix-Control is the host app and MT4/Unitor is in the path:

1. **Device Inquiry round-trip** — Host sends `F0 7E 7F 06 01 F7`; synth (or simulator) returns 15-byte reply with manufacturer `10`, family `06 00`, member `02 00` (M-1000). Matrix-Control gates all editor traffic on this.

2. **Single patch dump (dominant validation path)** — After Set Bank + PC (typical nav): host sends `F0 10 06 04 01 <n> F7`; bridge must deliver **exactly 275-byte** `F0 10 06 01 … F7` response without truncation/merge errors; 2 s timeout budget.

3. **Edit-buffer audition (M-1000)** — Host sends **275-byte** `F0 10 06 0D 00 … F7` (literal `00` after opcode — regressed bug if missing); synth must accept and sound. Smaller than dump but same size class.

4. **Live editor SysEx stream** — Sustained **7-byte** 0x06 and **9-byte** 0x0B messages with **≥10 ms** spacing (stock M-1000); bridge must not reorder or drop below coalescer expectations.

5. **Bank export stress (optional but high value)** — **100** sequential patch dumps with inter-message delay; validates bulk reliability (~28 KB inbound per bank). Import/restore adds symmetric **100 × 275 B** outbound.

6. **Parasitic frame filtering** — During dump wait, non-patch SysEx (inquiry, master 351 B) may appear; host ignores them and keeps listening (`MidiManagerAsyncPatch.cpp`). Bridge must not strip valid patch frames mixed in short windows.

*(Master 351 B dump/send and M-6-specific 0x01-only audition are **second-tier** unless V1 scope includes MASTER panel or Matrix-6 hardware.)*

---

## 6. What was NOT found (or not used in Matrix-Control v1)

| Gap | Detail |
|-----|--------|
| **Emagic Unitor / MT4 protocol** | No references in Matrix-Control; validation assumes transparent MIDI port forwarding of Oberheim SysEx. |
| **Request-all-bank (`04H` type 0)** | Documented in reference doc (100 patches + 50 dummy splits, 10 ms apart) but **explicitly not implemented**; export loops single-patch requests. |
| **Dedicated “restore” opcode** | Restore = re-send snapshot **0x01** patches, not a special SysEx command. |
| **Split patch (02H) in production editor** | Constants + parser exist; primary M-1000 flows use **01H/0DH** only. M-6 split path marked compatibility. |
| **Matrix-6 member byte hardware proof** | `0x01 0x00` marked provisional in `SysExConstants.h`. |
| **Matrix-Simulator full patch dump** | Deferred-work notes: simulator does **not** emulate full patch dumps; goldens live in unit tests / hardware. |
| **Exact Unitor bulk transfer limits** | Matrix-Control does not document MT4/Emagic packetization; only Oberheim-side framing. |
| **Set Group Mode (07H)** | Opcode defined, not traced in primary validation flows grep. |

---

## Primary source files (absolute paths)

| Topic | Path |
|-------|------|
| Constants / sizes / opcodes | `/Volumes/Guillaume/Dev/Projects/JUCE/Matrix-Control/Source/Core/MIDI/SysEx/SysExConstants.h` |
| Message builders | `/Volumes/Guillaume/Dev/Projects/JUCE/Matrix-Control/Source/Core/MIDI/SysEx/SysExEncoder.cpp` |
| Parser / validation | `/Volumes/Guillaume/Dev/Projects/JUCE/Matrix-Control/Source/Core/MIDI/SysEx/SysExParser.cpp` |
| MIDI orchestration | `/Volumes/Guillaume/Dev/Projects/JUCE/Matrix-Control/Source/Core/MIDI/MidiManager.cpp` |
| Async patch dump | `/Volumes/Guillaume/Dev/Projects/JUCE/Matrix-Control/Source/Core/MIDI/MidiManagerAsyncPatch.cpp` |
| Device load sequence | `/Volumes/Guillaume/Dev/Projects/JUCE/Matrix-Control/Source/Core/Actions/PatchManagerActionHandlerDeviceLoad.cpp` |
| Bank export loop | `/Volumes/Guillaume/Dev/Projects/JUCE/Matrix-Control/Source/Core/Actions/PatchManagerActionHandlerBankExport.cpp` |
| Bank import / restore | `/Volumes/Guillaume/Dev/Projects/JUCE/Matrix-Control/Source/Core/Actions/PatchManagerActionHandlerBankImportWrite.cpp` |
| Inter-SysEx delay | `/Volumes/Guillaume/Dev/Projects/JUCE/Matrix-Control/Source/Core/MIDI/Queue/SysExDelayProfile.h` |
| Golden encoder tests | `/Volumes/Guillaume/Dev/Projects/JUCE/Matrix-Control/Tests/Unit/SysExEncoderTests.cpp` |
| M-1000 reference | `/Volumes/Guillaume/Dev/Projects/JUCE/Matrix-Control/_bmad-output/reference-docs/oberheim/oberheim-matrix-1000-midi-sysex-implementation.md` |
| Bank utility spec | `/Volumes/Guillaume/Dev/Projects/JUCE/Matrix-Control/_bmad-output/implementation-artifacts/spec-bank-utility-import-export.md` |
