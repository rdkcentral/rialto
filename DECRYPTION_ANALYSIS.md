# Rialto Decryption Architecture Analysis

## Executive Summary

This document provides a comprehensive analysis of where decryption is being completed in the Rialto media framework. Rialto uses a layered architecture where decryption operations flow from GStreamer pipelines through various abstraction layers, ultimately calling the OpenCDM (Open Content Decryption Module) library.

---

## Main Decryption Source Files

### Core Implementation Files

1. **`media/server/gstplayer/source/GstDecryptor.cpp`**
   - GStreamer plugin element for decryption
   - Entry point for encrypted buffers in the GStreamer pipeline
   - Implements `gst_rialto_decryptor_transform_ip()` callback

2. **`media/server/main/source/MediaKeysServerInternal.cpp`**
   - High-level decryption service orchestrator
   - Manages multiple key sessions
   - Provides thread-safe decryption API

3. **`media/server/main/source/MediaKeySession.cpp`**
   - Per-session decryption logic
   - Handles different DRM key systems (Widevine, PlayReady, etc.)
   - Special Netflix PlayReady handling

4. **`wrappers/source/OcdmSession.cpp`**
   - **Final decryption wrapper** - Makes the actual OpenCDM library call
   - Wraps external C library functions
   - Converts OpenCDM errors to Rialto error codes

### Supporting Files

- **`media/server/gstplayer/include/GstDecryptorPrivate.h`** - Private decryption handler
- **`media/server/main/interface/IDecryptionService.h`** - Decryption service interface
- **`media/server/gstplayer/source/GstProtectionMetadata.cpp`** - Metadata extraction
- **`wrappers/interface/IOcdmSession.h`** - OCDM session interface

---

## Decryption Call Chain

The complete decryption flow follows this path:

```
┌─────────────────────────────────────────────────────┐
│  GStreamer Media Pipeline (encrypted buffer)        │
└─────────────────────┬───────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────┐
│  GstRialtoDecryptor::transform_ip()                 │
│  File: GstDecryptor.cpp:242                         │
└─────────────────────┬───────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────┐
│  GstRialtoDecryptorPrivate::decrypt()               │
│  File: GstDecryptor.cpp:310                         │
│  Call: m_decryptionService->decrypt(...)            │
└─────────────────────┬───────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────┐
│  MediaKeysServerInternal::decrypt()                 │
│  File: MediaKeysServerInternal.cpp:570-577          │
│  (Queues to main thread for thread safety)          │
└─────────────────────┬───────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────┐
│  MediaKeysServerInternal::decryptInternal()         │
│  File: MediaKeysServerInternal.cpp:582-592          │
│  (Finds correct session and delegates)              │
└─────────────────────┬───────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────┐
│  MediaKeySession::decrypt()                         │
│  File: MediaKeySession.cpp:223-227                  │
│  Call: m_ocdmSession->decryptBuffer(...)            │
└─────────────────────┬───────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────┐
│  OcdmSession::decryptBuffer()                       │
│  File: OcdmSession.cpp:191-199                      │
│  ★ ACTUAL DECRYPTION OCCURS HERE ★                  │
│  Call: opencdm_gstreamer_session_decrypt_buffer()   │
└─────────────────────┬───────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────┐
│  OpenCDM Library (External DRM System)              │
│  - Widevine CDM                                     │
│  - PlayReady CDM                                    │
│  - ClearKey CDM                                     │
└─────────────────────────────────────────────────────┘
```

---

## Key Methods and Line Numbers

### 1. GStreamer Plugin Entry Point

**File:** `media/server/gstplayer/source/GstDecryptor.cpp`

```cpp
// Line 242-290: Main transform callback
static GstFlowReturn gst_rialto_decryptor_transform_ip(GstBaseTransform *base, GstBuffer *buffer)

// Line 310: Initiates decryption service call
m_decryptionService->decrypt(protectionData->keySessionId, buffer, caps);
```

### 2. Decryption Service Orchestrator

**File:** `media/server/main/source/MediaKeysServerInternal.cpp`

```cpp
// Line 570-577: Public API with thread safety
MediaKeyErrorStatus MediaKeysServerInternal::decrypt(int32_t keySessionId, 
                                                      GstBuffer *encrypted, 
                                                      GstCaps *caps)
{
    return m_mainThread->enqueueTaskAndWait<MediaKeyErrorStatus>(
        [&]() { return decryptInternal(keySessionId, encrypted, caps); });
}

// Line 582-592: Internal implementation
MediaKeyErrorStatus MediaKeysServerInternal::decryptInternal(int32_t keySessionId, 
                                                              GstBuffer *encrypted, 
                                                              GstCaps *caps)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    // ... validation ...
    status = sessionIter->second->decrypt(encrypted, caps);
    return status;
}
```

### 3. Session-Level Decryption

**File:** `media/server/main/source/MediaKeySession.cpp`

```cpp
// Line 223-235: Delegates to OCDM wrapper
MediaKeyErrorStatus MediaKeySession::decrypt(GstBuffer *encrypted, GstCaps *caps)
{
    initOcdmErrorChecking();
    
    MediaKeyErrorStatus status = m_ocdmSession->decryptBuffer(encrypted, caps);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to decrypt buffer");
    }
    
    if ((checkForOcdmErrors("decrypt")) && (MediaKeyErrorStatus::OK == status))
    {
        status = MediaKeyErrorStatus::FAIL;
    }
    
    return status;
}
```

### 4. **Actual Decryption Call (Final Layer)**

**File:** `wrappers/source/OcdmSession.cpp`

```cpp
// Line 191-199: ★ WHERE DECRYPTION ACTUALLY HAPPENS ★
MediaKeyErrorStatus OcdmSession::decryptBuffer(GstBuffer *encrypted, GstCaps *caps)
{
    if (!m_session)
    {
        return MediaKeyErrorStatus::FAIL;
    }
    
    // THIS IS THE ACTUAL DECRYPTION CALL TO THE EXTERNAL DRM LIBRARY
    OpenCDMError status = opencdm_gstreamer_session_decrypt_buffer(m_session, encrypted, caps);
    return convertOpenCdmError(status);
}
```

---

## Decryption Metadata and Protection Information

### Protection Metadata Extraction

**File:** `media/server/gstplayer/source/GstProtectionMetadata.cpp`

The system extracts the following information from GStreamer protection metadata:

1. **Key Session ID** - Identifies which DRM session to use
2. **Initialization Vector (IV)** - Required for decryption
3. **Key ID (KID)** - Identifies the encryption key
4. **Subsample Information** - Defines which parts of the buffer are encrypted
5. **Cipher Mode** - CENC, CBCS, CBC1, or CENS
6. **Encryption Pattern** - For pattern-based encryption (crypt bytes / skip bytes)

### Supported Cipher Modes

- **CENC** (AES-CTR) - Common Encryption
- **CBCS** (AES-CBC with pattern) - Common Encryption with Patterns
- **CBC1** (AES-CBC)
- **CENS** (AES-CTR with pattern)

---

## Thread Safety

Decryption operations are thread-safe through the use of `IMainThread::enqueueTaskAndWait()`:

```cpp
// Operations queued to main thread
return m_mainThread->enqueueTaskAndWait<MediaKeyErrorStatus>(
    [&]() { return decryptInternal(keySessionId, encrypted, caps); }
);
```

This ensures:
- All session access happens on the main thread
- No race conditions when accessing session maps
- Synchronous return of decryption status

---

## DRM System Support

### Supported Key Systems

1. **Widevine** - `com.widevine.alpha`
2. **PlayReady** - `com.microsoft.playready`
   - Special handling for Netflix PlayReady (`com.netflix.playready.h264` / `com.netflix.playready.h265`)
3. **ClearKey** - `org.w3.clearkey`

### Netflix PlayReady Special Handling

**File:** `media/server/main/source/MediaKeySession.cpp` (Line 179-220)

Netflix PlayReady requires special initialization:
```cpp
if (isNetflixPlayreadyKeySystem(m_keySystem))
{
    status = m_ocdmSession->selectKeyId(keyLength, keyId);
}
```

---

## Error Handling

### Error Checking Flow

1. **OCDM Error Initialization** - `initOcdmErrorChecking()`
2. **Decryption Call** - `decryptBuffer()`
3. **OCDM Error Validation** - `checkForOcdmErrors()`
4. **Error Conversion** - `convertOpenCdmError()`

### Error Types

- `MediaKeyErrorStatus::OK` - Success
- `MediaKeyErrorStatus::FAIL` - General failure
- `MediaKeyErrorStatus::BAD_SESSION_ID` - Invalid session
- Additional error codes mapped from OpenCDM errors

---

## Key Interfaces

### IDecryptionService

**File:** `media/server/main/interface/IDecryptionService.h`

```cpp
virtual MediaKeyErrorStatus decrypt(int32_t keySessionId, 
                                    GstBuffer *encrypted, 
                                    GstCaps *caps) = 0;
```

Implemented by `MediaKeysServerInternal`

### IOcdmSession

**File:** `wrappers/interface/IOcdmSession.h`

```cpp
virtual MediaKeyErrorStatus decryptBuffer(GstBuffer *encrypted, 
                                          GstCaps *caps) = 0;
```

Implemented by `OcdmSession`

---

## Testing

### Component Tests

- **`tests/componenttests/client/tests/eme/SessionReadyForDecryptionTest.cpp`**
- **`tests/componenttests/server/tests/mediaKeys/SessionReadyForDecryptionTest.cpp`**

### Unit Tests

- **`tests/unittests/media/server/gstplayer/decryptor/DecryptTest.cpp`**
- **`tests/unittests/media/server/main/mediaKeys/DecryptTest.cpp`**
- **`tests/unittests/media/server/main/mediaKeySession/DecryptBufferTest.cpp`**

---

## Summary

**Decryption is completed in:** `wrappers/source/OcdmSession.cpp` at line 198

The actual decryption operation happens when `OcdmSession::decryptBuffer()` calls:
```cpp
opencdm_gstreamer_session_decrypt_buffer(m_session, encrypted, caps);
```

This is the final layer in the Rialto architecture that interfaces with the external OpenCDM library, which in turn communicates with the actual DRM CDM (Content Decryption Module) such as Widevine or PlayReady to perform the cryptographic decryption of the media content.

The architecture follows a clear separation of concerns:
- **GStreamer layer** - Handles media pipeline integration
- **Service layer** - Manages sessions and thread safety
- **Session layer** - Handles DRM-specific logic
- **Wrapper layer** - Interfaces with external DRM systems

This design allows Rialto to support multiple DRM systems while maintaining a clean, testable architecture.
