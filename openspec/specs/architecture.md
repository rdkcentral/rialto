# Architecture Spec: Rialto ServerManager

## Scope
This spec defines the approved architecture baseline for Rialto ServerManager only.

- In scope: `serverManager/public`, `serverManager/service`, `serverManager/common`, `serverManager/ipc`, `serverManager/serverManagerSim`, and `proto/servermanagermodule.proto`.
- Out of scope: full-platform architecture details outside ServerManager.

## Source of Truth
- Approved architectural source: `serverManager/architecture-brief.md`.
- Operational incident context: `serverManager/SME-notes.md`.
- Repository-wide architecture docs (for context only, not normative for this spec): `docs/architecture-brief.md`, `logging/architecture.md`.

## Purpose
ServerManager orchestrates `RialtoSessionServer` lifecycle per application and provides deterministic session-state transitions, local IPC control, healthcheck monitoring, and recovery behavior.

## Primary Responsibilities
- Spawn and configure session-server processes.
- Drive app/session lifecycle transitions.
- Manage ping/ack healthcheck and recovery thresholds.
- Propagate state changes to host integrators through observer callbacks.
- Provide simulator-backed control paths for integration testing.

## Architectural Building Blocks

### Public API Layer
- Entry interface: `IServerManagerService`.
- Observer interface: `IStateObserver` with `stateChanged(appId, state)`.
- Factory creation path used by host process.

### Service Layer
- `ServerManagerService` and `ServiceContext` coordinate initialization and orchestration wiring.
- Config resolution is centralized via `ConfigHelper` (including optional config-file precedence when enabled).

### Common Orchestration Layer
- `SessionServerAppManager` serializes lifecycle events on a single manager thread model.
- `SessionServerApp` owns per-app process/socket/session mappings.
- `HealthcheckService` tracks ping lifecycle and failed ping history by ping IDs.

### IPC Layer
- `Controller` manages per-server client registry.
- IPC client and loop exchange protobuf RPC/events with each session server.
- Contract service: `ServerManagerModule` (`setConfiguration`, `setState`, `setLogLevels`, `ping`).

### Simulator Layer
- `RialtoServerManagerSim` exposes HTTP test endpoints on port `9008`:
  - `POST /SetState/<AppName>/<NewState>`
  - `GET /GetState/<AppName>`
  - `GET /GetAppInfo/<AppName>`
  - `POST /SetLog/<component>/<level>`
  - `POST /Quit`

## State Model
Canonical session-server states:
- `UNINITIALIZED`
- `INACTIVE`
- `ACTIVE`
- `NOT_RUNNING`
- `ERROR`

Expected lifecycle includes cold start and preload paths, plus runtime transitions:
- `NOT_RUNNING -> UNINITIALIZED`
- `NOT_RUNNING -> INACTIVE`
- `UNINITIALIZED -> INACTIVE`
- `INACTIVE -> ACTIVE`
- `ACTIVE -> INACTIVE`
- `INACTIVE -> NOT_RUNNING`
- `ACTIVE -> NOT_RUNNING`

## Healthcheck and Recovery Contract
- Healthcheck runs periodically using a single manager timer.
- Ack matching is strict by ping ID.
- Timeouts/failures move assigned apps toward `ERROR` and update failure history.
- Recovery is triggered at `numOfFailedPingsBeforeRecovery` threshold.
- Outdated successful acks are treated as stale timing signals and can remove corresponding failed-ping records without being treated as current-cycle success.

## Integration Contracts
- Host controls lifecycle through public API methods (`initiateApplication`, `changeSessionServerState`, `getAppConnectionInfo`, `setLogLevels`).
- Manager-to-session-server control is local protobuf over Unix socketpair/fd channels.
- App connection details are delivered via `getAppConnectionInfo(appId)`.

## Non-Functional Expectations
- Deterministic event ordering via serialized manager orchestration.
- Local-only communication boundaries for control plane.
- Failure containment with restart-based recovery.
- Configurable socket permission/ownership policy for deployment environments.

## Constraints and Notes
- This document intentionally avoids asserting non-ServerManager architecture as normative.
- If broader architecture docs conflict with implementation, ServerManager implementation and `serverManager/architecture-brief.md` take precedence for this spec.

---

# Architecture Spec: GstPlayer

## Scope
This spec defines the approved architecture baseline for the GstPlayer layer only.

- In scope: `media/server/gstplayer/source`, `media/server/gstplayer/include`, `media/server/gstplayer/interface`, and all task files under `source/tasks/generic/` and `source/tasks/webAudio/`.
- Out of scope: `media/server/main`, `media/server/service`, `media/server/ipc`, and all layers above the `IGstGenericPlayer` / `IGstWebAudioPlayer` / `IGstCapabilities` interfaces.

## Source of Truth
- Approved architectural source: `media/server/gstplayer/architecture.md`.
- Operational context: `media/server/gstplayer/SME-notes.md`.

## Purpose
GstPlayer is the GStreamer pipeline management layer of `RialtoServer`. It constructs and drives GStreamer pipelines for audio, video, subtitle, and web-audio media; injects compressed frames from shared memory into `GstRialtoSrc` appsrc elements; attaches per-frame EME protection metadata for encrypted streams; reports pipeline events back to the server domain; and enumerates platform-supported MIME types and decoder properties via `GstCapabilities`.

## Primary Responsibilities
- Construct and manage GStreamer pipelines for A/V/subtitle and web-audio playback.
- Serialize all GStreamer pipeline mutations through a per-player `WorkerThread` task queue.
- Read compressed media frames from shared memory and push them into `GstAppSrc` elements via the task queue.
- Attach `GstRialtoProtectionMetadata` to encrypted buffers before GStreamer decryptor elements process them.
- Forward GStreamer bus messages (state changes, errors, EOS) from `GstDispatcherThread` to `WorkerThread` via `HandleBusMessage` tasks.
- Detect audio underflow via a periodic timer-based `CheckAudioUnderflow` task and report it to the server domain.
- Initialize GStreamer once via `GstInitialiser` singleton and gate `GstCapabilities` queries until initialization is complete.

## Architectural Building Blocks

### Public Interface Layer (`interface/`)
- `IGstGenericPlayer` — full A/V pipeline lifecycle (attach source, inject samples, seek, play, pause, stop, volume, rate, geometry, heartbeat).
- `IGstWebAudioPlayer` — PCM web audio pipeline lifecycle (set caps, write, play, pause, stop, volume).
- `IGstCapabilities` — MIME type and decoder property queries, gated by async initialization.
- `IGstGenericPlayerClient` — callbacks from GstPlayer to the server domain (playback state, NeedMediaData, position, underflow, EOS, first frame).
- `IGstInitialiser` — singleton `gst_init` wrapper; exposes `waitForInitialisation()` gate.

### Generic Player Layer (`GstGenericPlayer.cpp`)
- `GstGenericPlayer` implements `IGstGenericPlayer`, `IGstGenericPlayerPrivate`, and `IGstDispatcherThreadClient`.
- Owns `GenericPlayerContext` (all mutable pipeline state), `WorkerThread`, `GstDispatcherThread`, `IGenericPlayerTaskFactory`, `GstProfiler`, `FlushOnPrerollController`, and `FlushWatcher`.
- Every public API call converts to an `IPlayerTask` and is enqueued on `WorkerThread`, guaranteeing single-threaded GStreamer access.
- `GstDispatcherThread` polls the GStreamer bus and enqueues `HandleBusMessage` tasks to `WorkerThread` for processing.

### Generic Player Tasks (`tasks/generic/` — 37 tasks)
Each task encapsulates one atomic pipeline operation. Key tasks:
- `AttachSource` — creates `GstCaps` via `MediaSourceCapsBuilder`, adds an appsrc pad to `GstRialtoSrc`, optionally inserts a decryptor element.
- `ReadShmDataAndAttachSamples` — reads compressed data from the shared memory buffer, builds a `GstBuffer`, attaches `GstRialtoProtectionMetadata` for encrypted frames, pushes to the appsrc.
- `SetupElement` — connects decoder element signals (first-frame, audio underflow probes) once GStreamer auto-plugs elements.
- `CheckAudioUnderflow` — runs on timer; compares pipeline clock position against last decoded audio timestamp to detect stalls (threshold: 350 ms).
- `Flush` — sends `flush-start`/`flush-stop` on the appsrc; coordinates with `FlushOnPrerollController` to avoid the GStreamer preroll race (gstreamer/gstreamer#150).
- `SetSourcePosition` — records the target seek position, applied rate, and stop position per source type; queues as `SegmentData` in `GenericPlayerContext::initialPositions` for audio/video, or applies immediately via `gObjectSet` on the subtitle sink if source setup is already finished.

### Web Audio Player Layer (`GstWebAudioPlayer.cpp`)
- Separate, simpler pipeline for PCM web audio. Implements `IGstWebAudioPlayer` and `IGstWebAudioPlayerPrivate`.
- Uses a push-mode shared memory model: the client app writes to a circular buffer directly and calls `writeBuffer` when data is ready, rather than waiting for a `NeedData` / `haveData` round-trip.
- 8 tasks cover the full lifecycle: `SetCaps`, `WriteBuffer`, `Play`, `Pause`, `Stop`, `Shutdown`, `Eos`, `HandleBusMessage`.

### Capabilities Layer (`GstCapabilities.cpp`)
- Implements `IGstCapabilities`. Initialization is fully asynchronous: the background init thread waits for `GstInitialiser::waitForInitialisation()`, calls `fillSupportedMimeTypes()` (queries decoder, parser–decoder chain, and sink factory lists from GStreamer, saves results into `m_supportedMimeTypes`), sets `m_isInitialised`, notifies the condition variable, and exits. The thread exits only after all required information has been collected from GStreamer elements.
- All public query methods block on the condition variable until initialization is complete.
- `getSupportedMimeTypes(SUBTITLE)` always returns a hardcoded list (`{"text/vtt", "text/ttml", "text/cc"}`); it does not query GStreamer for subtitle support.

### Support Components
- `GstRialtoSrc` — custom `GstBin`-derived source element; one `GstAppSrc` per stream, with optional inline `GstRialtoDecryptor` for encrypted streams. Singleton per process (shared by all `GstGenericPlayer` instances).
- `GstProtectionMetadata` — registers a custom `GstMeta` type carrying all EME per-frame fields: key session ID, IV, subsamples, cipher mode, crypt/skip pattern.
- `GstInitialiser` — singleton; calls `gst_init` once and gates dependent components.
- `CapsBuilder` — builds `GstCaps` from `IMediaPipeline::MediaSource` descriptors.
- `FlushOnPrerollController` — workaround for gstreamer/gstreamer#150; synchronizes `Flush` tasks with the preroll path.
- `GstLogForwarding` — bridges GStreamer log messages into Rialto's logging system.
- `GstProfiler` — records per-stage pipeline timing via `IProfiler`.

## Integration Contracts
- All GStreamer API calls are routed through `IGstWrapper` and `IGlibWrapper` for testability; no direct GStreamer calls appear outside wrapper invocations.
- `IDecryptionService` is the only coupling between GstPlayer and the CDM layer; decryptor elements call back through this interface for per-buffer key session operations.
- `GstRialtoSrc` registers with rank `GST_RANK_PRIMARY + 100` so GStreamer auto-plugging selects it as the source element.
- Platform decoders (`omxh265dec`, `omxeac3dec`, `westerossink`) and OpenCDM decryptor elements are loaded at runtime via the GStreamer plugin registry; no compile-time coupling.

## Non-Functional Expectations
- All GStreamer pipeline mutations serialized on `WorkerThread`; no direct GStreamer API calls from any other thread, including `GstDispatcherThread` or OCDM callback threads.
- `GstCapabilities` queries always block on a condition variable if initialization has not yet completed; callers must not assume the result is available immediately after construction.
- Audio underflow detection threshold is hardcoded at 350 ms (`kAudioUnderflowMarginNs`); it is not configurable without a code change.
- `GstRialtoSrc` is a singleton; all `GstGenericPlayer` instances share one object. Destruction is reference-counted.
- `appsrc` buffer sizes are hardcoded: 8 MB (video), 512 KB (audio), 256 KB (subtitle).

## Deployment Architecture
- Compiled as a static library (`RialtoServerGstPlayer`) linked into `RialtoServer`.
- Runs entirely inside the `RialtoServer` process.
- Each `GstGenericPlayer` instance owns two runtime threads: `WorkerThread` (pipeline mutations) and `GstDispatcherThread` (GStreamer bus poll loop).
- `GstCapabilities` background init thread exits after `fillSupportedMimeTypes()` completes, not when `gst_init` completes.
- One `GstInitialiser` singleton per `RialtoServer` process, initialized once in `main()`.
- Multiple `GstGenericPlayer` instances can coexist in the same process, each with independent pipelines and threads.

## Constraints and Notes
- This spec is derived from `media/server/gstplayer/architecture.md`. If broader architecture docs conflict with implementation, the gstplayer implementation and `media/server/gstplayer/architecture.md` take precedence for this spec.
- The `FlushOnPrerollController` is a permanent workaround for an unresolved upstream GStreamer bug; it must not be removed without confirming the upstream issue is resolved.
- `rtkv1sink` must never be initialized during `GstCapabilities::fillSupportedMimeTypes` on platforms where doing so causes a video plane side effect; the existing WORKAROUND comment in the code must be preserved.