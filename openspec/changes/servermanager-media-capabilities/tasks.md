## 0. Common — move capability structs

- [X] 0.1 Copy `AudioDecoderCapabilities.h` to `common/public/include/`; update `common/public/CMakeLists.txt` to expose the source include directory via `RialtoCommonPublic` interface.
- [X] 0.2 Copy `VideoDecoderCapabilities.h` to `common/public/include/`; same CMakeLists update.
- [X] 0.3 In File `AudioDecoderCapabilities.h` and `VideoDecoderCapabilities.h`change `namespace firebolt::rialto` to `namespace firebolt::rialto::common`
- [X] 0.4 Update all includes of both headers in `media/`, `wrappers/`, and `tests/` to resolve through `RialtoCommonPublic`. Duplicate copies under `media/public/include/` removed; `media/public/CMakeLists.txt` now exposes `RialtoCommonPublic` include dirs so all `media/` consumers resolve the single `common/public/include/` copy.
- [X] 0.5 Verify `IYamlCppWrapper.h` resolves both headers from the new location.

## 1. Proto — typed `AudioCapabilities` / `VideoCapabilities` in `SetConfigurationRequest`

- [X] 1.1 Define `message AudioCapabilities` and `message VideoCapabilities` in `proto/servermanagermodule.proto` (or extract to a new `proto/mediacapabilities.proto` imported by both `servermanagermodule.proto` and `mediapipelinecapabilitiesmodule.proto`). Structures mirror `GetSupportedAudioCapabilitiesResponse` / `GetSupportedVideoCapabilitiesResponse`.
- [X] 1.2 Add `optional AudioCapabilities audioCapabilities = 12` and `optional VideoCapabilities videoCapabilities = 13` to `SetConfigurationRequest` in `proto/servermanagermodule.proto` only (symlinks update automatically).
- [ ] 1.3 If a shared proto is added, update `proto/CMakeLists.txt` and both consumer proto files with the import.
- [ ] 1.4 Verify all symlink-resolved copies compile consistently.

## 2. ServerManager — `IMediaCapabilities` interface and implementation

- [X] 2.1 Add `serverManager/public/include/IMediaCapabilities.h`: interface `IMediaCapabilities` in namespace `rialto::servermanager::service`; `getAudioDecoderCapabilities(AudioDecoderCapabilities&)` and `getVideoDecoderCapabilities(VideoDecoderCapabilities&)` returning `DecoderCapabilitiesStatus`.
- [X] 2.2 Add `serverManager/public/include/MediaCapabilitiesFactory.h`: factory `createMediaCapabilities()` returning `std::unique_ptr<IMediaCapabilities>`.
- [X] 2.3 Add `serverManager/service/include/MediaCapabilities.h` and `serverManager/service/source/MediaCapabilities.cpp`: delegates to `IYamlCppWrapper`; `DEBUG` on entry, `INFO` on `OK`, `INFO` on `CONFIG_NOT_FOUND`, `WARN` on schema/internal errors.
- [X] 2.4 Add `serverManager/service/source/MediaCapabilitiesFactory.cpp`: resolves `IYamlCppWrapperFactory::getFactory()`.
- [X] 2.5 Update `serverManager/service/CMakeLists.txt`: build new files, link `RialtoWrappers`.

## 3. ServerManager common — capabilities in `SessionServerAppManager`

- [X] 3.1 Update `serverManager/common/source/SessionServerAppManager.h`: add `std::shared_ptr<service::IMediaCapabilities>`, `std::optional<AudioDecoderCapabilities>`, `std::optional<VideoDecoderCapabilities>` members; update constructor signature.
- [X] 3.2 Update `serverManager/common/source/SessionServerAppManager.cpp` constructor: call `getAudioDecoderCapabilities()` / `getVideoDecoderCapabilities()`; populate optionals on `OK`; `INFO` on `CONFIG_NOT_FOUND`, `WARN` on other errors; `std::nullopt` on failure.
- [X] 3.3 Update both `performSetConfiguration` call sites in `SessionServerAppManager.cpp` to forward `m_audioCapabilities` and `m_videoCapabilities`.
- [X] 3.4 Update `serverManager/common/include/SessionServerAppManagerFactory.h` and `serverManager/common/source/SessionServerAppManagerFactory.cpp`: thread `IMediaCapabilities` to `SessionServerAppManager`.
- [X] 3.5 Update `serverManager/service/source/ServiceContext.cpp`: call `createMediaCapabilities()` and pass to `createSessionServerAppManager()`.

## 4. ServerManager IPC — `CapabilitySerialiser` and updated `IController` / `Client`

- [X] 4.1 Add `serverManager/ipc/source/CapabilitySerialiser.h/.cpp` (namespace `rialto::servermanager::ipc`): `serialiseAudioCapabilities()` and `serialiseVideoCapabilities()` converting C++ structs → typed proto `AudioCapabilities` / `VideoCapabilities`. Does NOT reuse `firebolt::rialto::client` converters.
- [X] 4.2 Update `serverManager/ipc/include/IController.h` both `performSetConfiguration()` overloads: add `const std::optional<AudioDecoderCapabilities>&` and `const std::optional<VideoDecoderCapabilities>&` parameters.
- [X] 4.3 Update `serverManager/ipc/source/Controller.h` / `Controller.cpp`: match new signatures; forward optional parameters to `Client`.
- [X] 4.4 Update `serverManager/ipc/include/ControllerFactory.h` and `serverManager/ipc/source/ControllerFactory.cpp`: remove any `IMediaCapabilities` constructor dependency (capabilities are per-call parameters now).
- [X] 4.5 Update `serverManager/ipc/source/Client.h` / `Client.cpp` both overloads: accept optional parameters; if both have values populate `request.mutable_audiocapabilities()` / `request.mutable_videocapabilities()` via `CapabilitySerialiser`; if `std::nullopt` leave fields absent and emit `DEBUG`.
- [X] 4.6 Update `serverManager/ipc/CMakeLists.txt`: add `CapabilitySerialiser.cpp`, link `RialtoWrappers` and `RialtoServerManagerService`.

## 5. Session Server — GstCapabilities two-path model

- [X] 5.1 Extend `configureServices()` in `media/server/service/include/ISessionServerManager.h` and `media/server/service/source/SessionServerManager.h/.cpp`: add `const std::optional<AudioDecoderCapabilities>&` and `const std::optional<VideoDecoderCapabilities>&` parameters; store for `GstCapabilities`.
- [X] 5.2 Update `media/server/ipc/source/ServerManagerModuleService.cpp` `setConfiguration()`: if typed capability fields present, deserialise into C++ structs using generated proto accessors and pass as `std::optional` to `configureServices()`; if absent pass `std::nullopt` and emit `INFO`.
- [X] 5.3 Update `media/server/gstplayer/include/GstCapabilities.h`: add optional pre-loaded capability parameters to constructor.
- [X] 5.4 Update `media/server/gstplayer/source/GstCapabilities.cpp` — **Path A**: when both pre-loaded optionals are present, use them directly and skip `IYamlCppWrapper` YAML load. **Path B**: when either optional is absent, run the existing GStreamer element-query mechanism (the legacy `IMediaPipelineCapabilities` logic) to populate capabilities — this path is **retained**, not removed.
- [X] 5.5 Update `GstCapabilitiesFactory::createGstCapabilities()`: obtain pre-loaded optionals from the session server manager and pass them to the `GstCapabilities` constructor.

## 6. Client Library — `IMediaCapabilities` replaces capability methods

- [X] 6.1 Add `media/public/include/IMediaCapabilities.h`: declares `getSupportedAudioCapabilities()` → `AudioDecoderCapabilities` and `getSupportedVideoCapabilities()` → `VideoDecoderCapabilities` in namespace `firebolt::rialto`.
- [X] 6.2 Remove `getSupportedAudioCapabilities()` and `getSupportedVideoCapabilities()` from `media/public/include/IMediaPipelineCapabilities.h` and all implementation files. No side-by-side support.
- [X] 6.3 Add `media/client/ipc/include/IMediaCapabilitiesIpc.h` and `media/client/ipc/source/MediaCapabilitiesIpc.cpp` (namespace `firebolt::rialto::client`): IPC caller invoking `MediaPipelineCapabilitiesModule` service; deserialises response into `AudioDecoderCapabilities` / `VideoDecoderCapabilities`.
- [X] 6.4 Add factory wiring in `media/client/main/` to expose `IMediaCapabilities` via the existing factory pattern: new `media/client/ipc/interface/IMediaCapabilitiesIpcFactory.h` + `MediaCapabilitiesIpcFactory` (ipc layer), and `media/client/main/include/MediaCapabilities.h` + `source/MediaCapabilities.cpp` implementing `IMediaCapabilitiesFactory::createFactory()` and the `client::MediaCapabilities` wrapper, mirroring the `MediaPipelineCapabilities` pattern. Registered in `media/client/main/CMakeLists.txt`.
- [ ] 6.5 Update `rialtomse*` sink components and all other consumers of the removed `IMediaPipelineCapabilities` capability methods to call `IMediaCapabilities` instead.

## 7. Build Wiring

- [X] 7.1 Update `serverManager/CMakeLists.txt`: link `serverManager/ipc/` against `RialtoWrappers` and `RialtoServerManagerService`.
- [X] 7.2 Update `media/client/ipc/CMakeLists.txt`: add `MediaCapabilitiesIpc.cpp`.
- [X] 7.3 Update `proto/CMakeLists.txt` if a shared capabilities proto is introduced.

## 8. Unit Tests

- [X] 8.1 Add `tests/unittests/serverManager/MediaCapabilitiesTest.cpp`: `OK`, `CONFIG_NOT_FOUND`, `SCHEMA_VALIDATION_FAILED`, `INTERNAL_ERROR`; inject mock `IYamlCppWrapper`.
- [X] 8.2 Update `tests/unittests/serverManager/SessionServerAppManagerTest.cpp`: optionals populated and forwarded on `OK`; `std::nullopt` forwarded on non-`OK`.
- [X] 8.3 Add or update `tests/unittests/serverManager/ClientTest.cpp`: typed proto fields set when optionals have values; fields absent when `std::nullopt`.
- [X] 8.4 Update `tests/unittests/media/server/ipc/serverManagerModuleService/ServerManagerModuleServiceTestsFixture.cpp`: `configureServices()` called with correct optionals when typed fields present; `std::nullopt` when absent.
- [X] 8.5 Update `tests/unittests/media/server/gstplayer/GstCapabilitiesTest.cpp`: Path A — pre-loaded data used, neither YAML load nor GStreamer query invoked; Path B — no pre-loaded data, GStreamer element query runs.
- [X] 8.6 Add `tests/unittests/media/client/MediaCapabilitiesIpcTest.cpp`: successful audio/video deserialisation; RPC failure path.
- [X] 8.7 Register all new test files in the relevant `CMakeLists.txt` targets.

## 9. Validation

- [ ] 9.1 Build `serverManager`, `media/server`, `media/client`, `proto`, and `tests/unittests` with no new warnings or errors.
- [ ] 9.2 Run `tests/unittests/serverManager` — `MediaCapabilitiesTest` passes.
- [ ] 9.3 Run `tests/unittests/media/server/ipc/serverManagerModuleService` — `ServerManagerModuleServiceTest` passes.
- [ ] 9.4 Run `tests/unittests/media/server/gstplayer` — `GstCapabilitiesTest` Path A and Path B pass.
- [ ] 9.5 Run `tests/unittests/media/client` — `MediaCapabilitiesIpcTest` passes.
- [ ] 9.6 Verify no regression in existing `ConfigReaderTest`, `MediaPipelineCapabilitiesIpcTest`, and related targets.
- [ ] 9.7 Confirm cpplint, clang-format, and cppcheck pass on all new and modified files.
