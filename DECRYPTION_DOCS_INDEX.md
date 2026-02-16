# Rialto Decryption Analysis - Documentation Index

This directory contains comprehensive documentation analyzing where and how decryption is completed in the Rialto media framework.

## Quick Answer

**Decryption is completed in:** `wrappers/source/OcdmSession.cpp` at line 198

```cpp
OpenCDMError status = opencdm_gstreamer_session_decrypt_buffer(m_session, encrypted, caps);
```

## Documentation Files

### 1. **DECRYPTION_SUMMARY.txt** - Start Here
Executive summary with the answer to "where is decryption completed?"
- Primary location with file path and line number
- Quick decryption flow overview
- Key files list
- Supported DRM systems and cipher modes

### 2. **DECRYPTION_QUICK_REFERENCE.md** - Quick Lookup
Fast reference guide for developers
- Critical files table
- Simplified decryption flow
- Key line numbers
- Architecture layers

### 3. **DECRYPTION_FLOW_DIAGRAM.md** - Visual Guide
Detailed visual diagram showing the complete decryption flow
- Layer-by-layer breakdown
- ASCII art flow diagram
- Code snippets at each layer
- Design patterns used

### 4. **DECRYPTION_ANALYSIS.md** - Complete Analysis
Comprehensive deep-dive documentation
- Detailed architecture explanation
- All source files involved
- Complete call chain with line numbers
- Protection metadata handling
- Thread safety mechanisms
- DRM system support details
- Error handling flow
- Testing information

## How to Use This Documentation

**If you want:**
- A quick answer → Read `DECRYPTION_SUMMARY.txt`
- To trace code quickly → Use `DECRYPTION_QUICK_REFERENCE.md`
- To understand the flow → View `DECRYPTION_FLOW_DIAGRAM.md`
- Complete technical details → Study `DECRYPTION_ANALYSIS.md`

## Key Findings Summary

### Where Decryption Happens
The actual decryption operation occurs in the `OcdmSession` wrapper class, which calls the external OpenCDM library. This library interfaces with DRM Content Decryption Modules (Widevine, PlayReady, etc.) to perform the cryptographic decryption.

### Decryption Architecture Layers
1. **GStreamer Layer** - Pipeline integration (`GstDecryptor.cpp`)
2. **Service Layer** - Thread safety and orchestration (`MediaKeysServerInternal.cpp`)
3. **Session Layer** - DRM-specific logic (`MediaKeySession.cpp`)
4. **Wrapper Layer** - External library interface (`OcdmSession.cpp`) ★
5. **DRM Layer** - Actual decryption (OpenCDM library)

### Critical Code Locations
| File | Line | Description |
|------|------|-------------|
| `OcdmSession.cpp` | 198 | **Actual decryption call** |
| `MediaKeySession.cpp` | 227 | Session-level decryption delegation |
| `MediaKeysServerInternal.cpp` | 590 | Session lookup and invocation |
| `GstDecryptor.cpp` | 310 | GStreamer integration entry point |

## Repository Context

This analysis was performed on the **rdkcentral/rialto** repository, which is a media framework that provides DRM (Digital Rights Management) support for protected content playback using GStreamer.

### Related Technologies
- **GStreamer** - Multimedia framework
- **OpenCDM** - Open Content Decryption Module API
- **Widevine/PlayReady** - DRM systems
- **EME** - Encrypted Media Extensions

---

**For questions or clarifications, refer to the detailed analysis documents listed above.**
