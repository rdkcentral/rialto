# Rialto Decryption Architecture - Visual Diagram

```
┌────────────────────────────────────────────────────────────────────────────┐
│                         RIALTO DECRYPTION FLOW                             │
└────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────────┐
│                         1. GSTREAMER LAYER                               │
│  ┌────────────────────────────────────────────────────────────────────┐  │
│  │  Encrypted Media Buffer (with protection metadata)                 │  │
│  │  - Key ID (KID)                                                     │  │
│  │  - Initialization Vector (IV)                                       │  │
│  │  - Subsample information                                            │  │
│  │  - Cipher mode (CENC/CBCS/CBC1/CENS)                               │  │
│  └───────────────────────────┬────────────────────────────────────────┘  │
│                              │                                            │
│  ┌───────────────────────────▼────────────────────────────────────────┐  │
│  │  GstRialtoDecryptor Element                                         │  │
│  │  File: media/server/gstplayer/source/GstDecryptor.cpp              │  │
│  │  Line: 242 - gst_rialto_decryptor_transform_ip()                   │  │
│  │                                                                      │  │
│  │  Extracts protection metadata and forwards to decryption service    │  │
│  └───────────────────────────┬────────────────────────────────────────┘  │
└──────────────────────────────┼───────────────────────────────────────────┘
                               │ Line 310: m_decryptionService->decrypt()
                               │
┌──────────────────────────────▼───────────────────────────────────────────┐
│                    2. DECRYPTION SERVICE LAYER                           │
│  ┌────────────────────────────────────────────────────────────────────┐  │
│  │  MediaKeysServerInternal::decrypt()                                 │  │
│  │  File: media/server/main/source/MediaKeysServerInternal.cpp        │  │
│  │  Line: 570                                                          │  │
│  │                                                                      │  │
│  │  Thread Safety: Queues operation to main thread                     │  │
│  │  m_mainThread->enqueueTaskAndWait()                                 │  │
│  └───────────────────────────┬────────────────────────────────────────┘  │
│                              │                                            │
│  ┌───────────────────────────▼────────────────────────────────────────┐  │
│  │  MediaKeysServerInternal::decryptInternal()                         │  │
│  │  Line: 582                                                          │  │
│  │                                                                      │  │
│  │  Looks up session: m_mediaKeySessions.find(keySessionId)            │  │
│  └───────────────────────────┬────────────────────────────────────────┘  │
└──────────────────────────────┼───────────────────────────────────────────┘
                               │ Line 590: sessionIter->second->decrypt()
                               │
┌──────────────────────────────▼───────────────────────────────────────────┐
│                      3. MEDIA KEY SESSION LAYER                          │
│  ┌────────────────────────────────────────────────────────────────────┐  │
│  │  MediaKeySession::decrypt()                                         │  │
│  │  File: media/server/main/source/MediaKeySession.cpp                │  │
│  │  Line: 223                                                          │  │
│  │                                                                      │  │
│  │  DRM-specific logic:                                                │  │
│  │  - Netflix PlayReady special handling                               │  │
│  │  - Error checking initialization                                    │  │
│  │  - OCDM error validation                                            │  │
│  └───────────────────────────┬────────────────────────────────────────┘  │
└──────────────────────────────┼───────────────────────────────────────────┘
                               │ Line 227: m_ocdmSession->decryptBuffer()
                               │
┌──────────────────────────────▼───────────────────────────────────────────┐
│                      4. OCDM WRAPPER LAYER                               │
│  ┌────────────────────────────────────────────────────────────────────┐  │
│  │  OcdmSession::decryptBuffer()                                       │  │
│  │  File: wrappers/source/OcdmSession.cpp                             │  │
│  │  Line: 191-199                                                      │  │
│  │                                                                      │  │
│  │  ╔══════════════════════════════════════════════════════════════╗  │  │
│  │  ║  ★★★ ACTUAL DECRYPTION CALL (LINE 198) ★★★                  ║  │  │
│  │  ║                                                               ║  │  │
│  │  ║  opencdm_gstreamer_session_decrypt_buffer(                   ║  │  │
│  │  ║      m_session,                                               ║  │  │
│  │  ║      encrypted,  // GstBuffer to decrypt                     ║  │  │
│  │  ║      caps         // GstCaps with stream info                ║  │  │
│  │  ║  )                                                            ║  │  │
│  │  ╚══════════════════════════════════════════════════════════════╝  │  │
│  │                                                                      │  │
│  │  Converts OpenCDM errors to Rialto error codes                      │  │
│  └───────────────────────────┬────────────────────────────────────────┘  │
└──────────────────────────────┼───────────────────────────────────────────┘
                               │
┌──────────────────────────────▼───────────────────────────────────────────┐
│                    5. EXTERNAL DRM LIBRARY LAYER                         │
│  ┌────────────────────────────────────────────────────────────────────┐  │
│  │  OpenCDM Library (open_cdm_adapter.cpp)                            │  │
│  │                                                                      │  │
│  │  Interfaces with DRM Content Decryption Modules:                    │  │
│  │  ┌───────────────┐  ┌────────────────┐  ┌──────────────┐          │  │
│  │  │  Widevine CDM │  │ PlayReady CDM  │  │ ClearKey CDM │          │  │
│  │  └───────────────┘  └────────────────┘  └──────────────┘          │  │
│  │                                                                      │  │
│  │  Performs actual cryptographic decryption:                          │  │
│  │  - AES-CTR decryption (CENC/CENS)                                   │  │
│  │  - AES-CBC decryption (CBCS/CBC1)                                   │  │
│  │  - Pattern-based decryption                                         │  │
│  │  - Subsample processing                                             │  │
│  └───────────────────────────┬────────────────────────────────────────┘  │
└──────────────────────────────┼───────────────────────────────────────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │  Decrypted Buffer    │
                    │  (Clear Content)     │
                    └──────────────────────┘


┌────────────────────────────────────────────────────────────────────────────┐
│                           KEY DESIGN PATTERNS                              │
├────────────────────────────────────────────────────────────────────────────┤
│  • Layered Architecture: Clear separation of concerns                      │
│  • Thread Safety: Main thread queuing for session operations               │
│  • Wrapper Pattern: Isolates external library dependencies                 │
│  • Factory Pattern: Creates decryptors, sessions, metadata helpers         │
│  • Error Handling: Continuous validation with detailed error reporting     │
└────────────────────────────────────────────────────────────────────────────┘


┌────────────────────────────────────────────────────────────────────────────┐
│                         SUPPORTED CONFIGURATIONS                           │
├────────────────────────────────────────────────────────────────────────────┤
│  DRM Systems:  Widevine, PlayReady, Netflix PlayReady, ClearKey           │
│  Cipher Modes: CENC (AES-CTR), CBCS (AES-CBC+pattern),                    │
│                CBC1 (AES-CBC), CENS (AES-CTR+pattern)                      │
│  Features:     Pattern encryption, Subsample decryption,                   │
│                Multi-key sessions, Error recovery                          │
└────────────────────────────────────────────────────────────────────────────┘
```

## Summary

**The actual decryption is completed at:**
- **File:** `wrappers/source/OcdmSession.cpp`
- **Method:** `OcdmSession::decryptBuffer()`
- **Line:** 198
- **Call:** `opencdm_gstreamer_session_decrypt_buffer(m_session, encrypted, caps)`

This call forwards the encrypted GStreamer buffer to the OpenCDM library, which interfaces with the platform's DRM Content Decryption Module (Widevine, PlayReady, etc.) to perform the actual cryptographic decryption of protected media content.
