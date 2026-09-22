## 1. Shared-Memory Model

- [x] 1.1 Add role-specific media-pipeline and WebAudio shared-memory interfaces and factories, backed by a common memfd allocation primitive where appropriate.
- [x] 1.2 Implement the fixed media-pipeline layout with 7 MiB video, 1 MiB audio, and 256 KiB subtitle regions.
- [x] 1.3 Implement the fixed 10 KiB WebAudio layout.
- [x] 1.4 Add allocation, kernel-zeroing, full-region clearing, sealing, mapping, region-offset, bounds, cleanup, and failure-path unit tests.
- [x] 1.5 Remove or retire global partition APIs (`mapPartition`, `unmapPartition`, playback-count sizing, and ID-based region lookup) after callers migrate.

## 2. Creation Protocol

- [x] 2.1 Add descriptor and mapping-size fields to `CreateSessionResponse` in `proto/mediapipelinemodule.proto`, marking the descriptor with `rialto.ipc.field_is_fd`.
- [x] 2.2 Add descriptor and mapping-size fields to `CreateWebAudioPlayerResponse` in `proto/webaudioplayermodule.proto`, marking the descriptor with `rialto.ipc.field_is_fd`.
- [x] 2.3 Update generated-message consumers, IPC mocks, matchers, and creation-response helpers.
- [x] 2.4 Update schema compatibility/version handling so mixed global-buffer and per-instance peers are rejected.
- [x] 2.5 Add IPC tests proving each creation response transfers a usable duplicate descriptor and closes it correctly on failures.

## 3. Server Media-Pipeline Ownership

- [x] 3.1 Inject the media-pipeline shared-memory factory into `MediaPipelineService` through existing playback-service construction.
- [x] 3.2 Enforce maximum-session admission before allocating a buffer.
- [x] 3.3 Allocate and map one buffer in `MediaPipelineService::createSession` before publishing the session.
- [x] 3.4 Pass the buffer owner into `MediaPipelineServerInternal` and expose its descriptor and size to `MediaPipelineModuleService::createSession`.
- [x] 3.5 Replace session-ID partition mapping and unmapping with direct pipeline-local region access.
- [x] 3.6 Update `NeedMediaData` and media-region lookup to use instance-relative offsets without changing `MediaPlayerShmInfo` format.
- [x] 3.7 Add service and main-layer tests for two isolated pipelines, allocation failure, constructor failure, destruction, and disconnect cleanup.

## 4. Server WebAudio Ownership

- [x] 4.1 Inject the WebAudio shared-memory factory into `WebAudioPlayerService`.
- [x] 4.2 Enforce maximum-player admission before allocating a buffer.
- [x] 4.3 Allocate and map one 10 KiB buffer before publishing a WebAudio player.
- [x] 4.4 Pass the buffer owner into `WebAudioPlayerServerInternal` and return its descriptor and size from player creation.
- [x] 4.5 Replace handle-based partition mapping and unmapping with direct player-local ring access.
- [x] 4.6 Add tests for multiple isolated players, wrap addressing, failure cleanup, destruction, and disconnect cleanup.

## 5. Client Media-Pipeline Ownership

- [x] 5.1 Change media-pipeline IPC creation to return the session ID, received descriptor, and mapping size with explicit ownership semantics.
- [x] 5.2 Construct a pipeline-local `SharedMemoryHandle` and fail client pipeline creation when the response or mapping is invalid.
- [x] 5.3 Store the handle on `MediaPipeline` and use it for every media frame writer instead of `ClientController::getSharedMemoryHandle`.
- [x] 5.4 On lifecycle invalidation, block writes and clear pending need-data requests before releasing or replacing the mapping.
- [x] 5.5 Destroy the newly created server session if local mapping fails after server creation.
- [x] 5.6 Add tests for mapping ownership, two-pipeline isolation, invalid responses, mapping failures, server loss, fresh-descriptor recreation, and destruction.

## 6. Client WebAudio Ownership

- [x] 6.1 Change WebAudio IPC creation to return the player handle, received descriptor, and mapping size with explicit ownership semantics.
- [x] 6.2 Construct and store a player-local `SharedMemoryHandle` on `WebAudioPlayer`.
- [x] 6.3 Route main and wrap writes through the player-local mapping and preserve existing `WebAudioShmInfo` semantics.
- [x] 6.4 On lifecycle invalidation, block writes and release the player-local mapping; require a fresh descriptor before reuse.
- [x] 6.5 Destroy the newly created server player if local mapping fails after server creation.
- [x] 6.6 Add tests for two-player isolation, wrap writes, invalid responses, mapping failures, server loss, fresh-descriptor recreation, and destruction.

## 7. Remove Global Playback Shared Memory

- [x] 7.1 Remove shared-memory allocation from `PlaybackService::switchToActive` and global reset from `switchToInactive`.
- [x] 7.2 Remove playback-buffer accessors and factory ownership from `IPlaybackService`, `PlaybackService`, and their mocks where no longer required.
- [x] 7.3 Remove shared-memory initialization, lookup, and termination from `IClientController`, `ClientController`, and their mocks.
- [x] 7.4 Remove client use of `ControlIpc::getSharedMemory` and server use of `ControlModuleService::getSharedMemory`.
- [x] 7.5 Retire the `ControlModule.getSharedMemory` protobuf RPC according to repository protobuf compatibility conventions.
- [x] 7.6 Update application-state tests to verify state changes no longer allocate or map playback memory.

## 8. End-to-End and Non-Regression Coverage

- [x] 8.1 Add a client component test creating two media pipelines and verifying distinct creation descriptors and mappings.
- [x] 8.2 Add a server component test proving media requests for two sessions use mapping-local offsets and independent storage.
- [x] 8.3 Add corresponding multi-player WebAudio component coverage.
- [x] 8.4 Add inactive-transition and graceful-disconnect coverage proving all per-instance resources are released on both sides.
- [x] 8.5 Add deployed-topology process coverage proving abrupt application death, RialtoServer loss, and ServerManager restart release resources and require fresh mappings.
- [x] 8.6 Verify media payload metadata, encryption metadata, full-region clear-before-request behavior, frame writing, WebAudio wrap behavior, and public application APIs remain unchanged.

## 9. Validation

- [x] 9.1 Run OpenSpec validation for `per-instance-shared-memory` when the OpenSpec CLI is available.
- [x] 9.2 Run clang-format on changed C++ headers and sources and run recursive cpplint.
- [x] 9.3 Run cppcheck and inspect `cppcheck_report.txt` for new findings.
- [x] 9.4 Run affected unit-test suites during implementation, followed by the full unit-test build.
- [x] 9.5 Run the full component-test build.
- [x] 9.6 Run unit-test coverage and valgrind builds.
- [x] 9.7 Run the native production build.
- [x] 9.8 Review descriptor ownership and error paths for leaks, double closes, stale mappings, and cross-instance access.

## 10. Deployed-Topology Process Coverage

- [x] 10.1 Add a purpose-built application child executable that uses the Rialto client, reports instance readiness to its parent, and supports normal exit or abrupt termination without destroy RPCs.
- [x] 10.2 Add a process fixture that starts, monitors, kills, and restarts RialtoServer through RialtoServerManager semantics with bounded readiness and cleanup waits.
- [x] 10.3 Prove abrupt application-process death closes client mappings and triggers server disconnect cleanup, restores two-pipeline admission capacity, and returns server resources to baseline.
- [x] 10.4 Prove RialtoServer death makes surviving client instances release mappings, clear pending users, and reject writes.
- [x] 10.5 Prove ServerManager restart requires newly created instances with fresh memfd identities before data resumes and rejects stale IDs, handles, mappings, and requests.
- [x] 10.6 Prove terminating both processes leaves no persistent shared-memory artifact and a replacement server can admit instances normally.
