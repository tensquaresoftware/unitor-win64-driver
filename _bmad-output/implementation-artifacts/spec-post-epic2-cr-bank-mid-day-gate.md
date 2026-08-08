---
title: 'Post-Epic2 CR bank+mid day gate (lab evidence)'
type: 'chore'
created: '2026-08-08'
status: 'done'
route: 'one-shot'
review_loop_iteration: 0
context: []
---

# Post-Epic2 CR bank+mid day gate (lab evidence)

## Intent

**Problem:** After Matrix short-dump race fixes (`835c992`), the overnight ~84% bank / mid hole needed a day gate (100% bank 20×100 + mid) before overnight or long SysEx work.

**Approach:** Rebuild debug Bridge, run Lab A then Lab B on MT4 In1/Out1 with Matrix correctly wired (MIDI In, not Thru). No product code changes; report Pass/Fail and residual TIMEOUT fingerprint.

## Suggested Review Order

**Cold-start first dump**

- First librarian dump after Bridge start still times out with no frame (`last=none`).
  [`DeviceSessionHostOutbound.cpp:240`](../../../src/Device/DeviceSessionHostOutbound.cpp#L240)

- Expect window / size-reject retry path still relevant for mid-burst misses.
  [`DeviceSessionMatrixDump.cpp:1`](../../../src/Device/DeviceSessionMatrixDump.cpp#L1)

**Lab evidence**

- Bank gate: 10/20 starts at 100%; 13× `TIMEOUT last=none` (many index 0001).
  [`sysex-matrix-bank-20260808T160736Z.log`](../../../tests/lab-logs/sysex-matrix-bank/post-epic2-cr/sysex-matrix-bank-20260808T160736Z.log)

- Mid gate: every start loses only the first `dump_patch`, then master/push clean.
  [`sysex-matrix-mid-20260808T161402Z.log`](../../../tests/lab-logs/sysex-matrix-mid/post-epic2-cr/sysex-matrix-mid-20260808T161402Z.log)
