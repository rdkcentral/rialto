# Rialto Decryption - Quick Reference Guide

## Where is Decryption Completed?

**Answer: The actual decryption happens in `wrappers/source/OcdmSession.cpp` at line 198**

```cpp
OpenCDMError status = opencdm_gstreamer_session_decrypt_buffer(m_session, encrypted, caps);
```

This line calls the external OpenCDM library which interfaces with DRM systems (Widevine, PlayReady, etc.) to perform the actual cryptographic decryption.

---

## Critical Files for Decryption

| File | Location | Purpose |
|------|----------|---------|
| **OcdmSession.cpp** | `wrappers/source/` | **Final decryption call** - Line 198 |
| **MediaKeySession.cpp** | `media/server/main/source/` | Session-level decryption logic |
| **MediaKeysServerInternal.cpp** | `media/server/main/source/` | Decryption orchestrator |
| **GstDecryptor.cpp** | `media/server/gstplayer/source/` | GStreamer plugin entry point |

---

## Decryption Flow (Simplified)

```
GStreamer → GstDecryptor → MediaKeysServerInternal → MediaKeySession → OcdmSession → OpenCDM Library
```

---

## Key Line Numbers

| Operation | File | Line | Code |
|-----------|------|------|------|
| GStreamer callback | GstDecryptor.cpp | 242 | `gst_rialto_decryptor_transform_ip()` |
| Service call | GstDecryptor.cpp | 310 | `m_decryptionService->decrypt()` |
| Thread safety wrapper | MediaKeysServerInternal.cpp | 570 | `m_mainThread->enqueueTaskAndWait()` |
| Session lookup | MediaKeysServerInternal.cpp | 590 | `sessionIter->second->decrypt()` |
| OCDM wrapper call | MediaKeySession.cpp | 227 | `m_ocdmSession->decryptBuffer()` |
| **Actual decryption** | **OcdmSession.cpp** | **198** | **`opencdm_gstreamer_session_decrypt_buffer()`** |

---

## Supported DRM Systems

- Widevine (`com.widevine.alpha`)
- PlayReady (`com.microsoft.playready`)
- Netflix PlayReady (`com.netflix.playready.h264`, `com.netflix.playready.h265`)
- ClearKey (`org.w3.clearkey`)

---

## Supported Cipher Modes

- **CENC** - AES-CTR (Common Encryption)
- **CBCS** - AES-CBC with pattern
- **CBC1** - AES-CBC
- **CENS** - AES-CTR with pattern

---

## Architecture Layers

1. **GStreamer Layer** - Media pipeline integration
2. **Service Layer** - Thread safety and session management
3. **Session Layer** - DRM-specific logic
4. **Wrapper Layer** - External library interface (OpenCDM)
5. **DRM Layer** - Actual decryption (Widevine/PlayReady CDM)

---

## How to Trace a Decryption

1. Start at GStreamer pipeline with encrypted buffer
2. Follow to `GstDecryptor.cpp:310` - Initial service call
3. Thread hop to `MediaKeysServerInternal.cpp:590` - Session delegation
4. Enter `MediaKeySession.cpp:227` - OCDM call
5. **Final destination: `OcdmSession.cpp:198`** - OpenCDM library call

---

## Related Interfaces

- `IDecryptionService` - Decryption service interface
- `IOcdmSession` - OCDM session interface
- `IGstGenericPlayer` - Player interface with decryption support

---

## For More Details

See the full analysis in `DECRYPTION_ANALYSIS.md`
