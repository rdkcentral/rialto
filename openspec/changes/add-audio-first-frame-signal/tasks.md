## 1. Protobuf / IPC Definition

- [x] 1.1 Add `FirstFrameReceivedEvent` message to `proto/mediapipelinemodule.proto` with `session_id` (field 1) and `source_id` (field 2)

## 2. Public and IPC Client Interfaces

- [x] 2.1 Add `virtual void notifyFirstFrameReceived(int32_t sourceId) = 0` to `media/public/include/IMediaPipelineClient.h`
- [x] 2.2 Add `virtual void notifyFirstFrameReceived(int32_t sourceId) = 0` to `media/client/ipc/interface/IMediaPipelineIpcClient.h`
- [x] 2.3 Add `virtual void notifyFirstFrameReceived(int32_t sourceId) = 0` to `media/server/gstplayer/interface/IGstGenericPlayerClient.h`

## 3. Server Internal Interface

- [x] 3.1 Add `virtual void notifyFirstFrameReceived(int32_t sourceId) = 0` to `media/server/main/interface/IMediaPipelineServerInternal.h` (or equivalent server-side client interface)

## 4. Detection Utility

- [x] 4.1 Add `getFirstAudioFrameSignalName()` utility to `media/server/gstplayer/source/Utils.cpp` and declare it in `media/server/gstplayer/include/Utils.h`, searching for signals `first-audio-frame` and `first-audio-frame-callback` using the same pattern as `getUnderflowSignalName`

## 5. GStreamer Backend – IGstGenericPlayerPrivate

- [x] 5.1 Add `virtual void scheduleAudioFirstFrame() = 0` to `media/server/gstplayer/include/IGstGenericPlayerPrivate.h`
- [x] 5.2 Implement `GstGenericPlayer::scheduleAudioFirstFrame()` in `media/server/gstplayer/source/GstGenericPlayer.cpp` to enqueue a `FirstFrameReceived` task on the worker task queue

## 6. FirstFrameReceived Task

- [x] 6.1 Create `media/server/gstplayer/include/tasks/generic/FirstFrameReceived.h` declaring the task class
- [x] 6.2 Create `media/server/gstplayer/source/tasks/generic/FirstFrameReceived.cpp` implementing the task: read the audio source ID from `GenericPlayerContext`, check the deduplication guard flag, set the flag, call `IGstGenericPlayerClient::notifyFirstFrameReceived(sourceId)`
- [x] 6.3 Add a `firstAudioFrameReceived` boolean guard field to `StreamInfo` in `media/server/gstplayer/include/GenericPlayerContext.h` (or equivalent context struct), defaulting to `false`
- [x] 6.4 Register the new task in `media/server/gstplayer/source/tasks/generic/GenericPlayerTaskFactory.cpp` and update the factory header/interface

## 7. SetupElement – Signal and Probe Hookup

- [x] 7.1 Add a `firstAudioFrameCallback` static callback in `media/server/gstplayer/source/tasks/generic/SetupElement.cpp` that calls `player->scheduleAudioFirstFrame()`
- [x] 7.2 Add a pad probe callback `audioFirstFrameProbeCallback` in `SetupElement.cpp` that calls `player->scheduleAudioFirstFrame()` and returns `GST_PAD_PROBE_REMOVE`
- [x] 7.3 In `SetupElement::execute()`, after the existing underflow signal hookup block, add logic for audio sinks/decoders: call `getFirstAudioFrameSignalName()`; if a signal name is returned connect `firstAudioFrameCallback`; otherwise install `audioFirstFrameProbeCallback` as a buffer pad probe on the element's sink pad

## 8. Server Internal – MediaPipelineServerInternal

- [x] 8.1 Add `notifyFirstFrameReceived(int32_t sourceId)` override declaration to `media/server/main/include/MediaPipelineServerInternal.h`
- [x] 8.2 Implement `MediaPipelineServerInternal::notifyFirstFrameReceived(sourceId)` in `media/server/main/source/MediaPipelineServerInternal.cpp`: enqueue a main-thread task that calls `m_mediaPipelineClient->notifyFirstFrameReceived(sourceId)`

## 9. Server IPC – MediaPipelineClient

- [x] 9.1 Add `notifyFirstFrameReceived(int32_t sourceId)` override declaration to `media/server/ipc/include/MediaPipelineClient.h`
- [x] 9.2 Implement `server::ipc::MediaPipelineClient::notifyFirstFrameReceived(sourceId)` in `media/server/ipc/source/MediaPipelineClient.cpp`: construct and send a `FirstFrameReceivedEvent` with `session_id` and `source_id`

## 10. Client IPC – MediaPipelineIpc

- [x] 10.1 Add `onFirstFrameReceived` handler declaration to `media/client/ipc/include/MediaPipelineIpc.h`
- [x] 10.2 Register subscription to `FirstFrameReceivedEvent` in `MediaPipelineIpc` constructor / subscription setup (following the same pattern as `onBufferUnderflow`)
- [x] 10.3 Implement `MediaPipelineIpc::onFirstFrameReceived(event)` in `media/client/ipc/source/MediaPipelineIpc.cpp`: check `session_id`, call `m_mediaPipelineIpcClient->notifyFirstFrameReceived(source_id)`

## 11. Unit Tests – GstPlayer Layer

- [x] 11.1 Add unit tests for `getFirstAudioFrameSignalName()` in `tests/unittests/media/server/gstplayer/`: signal present returns name, signal absent returns nullopt
- [x] 11.2 Add unit tests for `SetupElement` covering: signal connected when `first-audio-frame` present, signal connected when `first-audio-frame-callback` present, probe installed when no signal present, no hookup for non-audio elements
- [x] 11.3 Add unit tests for `FirstFrameReceived` task: first call notifies client, second call is no-op (deduplication guard)
- [x] 11.4 Add unit tests for `GstGenericPlayer::scheduleAudioFirstFrame()` in `tests/unittests/media/server/gstplayer/genericPlayer/GstGenericPlayerPrivateTest.cpp`

## 12. Unit Tests – Server and IPC Layers

- [x] 12.1 Add unit tests for `MediaPipelineServerInternal::notifyFirstFrameReceived` verifying main-thread task is enqueued and client is notified
- [x] 12.2 Add unit tests for `server::ipc::MediaPipelineClient::notifyFirstFrameReceived` verifying `FirstFrameReceivedEvent` is sent with correct fields
- [x] 12.3 Add unit tests for `MediaPipelineIpc::onFirstFrameReceived`: matching session dispatches to client, non-matching session is ignored

## 13. Component Tests

- [x] 13.1 Add a component test for server-side first-frame propagation: simulate first-audio-frame signal from GStreamer stub, verify `FirstFrameReceivedEvent` is sent to client
- [x] 13.2 Add a component test for client-side first-frame reception: send `FirstFrameReceivedEvent` from stub, verify `IMediaPipelineClient::notifyFirstFrameReceived` is called with correct source ID

## 14. CMake and Build Integration

- [x] 14.1 Add `FirstFrameReceived.cpp` to the relevant CMakeLists for the gstplayer tasks target
- [x] 14.2 Verify all new test files are added to their respective CMakeLists targets
