# Proposal: ServerManager Media Capabilities — `IMediaCapabilities`

## Criteria Coverage

This proposal addresses the following Rialto Session Server change requirements:

| Criterion                                                    | Coverage                                                                                                                                                                                         |
| ------------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Integrate with RialtoServerManager via`IMediaCapabilities` | `IMediaCapabilities` added to `serverManager/public/`; YAML-backed implementation in `serverManager/service/`                                                                              |
| Define and implement new MediaCapabilities protobuf service  | Extend`SetConfigurationRequest` in `servermanagermodule.proto` with **required** audio/video capability fields; session server deserialises and passes them to `configureServices()` |
| Forward capability responses to Rialto Client Library        | `IMediaCapabilities` replaces `IMediaPipelineCapabilities` capability methods on the client side; session server serves via `IMediaCapabilities` with GStreamer-query fallback when no YAML data is supplied by ServerManager |
| Maintain existing IPC architecture (no COM-RPC)              | All new IPC uses`firebolt::rialto::ipc` Unix socket + protobuf; no COM-RPC introduced                                                                                                          |

## Why

The Rialto session server loads audio and video decoder capabilities from HFP YAML files
(`/product/hfp/config/hfp-audiodecoder.yaml`, `/product/hfp/config/hfp-videodecoder.yaml`) via
`IYamlCppWrapper` and exposes them through `IMediaPipelineCapabilities`. Capabilities can
already be queried from the client library without an active media pipeline session.

However, the YAML loading happens independently inside every spawned session server process.
The ServerManager, which orchestrates those processes, has no visibility into what codecs are
available and cannot supply that information to the session servers it launches. Each session
server loads its own YAML at startup, creating redundant per-process file reads.

Centralising capability loading in the ServerManager gives it authoritative knowledge of
available codecs at the orchestration layer and allows it to supply pre-parsed capabilities
to each session server via the existing `SetConfiguration` call. On the client side, the
existing `IMediaPipelineCapabilities` capability methods (`getSupportedAudioCapabilities`,
`getSupportedVideoCapabilities`) are retired and replaced by the new `IMediaCapabilities`
interface. Side-by-side support is not carried forward; the legacy code paths are removed.
When the HFP YAML files are absent from a platform, the session server falls back to its
existing GStreamer element-query mechanism to populate the `IMediaCapabilities` response —
the new interface always returns a valid answer regardless of whether YAML is present.

## Architecture

```
ServerManager process
  └── SessionServerAppManager  (updated)
        └── IMediaCapabilities → IYamlCppWrapper → HFP YAML files
              ↓  optional AudioCapabilities
              ↓  optional VideoCapabilities
  └── Client::performSetConfiguration()  (updated params)
        └── CapabilitySerialiser (new, rialto::servermanager::ipc)
              ↓  typed AudioCapabilities / VideoCapabilities in SetConfigurationRequest
        │  ServerManagerModule IPC  [existing Unix socket]
        ▼
Session Server process
  └── ServerManagerModuleService::setConfiguration()  (updated)
        └── CapabilityDeserialiser (new, firebolt::rialto::server::ipc)
              ↓  std::optional passed to configureServices()
  └── GstCapabilities  (updated)
        ├── [path A] pre-loaded from ServerManager → use directly
        └── [path B] no ServerManager data → GStreamer element query (legacy fallback)
  └── IMediaCapabilitiesModuleService  (new — replaces MediaPipelineCapabilitiesModuleService
        │          capability methods, reuses transport layer)
        │  MediaPipelineCapabilitiesModule IPC  [existing Unix socket]
        ▼
Rialto Client Library
  └── IMediaCapabilities client interface  (new, replaces getSupportedAudio/VideoCapabilities
        └── on IMediaPipelineCapabilities)
```

Capabilities are loaded once in the ServerManager and forwarded via typed proto fields in
`SetConfigurationRequest`. In the session server, `GstCapabilities` uses the received data if
present, or falls back to the GStreamer element-query path when the ServerManager sent no
capabilities. The Rialto Client Library calls the new `IMediaCapabilities` interface; the
legacy `IMediaPipelineCapabilities` capability methods are removed. `rialtomse*` sink
components that currently use `IMediaPipelineCapabilities` are migrated to `IMediaCapabilities`.

## What Changes

### 1 — Common: move capability structs to `common/`

- Move `AudioDecoderCapabilities.h` and `VideoDecoderCapabilities.h` from
  `media/public/include/` (namespace `firebolt::rialto`) to `common/public/include/`
  (namespace `firebolt::rialto::common`).
  - The ServerManager lives in namespace `rialto::servermanager` and already depends on
    `firebolt::rialto::common` types (e.g. `SessionServerCommon.h`, `MaxResourceCapabilitites`).
    Moving the capability structs to the same `common` location makes them accessible from
    both the ServerManager and the session server without cross-layer includes.
  - Update all existing includes of these headers in `media/` to use the new path.
  - Verify that `IYamlCppWrapper.h` and `GstCapabilities.cpp` references are updated.

### 2 — ServerManager: `IMediaCapabilities` interface and implementation

- Add `IMediaCapabilities.h` to `serverManager/public/include/`:

  - New interface class `IMediaCapabilities` in namespace `rialto::servermanager::service`.
  - Two pure virtual methods:
    - `DecoderCapabilitiesStatus getAudioDecoderCapabilities(AudioDecoderCapabilities &capabilities)`
    - `DecoderCapabilitiesStatus getVideoDecoderCapabilities(VideoDecoderCapabilities &capabilities)`
  - Returns `DecoderCapabilitiesStatus` (`OK`, `CONFIG_NOT_FOUND`, `SCHEMA_VALIDATION_FAILED`,
    `INTERNAL_ERROR`) matching the status contract defined in `IYamlCppWrapper.h`.
- Add `MediaCapabilitiesFactory.h` to `serverManager/public/include/`:

  - Standalone factory function `createMediaCapabilities()` returning
    `std::unique_ptr<IMediaCapabilities>`, mirroring the `ServerManagerServiceFactory.h` pattern.
- Add `MediaCapabilities.h/.cpp` to `serverManager/service/`:

  - Concrete class `MediaCapabilities : public IMediaCapabilities`.
  - Constructor accepts `std::shared_ptr<firebolt::rialto::wrappers::IYamlCppWrapper>` for
    testability.
  - Delegates both capability calls directly to the injected wrapper.
  - Logs at `INFO` on `CONFIG_NOT_FOUND`; at `WARN` on `SCHEMA_VALIDATION_FAILED` or
    `INTERNAL_ERROR`, matching the policy in `GstCapabilities.cpp`.
- Add `MediaCapabilitiesFactory.cpp` to `serverManager/service/source/`:

  - Production factory resolves `IYamlCppWrapperFactory::getFactory()` and constructs
    `MediaCapabilities` with the resulting wrapper.

### 3 — Proto: extend `SetConfigurationRequest` with typed capability fields

- Update `SetConfigurationRequest` in **`proto/servermanagermodule.proto` only**.
  The files at `serverManager/ipc/proto/servermanagermodule.proto` and
  `media/server/ipc/proto/servermanagermodule.proto` are **symbolic links** to the same
  file; they do not need separate modification.
  - Define two new message types within or imported by `servermanagermodule.proto`:
    ```proto
    message AudioCapabilities { ... }   // mirrors GetSupportedAudioCapabilitiesResponse
    message VideoCapabilities { ... }   // mirrors GetSupportedVideoCapabilitiesResponse
    ```
    These may either be defined inline in `servermanagermodule.proto` (within the `rialto`
    package) or in a new `mediacapabilities.proto` that both `servermanagermodule.proto`
    and `mediapipelinecapabilitiesmodule.proto` import. Either way, the fields use typed
    messages, not raw bytes.
  - Add the two typed fields to `SetConfigurationRequest`:
    ```proto
    optional AudioCapabilities audioCapabilities = 12;
    optional VideoCapabilities videoCapabilities = 13;
    ```
  - Field numbers 12 and 13 are the next available after field 11
    (`subtitleClockResyncInterval`).
  - Using typed proto messages (rather than `optional bytes`) makes the wire format
    self-describing, eliminates a manual serialise/parse step, and avoids the need for
    `CapabilitySerialiser`/`CapabilityDeserialiser` shim classes.

### 4 — ServerManager common: store capabilities in `SessionServerAppManager`

- Update `serverManager/common/source/SessionServerAppManager.h` and
  `serverManager/common/source/SessionServerAppManager.cpp`
  (namespace `rialto::servermanager::common`):

  - Add `std::shared_ptr<service::IMediaCapabilities>` member `m_mediaCapabilities`.
  - Add `std::optional<firebolt::rialto::common::AudioDecoderCapabilities> m_audioCapabilities`
    and `std::optional<firebolt::rialto::common::VideoDecoderCapabilities> m_videoCapabilities`.
  - In the constructor (or at first use), call `m_mediaCapabilities->getAudioDecoderCapabilities()`
    and `getVideoDecoderCapabilities()`; populate the optionals on `OK`; leave as `std::nullopt`
    on any non-`OK` status and log at `INFO` (`CONFIG_NOT_FOUND`) or `WARN` (other errors).
  - Pass both optionals as additional parameters to the existing
    `IController::performSetConfiguration()` call sites in `SessionServerAppManager.cpp`.
- Update `serverManager/ipc/include/IController.h` and both
  `performSetConfiguration()` overloads in `serverManager/ipc/source/Controller.cpp`:

  - Add `const std::optional<firebolt::rialto::common::AudioDecoderCapabilities>&` and
    `const std::optional<firebolt::rialto::common::VideoDecoderCapabilities>&` parameters.
  - Forward them to `Client::performSetConfiguration()`.
- Update both `performSetConfiguration()` overloads in `serverManager/ipc/source/Client.cpp`:

  - Accept the same two `std::optional` parameters (do **not** add them as class members;
    keep them as per-call parameters consistent with the existing pattern).
  - If both optionals have values, serialise each into `GetSupportedAudioCapabilitiesResponse`
    / `GetSupportedVideoCapabilitiesResponse` using **new converter functions** written in the
    `rialto::servermanager::ipc` namespace (see §5), serialize to bytes, and call
    `request.set_audiocapabilities(bytes)` / `request.set_videocapabilities(bytes)`.
  - If either optional is `std::nullopt`, leave both fields absent; emit `DEBUG` log.
  - Remove the `m_mediaCapabilities` member that was previously proposed for `Client.h`.

### 5 — New capability converter functions (`rialto::servermanager` namespace)

- Add `CapabilitySerialiser.h/.cpp` to `serverManager/ipc/source/`
  (namespace `rialto::servermanager::ipc`):

  - Provides converters from `firebolt::rialto::common::AudioDecoderCapabilities` /
    `VideoDecoderCapabilities` C++ structs into the typed proto `AudioCapabilities` /
    `VideoCapabilities` messages defined in `servermanagermodule.proto`.
  - These converters are required because the ServerManager uses namespace `rialto`, which
    is separate from `firebolt::rialto::client`; the client-side converters cannot be reused.
  - If `AudioCapabilities` / `VideoCapabilities` are defined in a shared proto (see §3),
    the `CapabilityDeserialiser` on the session server side may share the same generated
    types and the converter logic can be simplified or eliminated.

### 6 — Session Server: receive capabilities, implement `IMediaCapabilities` with GStreamer fallback

- Update `media/server/ipc/source/ServerManagerModuleService.cpp` —
  `ServerManagerModuleService::setConfiguration()`:

  - After existing socket configuration, check `request->has_audiocapabilities()` and
    `request->has_videocapabilities()`.
  - If both are present: convert the typed proto messages into
    `firebolt::rialto::common::AudioDecoderCapabilities` / `VideoDecoderCapabilities`,
    wrap in `std::optional`, and pass them as new parameters to
    `m_sessionServerManager.configureServices()`.
  - If absent: pass `std::nullopt` for both; emit `INFO` log; session server uses GStreamer
    fallback (see `GstCapabilities` below).
- Extend `configureServices()` in `media/server/service/include/ISessionServerManager.h` and
  `media/server/service/source/SessionServerManager.cpp` with two `std::optional` parameters.

- Update `GstCapabilities` to implement the two-path model:
  - **Path A — YAML / ServerManager data present**: use the pre-loaded
    `AudioDecoderCapabilities` / `VideoDecoderCapabilities` received from the ServerManager.
    Skip the `IYamlCppWrapper` YAML load.
  - **Path B — no ServerManager data**: populate capabilities by querying GStreamer elements
    (the existing logic used today by `IMediaPipelineCapabilities`). This is the fallback when
    the platform has no HFP YAML files. The legacy GStreamer-query code path is **retained
    inside `GstCapabilities`** as the fallback; it is not removed.

  **Note**: Path B is the same mechanism that `IMediaPipelineCapabilities` uses today. The
  legacy `getSupportedAudioCapabilities()` / `getSupportedVideoCapabilities()` methods on
  `IMediaPipelineCapabilities` are **removed** from that interface; the identical logic now
  lives exclusively in the `GstCapabilities` fallback path of `IMediaCapabilities`.

### 7 — Client Library: replace `IMediaPipelineCapabilities` capability methods with `IMediaCapabilities`

- Add `IMediaCapabilities.h` to `media/public/include/`:
  - Declares `getSupportedAudioCapabilities()` → `AudioDecoderCapabilities` and
    `getSupportedVideoCapabilities()` → `VideoDecoderCapabilities`.
  - This is the single capabilities interface for Rialto clients going forward.

- Remove `getSupportedAudioCapabilities()` and `getSupportedVideoCapabilities()` from
  `media/public/include/IMediaPipelineCapabilities.h` and their implementations.
  Side-by-side support is not carried forward.

- Add `MediaCapabilitiesIpc.h/.cpp` to `media/client/ipc/` (namespace
  `firebolt::rialto::client`):
  - Client-side IPC caller that maps `IMediaCapabilities` calls onto the
    `MediaPipelineCapabilitiesModule` service (reusing the existing IPC transport).
  - Deserialises the proto response into `AudioDecoderCapabilities` / `VideoDecoderCapabilities`.

- Update `media/client/main/` to expose `IMediaCapabilities` to application code via the
  existing factory pattern.

- Update `rialtomse*` sink components (and any other Rialto consumers of
  `IMediaPipelineCapabilities::getSupportedAudioCapabilities` /
  `getSupportedVideoCapabilities`) to call `IMediaCapabilities` instead.

### 8 — Build

- Update `serverManager/CMakeLists.txt`: link `serverManager/ipc/` against `RialtoWrappers`
  and `RialtoServerManagerService`.
- Update `media/client/ipc/CMakeLists.txt`: add `MediaCapabilitiesIpc.cpp`.
- `proto/CMakeLists.txt`: add new proto file if `AudioCapabilities`/`VideoCapabilities`
  messages are extracted into a shared proto.

### 9 — Unit tests

- Add `tests/unittests/serverManager/MediaCapabilitiesTest.cpp`: `OK`, `CONFIG_NOT_FOUND`,
  `SCHEMA_VALIDATION_FAILED`, `INTERNAL_ERROR` cases; inject mock `IYamlCppWrapper`.
- Update `tests/unittests/serverManager/SessionServerAppManagerTest.cpp`: capability
  optionals populated and passed to `performSetConfiguration()` on `OK`; `std::nullopt`
  passed on error.
- Update `tests/unittests/serverManager/ClientTest.cpp`: typed fields set in request when
  optionals have values; fields absent when `std::nullopt`.
- Update `tests/unittests/media/server/ipc/serverManagerModuleService/`
  `ServerManagerModuleServiceTestsFixture.cpp`: `configureServices()` called with correct
  optional capability values when fields present; `std::nullopt` passed when absent.
- Update `tests/unittests/media/server/gstplayer/GstCapabilitiesTest.cpp`:
  - Path A: pre-loaded capabilities used, YAML and GStreamer query both skipped.
  - Path B: no pre-loaded capabilities, GStreamer element query used as fallback.
- Add `tests/unittests/media/client/MediaCapabilitiesIpcTest.cpp`: successful audio/video
  response deserialisation; RPC failure path.

## Logging

Logging is a required part of this implementation, not optional. The following log points must be
present so that the working path and every failure mode are observable from the log without
attaching a debugger.

### `MediaCapabilities` (ServerManager service layer)

| Event                                         | Level     | Message content                                                                                            |
| --------------------------------------------- | --------- | ---------------------------------------------------------------------------------------------------------- |
| YAML load attempted                           | `DEBUG` | `"MediaCapabilities: loading audio capabilities from YAML"` / `"loading video capabilities from YAML"` |
| Load succeeded (`OK`)                       | `INFO`  | `"MediaCapabilities: audio capabilities loaded successfully, <N> codec(s) present"` / same for video     |
| YAML file absent (`CONFIG_NOT_FOUND`)       | `INFO`  | `"MediaCapabilities: HFP YAML config not found — returning empty capabilities"`                         |
| Schema invalid (`SCHEMA_VALIDATION_FAILED`) | `WARN`  | `"MediaCapabilities: YAML schema validation failed for audio/video capabilities"`                        |
| Internal parse error (`INTERNAL_ERROR`)     | `WARN`  | `"MediaCapabilities: internal error while parsing audio/video capabilities"`                             |

### `SessionServerAppManager` — capability loading and forwarding

| Event                                              | Level     | Message content                                                                             |
| -------------------------------------------------- | --------- | ------------------------------------------------------------------------------------------- |
| YAML load attempted                                | `DEBUG` | `"SessionServerAppManager: loading audio/video capabilities from YAML"`                   |
| Load succeeded (`OK`)                            | `INFO`  | `"SessionServerAppManager: capabilities loaded successfully"`                             |
| `CONFIG_NOT_FOUND`                               | `INFO`  | `"SessionServerAppManager: HFP YAML not found — capabilities will not be forwarded"`     |
| `SCHEMA_VALIDATION_FAILED` or `INTERNAL_ERROR` | `WARN`  | `"SessionServerAppManager: capability load failed — capabilities will not be forwarded"` |
| Capabilities forwarded                             | `DEBUG` | `"SessionServerAppManager: passing capabilities to SetConfiguration for server <id>"`     |

### `ServerManagerModuleService` — capability deserialisation on session server

| Event                           | Level     | Message content                                                                                  |
| ------------------------------- | --------- | ------------------------------------------------------------------------------------------------ |
| Capability bytes fields present | `DEBUG` | `"ServerManagerModuleService: audio/video capabilities received in SetConfiguration"`          |
| Deserialisation succeeded       | `INFO`  | `"ServerManagerModuleService: capabilities deserialised and forwarded to configureServices"`   |
| Capability fields absent        | `INFO`  | `"ServerManagerModuleService: no capability fields — session server will load YAML directly"` |

### Log level policy

Follows the existing Rialto policy (`GstCapabilities.cpp`, `ConfigHelper.cpp`):

- `DEBUG` — normal operation tracing; enabled only at debug log level.
- `INFO` — expected operational events visible at default log level (successful load, expected
  absent YAML, connect/disconnect).
- `WARN` — unexpected conditions that require investigation (schema errors, RPC failures,
  serialisation errors).
- `ERROR` is not used for capability queries; a failed load degrades gracefully to empty
  capabilities rather than terminating.

## Design Constraints

- `IYamlCppWrapper` / `YamlCppWrapper` in `wrappers/` are **not copied**. No YAML parsing
  logic is duplicated.
- `AudioDecoderCapabilities` and `VideoDecoderCapabilities` structs are moved to
  `common/public/include/` so both ServerManager and session server can access them without
  cross-layer includes.
- Capabilities are transported over the **existing** `ServerManagerModule` IPC socket via
  `SetConfigurationRequest` using **typed proto message fields** (not raw bytes).
- When the ServerManager sends no capability fields (HFP YAML absent), the session server
  falls back to the **GStreamer element-query mechanism** to populate capability responses.
  This fallback is the same code path that `IMediaPipelineCapabilities` uses today and it
  is **retained inside `GstCapabilities`**, not removed.
- `IMediaPipelineCapabilities::getSupportedAudioCapabilities()` and
  `getSupportedVideoCapabilities()` are **removed**. The new `IMediaCapabilities` interface
  is the single capabilities query surface for clients, `rialtomse*` sinks, and any other
  Rialto consumers. Side-by-side support is not provided.
- All new IPC uses the existing `firebolt::rialto::ipc` Unix socket + protobuf. No COM-RPC,
  D-Bus, or other mechanism is introduced.

## Capabilities

### New Capabilities

- `servermanager-media-capabilities`: Centralise audio/video decoder capability loading in
  the ServerManager via `IMediaCapabilities`; forward via typed proto fields in
  `SetConfigurationRequest`; expose through a new `IMediaCapabilities` client interface that
  replaces `IMediaPipelineCapabilities` capability methods; fall back to GStreamer element
  query when no YAML data is available.

### Modified Capabilities

None.

## Impact

Affected areas:

| Area                                                           | Change                                                                                                                           |
| -------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------- |
| `common/public/include/`                                     | `AudioDecoderCapabilities.h`, `VideoDecoderCapabilities.h` moved here from `media/public/include/`                         |
| `serverManager/public/include/`                              | Two new public headers:`IMediaCapabilities.h`, `MediaCapabilitiesFactory.h`                                                  |
| `serverManager/service/`                                     | Two new files:`MediaCapabilities.h/.cpp`, `MediaCapabilitiesFactory.cpp`                                                     |
| `serverManager/common/source/SessionServerAppManager.h/.cpp` | Add`IMediaCapabilities` member and `std::optional` capability storage; forward to `performSetConfiguration()`              |
| `serverManager/ipc/include/IController.h`                    | New`std::optional` capability parameters on `performSetConfiguration()` overloads                                            |
| `serverManager/ipc/source/Client.cpp`                        | Serialise`std::optional` capability parameters to bytes in `SetConfigurationRequest`                                         |
| `serverManager/ipc/source/CapabilitySerialiser.h/.cpp`       | New converter functions in`rialto::servermanager::ipc` namespace                                                               |
| `proto/servermanagermodule.proto`                            | Two new`optional bytes` fields (symlinks in `serverManager/ipc/proto/` and `media/server/ipc/proto/` update automatically) |
| `media/server/ipc/source/ServerManagerModuleService.cpp`     | Deserialise bytes and pass`std::optional` capabilities to `configureServices()`                                              |
| `media/server/ipc/source/CapabilityDeserialiser.h/.cpp`      | New converter functions in`firebolt::rialto::server::ipc` namespace                                                            |
| `media/server/service/include/ISessionServerManager.h`       | Extended`configureServices()` with two `std::optional` capability parameters                                                 |
| `media/server/service/source/SessionServerManager.cpp`       | Store and forward optionals to`GstCapabilities`                                                                                |
| `media/server/gstplayer/source/GstCapabilities.cpp`          | Accept pre-loaded optional capabilities; skip YAML when present                                                                  |
| `serverManager/CMakeLists.txt`                               | Link`serverManager/ipc/` against `RialtoWrappers`                                                                            |
| `media/server/gstplayer/source/GstCapabilities.cpp`          | Updated to accept pre-loaded capabilities; YAML load skipped when present                                                        |
| `serverManager/CMakeLists.txt`                               | Additional link dependency on`RialtoWrappers`                                                                                  |
| `tests/unittests/`                                           | New/updated tests for`MediaCapabilities`, `Client`, `ServerManagerModuleService`, `GstCapabilities`                      |

No changes to:

- Session lifecycle or state machine (`SessionServerApp`, `SessionServerAppManager`).
- Healthcheck service (`HealthcheckService`).
- `IServerManagerService` public API.
- Rialto Client Library (`media/client/`) — no new files, no modified files.
- `MediaPipelineCapabilitiesModuleService` — existing session server capability service is
  unchanged; it now serves capabilities received via `SetConfiguration` instead of from YAML.
