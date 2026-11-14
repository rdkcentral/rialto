# Rialto Reference Player

A playback library over the public Rialto client API (`IControl`, `IMediaPipeline`), with an AAMP-compatible JavaScript binding for the WPE WebProcess and a command-line front-end for debugging outside a browser.

It was built to the phase-1 component design, `REFERENCE_PLAYER_COMPONENT_DESIGN.md` one level up. Phase 1 plays local H.264/AAC MP4 files over MSE. Phases 2 to 5 are declared in the API and answer `NotSupported` until they are implemented.

## Layout

| Path | Contents |
|------|----------|
| `include/rialto_refplayer/` | Public library API: `IRefPlayer`, `RefPlayerTypes.h` (`PlayerState`, `Result`, `Options`), `RefPlayerEvent.h` (`Event`, `IRefPlayerClient`) |
| `src/core/` | `libRialtoRefPlayer` internals: `RefPlayer` (command validation), `CommandWorker`, `PlayerStateMachine`, `EventBus`, `PositionTracker`, `RialtoSession`, `SourceManager`, `MediaFeeder`, `PipelineCallbacks`, `Logger` |
| `src/demux/` | `IDemuxer` seam, `TrackModel`, `DemuxerRegistry.cpp` (MP4 adapter), `Mp4Demuxer.{h,cpp}` (parser, unchanged) |
| `src/frontend/cli/` | `rialto_reference_player`: option parsing and driver (`main.cpp`), probe checks (`ContractValidator`), negative API suite (`NegativeChecks`), and the log helper (`CliLog.h`) |
| `src/frontend/jsbinding/` | `libRialtoPlayerJSBindings.so`: JavaScriptCore class `RialtoMediaPlayer`, exports `RialtoPlayer_LoadJS` and `RialtoPlayer_UnloadJS` only |
| `test/mocks/rialto/` | Mock Rialto headers for building without the client (mock mode) |
| `test/mocks/JavaScriptCore/`, `test/mocks/glib/`, `test/mocks/stubs/` | Compile-time JavaScriptCore and GLib declarations and empty stub definitions, for checking the binding without the WebKit SDK. Not part of the recipe |
| `test/unit/RefPlayerTest.cpp` | Host test for the library against a scripted mock pipeline |

## Threading and lifetime rules

- **Control calls** (`load`, `play`, `pause`, `stop`, `seek`) validate what they can, enqueue on the command worker, and return. Completion and failure are events.
- **Getters** (`positionMs`, `durationMs`, `state`) read cached atomics and never touch Rialto.
- **State** moves only from Rialto callbacks, except for the states the worker owns (INITIALIZING, PREPARING, PREPARED, STOPPING, RELEASED, IDLE). Callbacks are dropped while INITIALIZING, so a stale PLAYING from the pipeline being replaced cannot land.
- **Teardown order** is fixed: join the feeder, remove the sources, release the pipeline, and keep the control registration.
- **JavaScriptCore** is touched only on the JS thread. Library events hop there with `g_idle_add_full`. Finalize only reads the private pointer.

## How to build

### Yocto (target build)

The recipe is `rialto-reference-player_1.0.bb`. It needs the `enable_rialto` distro feature and links `libRialtoClient`. The JavaScript binding is built with `PACKAGECONFIG = "js-binding"`, which adds `wpe-webkit` to the build dependencies and packages the binding as `${PN}-jsbindings`.

### CMake

```bash
cmake -S . -B build -DRIALTO_INCLUDE_DIR=/usr/include/rialto -DRIALTO_CLIENT_LIB=/usr/lib/libRialtoClient.so
cmake --build build
```

| Option | Default | Meaning |
|--------|---------|---------|
| `REFERENCE_PLAYER_USE_MOCKS` | OFF | Build against `test/mocks/rialto` instead of the installed client. Also selected automatically when the Rialto headers or library are missing |
| `REFERENCE_PLAYER_BUILD_JS_BINDING` | OFF | Build `libRialtoPlayerJSBindings.so`. Needs JavaScriptCore and GLib through pkg-config |
| `REFERENCE_PLAYER_JSC_PKG` | `javascriptcoregtk-4.1` | pkg-config name of the JavaScriptCore package. It is platform-dependent. The Yocto recipe sets `wpe-webkit-1.1` for the WPE target |
| `REFERENCE_PLAYER_BUILD_TESTS` | OFF | Build the host test. Requires mock mode |

`REFERENCE_PLAYER_ENABLE_LOCAL_DEMUXER` is retired. The MP4 demuxer is always built.

## Host test

The host test drives the library against the scripted mock pipeline, which answers each call as a server would, on its own thread. It needs no device and no MP4 file, because a fake demuxer supplies the track model.

```bash
cmake -S . -B build-test -DREFERENCE_PLAYER_USE_MOCKS=ON -DREFERENCE_PLAYER_BUILD_TESTS=ON
cmake --build build-test && ctest --test-dir build-test --output-on-failure
```

It covers play, pause, seek and stop with release, end of stream, reload without stop, an unsupported source, illegal commands, destroy during playback, rapid load and stop, and stop immediately followed by load. It has been run under AddressSanitizer, UBSan and ThreadSanitizer.

The mock headers mirror only the subset of the Rialto v0.25.0 public headers that the code calls. Check them against the real headers before trusting a mock-mode run.

## Running on target

The client connects to the Rialto server through `RIALTO_SOCKET_PATH`. The application must be ACTIVE in the server manager before a media pipeline can be created:

```bash
RialtoServerManagerSim &
curl -X POST -d "refplayer" 127.0.0.1:9008/SetState/RefPlayer/Active
export RIALTO_SOCKET_PATH=/tmp/refplayer
```

The library's `initialize()` fails with `ApplicationNotRunning` when the application is not RUNNING. That is a distinct result, so the failure is not mistaken for a Rialto error.

## Logging

Lines are `[rialto-ref-player] HH:MM:SS.mmm LEVEL tid=<id> message`. The sink is stdout, which the test app reads from `/package/logs.txt`. Debug detail is enabled with `Options::verboseLogging`.

The tune-time marker is printed once per load, at the first frame:

```
[rialto-ref-player] TUNE_TIME_MS=<n>
```

Its format is a consumed interface for the test app's tune-time panel. Change it only together with `testutils.js`.

## Command-line front-end

`rialto_reference_player` drives the library through the public API, with bounded waits, and runs the probe checks and the negative suite. It is a debugging aid, not the acceptance gate.

For per-scenario stimulus details and expected metric behavior, see the [fault-injection guide](guide_to_faultinjection.md).

```bash
rialto_reference_player --url file:///media/clip.mp4 --set-volume 0.5 --seek 2000000000
rialto_reference_player --url file:///media/clip.mp4 --inject-fault VIDEO_FRAME_REPEAT --expect-metric VIDEO_FRAME_REPEAT
rialto_reference_player --negative-runs            # contract checks only, no player
```

| Option | Default | Meaning |
|--------|---------|---------|
| `--url <url>` | none | Media URL. A local `file://` `.mp4` (H.264, optional AAC) is demuxed and fed over MSE. Without it, only the contract checks run |
| `--mime <mime>` | `video/mp4` | MIME type for the capability probes. Phase 1 plays only `video/mp4`, so playback ignores this value |
| `--live` | off | CLI only: do not wait for end of stream after PLAYING. The library loads as non-live in phase 1 |
| `--web-audio` | off | Create an `IWebAudioPlayer` and validate the PCM API |
| `--negative-runs` | off | Run the negative API suite after the contract checks. Real client only; logs a skip in mock mode |
| `--set-volume <v>` | unset | Volume in `[0.0, 1.0]`. Set and read back on a probe pipeline; the player's own `setVolume` is NotSupported in phase 1. Without it, 0.7 is used on the probe |
| `--seek <ns>` | unset | Seek to a position in nanoseconds. Needs `--url`; cannot be combined with `--no-playback` |
| `--expect-metric <type>` | unset | Require a matching metric callback during playback. Used with `--url` |
| `--inject-fault <type>` | unset | Apply a bounded feeder stimulus during active playback. Requires `--url`, `--expect-metric`, and playback enabled. Supported types: `VIDEO_FRAME_DROP`, `VIDEO_DECODE_ERROR`, `VIDEO_FRAME_REPEAT`, `BUFFER_UNDERFLOW` |
| `--priority <n>` | `1` | Audio player priority, used with `--web-audio` |
| `--no-playback` | off | Stop at PREPARED instead of PLAYING; no play() |
| `--no-validate-capabilities` | off | Skip the capability probe |
| `--timeout-ms <ms>` | `30000` | Ceiling for each wait on a player state, and for each library call that could block. Playback completion adds the media's own duration |
| `--help` | | Print usage |

Exit status is 0 when every step passes and 1 otherwise, including bad arguments and timeouts. A library call that does not return within its ceiling prints a line and exits with status 1 without unwinding, because the stalled call still holds library state.

Fault stimuli alter media delivery but do not fabricate metric events. They do not guarantee that the target decoder/renderer or Rialto server supports or reports the requested metric; the CLI succeeds only when the requested `--expect-metric` callback is actually received. A deliberately mismatched expected type is accepted as a negative test and fails if that type is not emitted.

### Real-target metric runs

After starting the Rialto server and activating the `RefPlayer` application as described above, set `RIALTO_SOCKET_PATH` and run the integration helper with a target-qualified H.264 MP4 containing at least 16 video samples:

```bash
sh ./tests/refplayer/run_fault_metric_tests.sh file:///media/metric-test.mp4
```

The helper runs the four matching `--inject-fault` / `--expect-metric` pairs against the real client and server, prints each CLI callback payload, continues after individual failures, and exits nonzero if any expected callback is absent. Set `RIALTO_REFERENCE_PLAYER` to use a non-default binary path and `REFPLAYER_METRIC_TIMEOUT_MS` to override the per-run metric wait. Target support remains platform-dependent; a stimulus alone does not establish that the metric was emitted.

## Negative checks

The negative API suite runs only against the real client, and only with `--negative-runs`. It is skipped in mock mode. The cases are documented in [RIALTO_NEGATIVE_CASES.md](../RIALTO_NEGATIVE_CASES.md).

## Open items

- The JavaScript binding is compiled and its exports are checked, but it has not run in a WebProcess. The JavaScriptCore signatures were transcribed from the WebKit C API and need checking against the real headers on target.
- The `UnloadJS` path releases the JSC class while live objects may still reference it. Confirm on target.
- `speedsChanged` uses `playbackSpeeds` as its field name, from AAMP's legacy binding. `httpResponseHeader` carries only `header`, not AAMP's `response`. Both are phase-2 and phase-3 decisions.
- `initialize()` and `destroy()` have no bound inside the library. `destroy()` joins the command worker, which can be inside a Rialto IPC call. The CLI puts a ceiling on both; the browser does not, so a stuck server would block the GC thread that runs finalize.
- `load()` takes no MIME type or live flag. The library plays only non-live `video/mp4` in phase 1.
- The contract checks register a Rialto client-log handler. That can displace the library's client-log forwarding for the rest of the process.
- The injected-bundle hook that calls `RialtoPlayer_LoadJS` is owned by the WPE/browser team and is not in this recipe.
