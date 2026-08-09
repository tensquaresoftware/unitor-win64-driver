// Long-running MT4 DeviceSession CLI (--start-session / --run-midi / --auto-session).

#pragma once

// Lab / manual session: start immediately (optional Zadig fallback).
// preserveCancel=true keeps a prior Ctrl+C (used by --auto-session after wait).
int runMt4MidiSession(bool allowZadigFallback, bool preserveCancel = false);

// Auto-Start host: wait/rescan for project WinUSB GUID if absent, then start session.
// Does not enable Zadig fallback (product Auto-Start path).
int runMt4AutoSession();
