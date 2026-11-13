# Project Context

## Purpose
Rialto is an RDK Central media playback client/server system. The client library exposes media-control APIs to applications and communicates over protobuf-based IPC with a separate Rialto server, which manages media pipelines and playback. This checkout also contains a standalone reference player under `tests/refplayer`.

The current integration goal is to exercise Rialto's process and playback-state metrics while a real reference-player workload is active. The metrics subsystem samples both the application/client and server, reports periodic and state-transition data, aggregates measurements by playback/application state, and emits reports through the server logging reporter by default.

## Tech Stack
- C++17 for Rialto and the reference-player library, CLI, and JavaScript binding.
- CMake for the main project and the independently configured reference player; Python scripts wrap Rialto unit/component-test builds.
- Protocol Buffers for client/server IPC, including the internal `PrivateMetricsModule` metrics channel.
- GoogleTest and GoogleMock for Rialto unit/component tests; the reference-player host test is a standalone C++ test executable registered with CTest.
- Linux process and resource interfaces (`/proc`, cgroups, Unix sockets), pthreads, and `ITimer` for periodic collection.
- GStreamer and RDK/WPE Framework integrations for media playback; JavaScriptCore and GLib are optional dependencies for the reference-player binding.
- [Not clearly defined in repository] There is no `package.json`, `requirements.txt`, or equivalent application package manifest. External build dependencies are supplied through CMake, platform/Yocto recipes, and test setup scripts.

## Project Conventions

### Code Style
- C++ uses the repository `.clang-format` configuration: LLVM-derived, Allman braces, four-space indentation, a 120-column limit, C++17, and right-aligned pointers. CI also runs clang-format, cpplint, cppcheck, and license checks.
- Use descriptive PascalCase type names and camelCase methods/variables, matching local interfaces and implementations. Keep public interfaces in `interface` or `public` areas and implementation headers/sources in `include` and `source` areas, following the owning component's existing layout.
- C++ files generally include the project license header and use explicit namespaces. Follow neighboring files for include ordering, test fixture names, and mock conventions.
- Python build/test scripts use four-space indentation and camelCase names; follow the existing scripts when editing them.

### Architecture Patterns
- Rialto is divided into client/server components and commonly uses three server layers: `ipc` handles protobuf transport, `service` routes requests and lifecycle events, and `main` owns media/metrics business logic.
- Client and server communicate using protobuf RPC over Unix sockets. Metrics use a separate internal `PrivateMetricsModule`: the server requests client samples and associates each response with a client ID, sample ID, and reason.
- `MetricsCollector` is created per connected client. It samples server-side process/cgroup data, calculates CPU percentages from successive cumulative CPU-time samples, feeds online accumulators for playback/application states, checks thresholds, and reports via `IMetricsReporter`.
- The reference player is independent of the top-level Rialto build. Its public `IRefPlayer` API fronts a command worker, state machine, event bus, Rialto session, source manager, and media feeder. The CLI and optional JavaScriptCore binding are front ends over this library.
- Keep IPC transport concerns out of metrics calculation and keep test-only mock behavior separate from real-client target behavior.

### Testing Strategy
- Rialto unit suites are selected and built with `build_ut.py`; component suites use `build_ct.py`. These scripts configure CMake, build GoogleTest targets, run them, and optionally generate coverage, XML, and Valgrind results. Relevant metrics suites include server-main metrics tests, server-service metrics tests, and client metrics component tests.
- Metrics unit/component tests currently use timer and IPC mocks to verify request sequencing, sample processing, state boundaries, reporting, and threshold behavior.
- The reference-player host test is separate: configure from `tests/refplayer` with `REFERENCE_PLAYER_USE_MOCKS=ON` and `REFERENCE_PLAYER_BUILD_TESTS=ON`, then build and run CTest. It validates player lifecycle against a scripted mock pipeline; it does not exercise Rialto metrics or require a device/media file.
- For metrics integration, use the reference player with the real Rialto client/server on a supported Linux target. Make the server-manager application ACTIVE, run a playback workload long enough to cross the 15-second periodic interval, then cause playback state transitions and inspect server logs for the connected baseline, periodic samples, and state reports. Treat CPU/memory values as environment-dependent; assert sample/report presence, reason, and lifecycle behavior rather than fixed resource numbers.
- The repository does not currently define a combined reference-player/metrics end-to-end test target or CI gate. Add integration coverage without treating a mock-mode reference-player run as proof of the metrics IPC path.

### Git Workflow
- Contributions are made through GitHub forks and pull requests; the RDK Contributor License Agreement is required before acceptance.
- CI expects a pull request title of the form `<TICKET>: <summary>` with a ticket matching the configured Jira-style pattern and a summary under 65 characters. The body starts with the ticket and summary, includes `Reason for change:`, and may include `Test Procedure:`. Include the test procedure for behavior changes.
- Pull requests target `master` or `release/*` in the documented workflows. A broader branch naming strategy is [Not clearly defined in repository].
- The README points to an external RDK commit-message template; details beyond the CI pull-request format are [Not clearly defined in repository].

## Domain Context
- The client process runs inside the application; the Rialto server is a separate renderer process. Metrics must therefore collect client-side data in the client and send it to the server rather than assuming the server can inspect the client's `/proc` entries itself.
- A client connection produces a `CONNECTED` baseline. The server then requests samples periodically (15 seconds) and at playback/application state boundaries. CPU percentages are computed from deltas, so the baseline itself has no percentage and state-transition samples with very short elapsed time are not used in state aggregations.
- Metrics include client, server, and combined CPU; client/server resident memory; and cgroup memory usage/limit. On Linux, process data comes from `/proc`, and cgroup memory is resolved from the process cgroup. A cgroup limit reported as unlimited is represented as zero.
- Metrics reports and alerts are emitted through `IMetricsReporter`; the default `LogMetricsReporter` writes to the Rialto server log. State reports summarize samples accumulated during a playback or application state.
- `tests/refplayer` currently supports phase-1 local MP4 playback (H.264 video, optional AAC audio) through Rialto's public client API. The target application must be ACTIVE before it can create a media pipeline. Its mock host test cannot confirm real IPC, server metrics sampling, or target GStreamer behavior.

## Important Constraints
- Rialto requires a Linux-like target with Unix sockets, `/proc`, and cgroup interfaces for the documented metrics collection. Cgroup v1/v2 layouts and resource availability can vary by device.
- The 15-second sampling cadence means a short playback test may produce no periodic sample. Ensure integration playback lasts beyond one full interval, and allow for startup and asynchronous IPC timing.
- CPU percentages are per one core and can exceed 100% on multicore systems. Runtime load, target hardware, and cgroup configuration make exact metrics values unsuitable as portable fixed assertions.
- The reference-player CLI is a debugging aid, not the acceptance gate. The host test requires mock mode; real-client playback and metrics validation require a correctly built target environment, server, manager state, socket configuration, and compatible media.
- Reference-player phase 1 does not implement all declared API features; unsupported operations return `NotSupported`. Do not build an integration scenario around unsupported behavior.
- [Not clearly defined in repository] No regulatory, privacy, retention, or service-level requirements for metrics are specified here.

## External Dependencies
- Protocol Buffers compiler/runtime and `Threads` are explicit top-level CMake dependencies. Other Rialto components locate dependencies through CMake modules, including JsonCpp and Mongoose.
- Rialto's target environment supplies GStreamer and relevant RDK/WPE Framework, OpenCDM, and platform-specific integrations; exact versions are determined by the target Yocto/platform recipe rather than a repository-wide lockfile.
- GoogleTest/GoogleMock and the repository test/build scripts are used for unit and component testing.
- The reference player links the installed `libRialtoClient` and public Rialto headers for real target use. Optional JavaScript binding builds need JavaScriptCore and GLib, with the JavaScriptCore pkg-config package configurable for the platform.
- The reference player depends on an external server manager and Rialto server at runtime. Its README documents `RialtoServerManagerSim`, the manager `SetState/.../Active` endpoint, and `RIALTO_SOCKET_PATH` setup for local/target runs.

## Additional Notes
- Metrics design and field semantics: `docs/MetricsDesign.md`; example production log/report interpretation: `docs/metrics/RialtoMetricsReport.md`.
- Reference-player build, target setup, CLI options, and limitations: `tests/refplayer/README.md`.
- A practical integration scenario is: start the server and manager, activate the reference-player application, point `RIALTO_SOCKET_PATH` at the server socket, run a supported local MP4 for longer than 15 seconds, pause/resume or stop to create state boundaries, and verify corresponding metrics entries in the server log.
- Build-system entry points include `build_ut.py`, `build_ct.py`, and `tests/refplayer/CMakeLists.txt`. The top-level build does not automatically include the standalone reference-player subproject.