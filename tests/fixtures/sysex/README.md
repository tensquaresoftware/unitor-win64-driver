# SysEx fixtures

Reusable `.syx` dumps for Epic 2 smoke and Matrix-Control-scale sizes.

| File | Bytes | Notes |
|---|---|---|
| `DeviceEnquiry.syx` | 6 | `F0 7E 7F 06 01 F7` |
| `Patch.syx` | 275 | Same bytes as Matrix-Control `tests/fixtures/Init/PatchInit.syx` |
| `Master.syx` | 351 | Same bytes as Matrix-Control `tests/fixtures/Init/MasterInit.syx` |

Optional lab reply dump (re-add if needed): `DeviceEnquiryReply.syx` (~15 B).
