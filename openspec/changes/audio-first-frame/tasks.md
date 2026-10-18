## 1. Gstplayer Detection Wiring

- [x] 1.1 Update signal lookup to include audio first-frame signal names in media/server/gstplayer/source/Utils.cpp.
- [x] 1.2 Add audio first-frame callback connection in media/server/gstplayer/source/tasks/generic/SetupElement.cpp.
- [x] 1.3 Add audio first-frame scheduler API in media/server/gstplayer/include/IGstGenericPlayerPrivate.h and media/server/gstplayer/include/GstGenericPlayer.h.
- [x] 1.4 Implement audio first-frame scheduling with MediaSourceType::AUDIO in media/server/gstplayer/source/GstGenericPlayer.cpp.

## 2. Audio Sink Fallback Probe

- [x] 2.1 Implement sink pad probe installation for audio sinks without callback capability in media/server/gstplayer/source/tasks/generic/SetupElement.cpp.
- [x] 2.2 Add probe state storage and lifecycle hooks in media/server/gstplayer/source/GstGenericPlayer.cpp and media/server/gstplayer/include/GstGenericPlayer.h.
- [x] 2.3 Remove probe on first trigger and terminal paths (flush/reset/stop/teardown) in media/server/gstplayer/source/GstGenericPlayer.cpp.

## 3. One-Shot Emission Guard

- [x] 3.1 Add a per-audio-source one-shot first-frame guard in media/server/gstplayer/source/GstGenericPlayer.cpp.
- [x] 3.2 Wire guard checks in both callback and probe paths in media/server/gstplayer/source/tasks/generic/SetupElement.cpp.
- [x] 3.3 Extend private-interface mocks for new audio scheduler hooks in tests/unittests/media/server/mocks/gstplayer/GstGenericPlayerPrivateMock.h.

## 4. End-to-End Notification Reuse

- [ ] 4.1 Validate audio source-id mapping path in media/server/main/source/MediaPipelineServerInternal.cpp (no API changes expected).
- [ ] 4.2 Validate server IPC event emission remains unchanged in media/server/ipc/source/MediaPipelineClient.cpp and proto/mediapipelinemodule.proto.
- [ ] 4.3 Validate client IPC and callback path remains unchanged in media/client/ipc/source/MediaPipelineIpc.cpp and media/client/main/source/MediaPipeline.cpp.

## 5. Test Coverage and Validation

- [x] 5.1 Add setup-task unit coverage for audio first-frame signal hookup in tests/unittests/media/server/gstplayer/genericPlayer/tasksTests/SetupElementTest.cpp and tests/unittests/media/server/gstplayer/genericPlayer/common/GenericTasksTestsBase.cpp.
- [x] 5.2 Add private-player unit coverage for audio first-frame scheduling in tests/unittests/media/server/gstplayer/genericPlayer/GstGenericPlayerPrivateTest.cpp.
- [x] 5.3 Add/update first-frame task behavior assertions in tests/unittests/media/server/gstplayer/genericPlayer/tasksTests/FirstFrameReceivedTest.cpp.
- [x] 5.4 Add/update server component first-frame flow for audio source in tests/componenttests/server/tests/mediaPipeline/FirstFrameNotificationTest.cpp.
- [x] 5.5 Add/update client component first-frame flow for audio source in tests/componenttests/client/tests/base/MediaPipelineTestMethods.cpp and tests/componenttests/client/tests/mse/FirstFrameNotificationTest.cpp.
- [ ] 5.6 Run openspec validate audio-first-frame --type change --json --no-interactive and execute affected unit/component test targets for non-regression.