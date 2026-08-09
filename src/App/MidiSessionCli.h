// Long-running MT4 DeviceSession CLI (--start-session / --run-midi / --auto-session).

#pragma once

// Lab / manual session: start immediately (optional Zadig fallback).
// preserveCancel=true keeps a prior Ctrl+C (used by --auto-session after wait).
// Lab one-shot: process exits on mid-session USB loss (scripts expect exit).
int runMt4MidiSession(bool allowZadigFallback, bool preserveCancel = false);

// Auto-Start / product host: wait/rescan for project WinUSB GUID if absent, then
// start session. Survives mid-session unplug by Stop → wait → new DeviceSession
// (AD-9 / AD-10). Does not enable Zadig fallback. Not a Windows Service (AD-20).
int runMt4AutoSession();
