// Minimal project-owned VirtualMIDI C API surface for runtime LoadLibrary.
// Do not paste large third-party SDK samples. Flags match public teVirtualMIDI docs.

#pragma once

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// Bits for virtualMIDICreatePortEx2 (public teVirtualMIDI flag set).
inline constexpr DWORD kTeVmFlagsParseRx = 1;
inline constexpr DWORD kTeVmFlagsParseTx = 2;
inline constexpr DWORD kTeVmFlagsInstantiateRx = 4;
inline constexpr DWORD kTeVmFlagsInstantiateTx = 8;

// Default queue / max sysex size used at port create.
#include "Midi/TeVirtualMidiLimits.h"
inline constexpr DWORD kTeVmDefaultMaxSysexLength =
    static_cast<DWORD>(kTeVmDefaultMaxSysexLengthSize);

#ifdef __cplusplus
extern "C" {
#endif

struct TeVmMidiPort;
typedef struct TeVmMidiPort* TeVmMidiPortHandle;

typedef void(CALLBACK* TeVmMidiDataCallback)(
    TeVmMidiPortHandle midiPort,
    LPBYTE midiDataBytes,
    DWORD length,
    DWORD_PTR callbackInstance);

typedef TeVmMidiPortHandle(CALLBACK* TeVmCreatePortEx2Fn)(
    LPCWSTR portName,
    TeVmMidiDataCallback callback,
    DWORD_PTR callbackInstance,
    DWORD maxSysexLength,
    DWORD flags);

typedef void(CALLBACK* TeVmClosePortFn)(TeVmMidiPortHandle midiPort);

typedef BOOL(CALLBACK* TeVmSendDataFn)(
    TeVmMidiPortHandle midiPort,
    LPBYTE midiDataBytes,
    DWORD length);

#ifdef __cplusplus
}
#endif

#endif // _WIN32
