# SME Notes: GstPlayer

## Operations

### Deployment
How to deploy this system.
- Pre-deployment checklist
  - Confirm GStreamer 1.0 runtime libraries are installed on target: `libgstreamer-1.0.so`, `libgst-app-1.0.so`, `libgst-base-1.0.so`, `libgst-audio-1.0.so`, `libgst-pbutils-1.0.so`.
  - Confirm platform-specific decoder plugins are registered in the GStreamer plugin registry: `omxh265dec`, `omxeac3dec`, `westerossink` (or platform equivalents). Verify with `gst-inspect-1.0`.
  - Confirm OpenCDM decryptor plugin is present and loadable: `gst-inspect-1.0 | grep -i crypt`.
  - Confirm `librdk-gstreamer-utils` is installed if RDK platform helpers are needed.
  - Confirm yaml-cpp is installed if YAML decoder capability config files are expected. Verify config files exist and are readable before first playback.
  - Confirm `GstRialtoSrc` registers successfully as `rialtosrc` with rank `GST_RANK_PRIMARY + 100` — visible in GStreamer debug log on server startup.
- Deployment commands
  - Build gstplayer library:
    - `cmake --build build --target RialtoServerGstPlayer`
  - Build full server (gstplayer is linked statically into `RialtoServer`):
    - `cmake --build build --target RialtoServer`
  - Install:
    - `cmake --install build`
  - Enable GStreamer debug at runtime for pipeline diagnostics:
    - `GST_DEBUG=3 ./RialtoServer` for warnings and above.
    - `GST_DEBUG=rialtosrc:5,rialtoserver:5 ./RialtoServer` for focused source/server traces.
    - `GST_DEBUG_DUMP_DOT_DIR=/tmp/dots ./RialtoServer` to capture pipeline `.dot` graphs on state changes (triggered automatically by `HandleBusMessage` task on EOS and ERROR).
- Post-deployment verification
  - Verify `rialtosrc` appears in `gst-inspect-1.0` output after `RialtoServer` first initializes.
  - Verify at least one audio and one video MIME type are returned by `getSupportedMimeTypes` — if both return empty the GStreamer plugin registry is broken or incomplete.
  - Confirm `GstCapabilities` initialization completes within expected startup time; if playback requests arrive before `gst_init` completes they will block on a condition variable until it finishes.
  - For EME streams, verify decryptor element appears in a pipeline `.dot` dump after `AttachSource` for a DRM-protected track.

### Monitoring
What to monitor and where.
- Key metrics
  - `RIALTO_SERVER_LOG_WARN("Audio stream underflow!")` rate — this fires inside `CheckAudioUnderflow.cpp` when the pipeline clock is more than 350 ms ahead of `lastAudioSampleTimestamps`. More than 1 per second sustained is a critical signal that the audio pipeline has stalled.
  - VDEC gap warnings from hardware decoder: `get over 300ms gap(Xms)` in RialtoServer logs — a per-frame warning emitted when each decoded frame is more than 300 ms late relative to the previous one.
  - `RialtoServer` VIRT memory growth in `top` / `ps` — the session-server virtual address space should remain stable in steady-state playback; unbounded growth indicates shared memory segments are not being unmapped after use.
  - Pipeline state change failure: `GST_WARNING("Failed to set pipeline to READY state")` — reported in `GstGenericPlayer.cpp` during construction; treat as a hard failure.
  - `GST_DEBUG_BIN_TO_DOT_FILE` dumps are written to `GST_DEBUG_DUMP_DOT_DIR` on EOS and ERROR; presence of unexpected `.dot` files in that directory signals abnormal pipeline termination.
  - Decryption failures: `GST_WARNING_OBJECT(self, "HDCP output protection failure")` from `GstDecryptor.cpp` — indicates DRM key or output protection issue.
  - Unhandled cipher modes: `GST_WARNING_OBJECT(self, "Untested cipher mode")` from `GstDecryptor.cpp` — stream uses a cipher mode not exercised in testing.
- Dashboard locations
  - No built-in dashboard; correlate `RialtoServer[<pid>]` log lines in the platform syslog or `sky-messages.log`.
  - Use `top` or `ps` to monitor VIRT/RSS of specific `RialtoServer` PIDs across playback sessions.
  - GStreamer `.dot` files (when `GST_DEBUG_DUMP_DOT_DIR` is set) provide visual pipeline snapshots for post-hoc analysis.
- Alert thresholds
  - Audio underflow rate ≥ 1/second for more than 10 seconds: critical — audio pipeline stalled, SIGABRT likely imminent from WPE watchdog.
  - `lastAudioSampleTimestamps` frozen (not advancing across consecutive underflow log lines): critical — audio decoder is not consuming new data.
  - RialtoServer VIRT > 400 MB for a single session (baseline ~75 MB): warning — shared memory not being unmapped between playback transitions.
  - VDEC gap > 300 ms on every frame: warning — hardware decoder consistently behind pipeline clock; investigate clock vs decoder scheduling pressure.
  - Decryptor element creation failure: critical — encrypted playback will silently produce corrupt frames or black video.

### Common Issues
Problems that happen regularly.
- Symptom → Cause → Fix
  - Video goes black after starting a second concurrent playback → `rtkv1sink` initialization side-effect (`GstCapabilities.cpp` WORKAROUND comment at `fillSupportedMimeTypes`) → `rtkv1sink` must not be initialized during capability queries on platforms where it causes a side effect on an active video plane. The `fillSupportedMimeTypes` function skips initializing `rtkv1sink` for this reason. If black video reappears, check whether a new platform sink has the same issue.
  - H.264 or H.265 stream does not play with no caps error → Missing `stream-format` field in GstCaps when source has neither `stream-format` nor `codec_data` → `GstSrc::setDefaultStreamFormatIfNeeded` sets `stream-format=byte-stream` as a fallback. If the stream still fails, verify the source attaches `codec_data` via `AttachSource` / `CapsBuilder`.
  - `getSupportedMimeTypes` returns empty → GStreamer not yet initialized or plugin registry empty → `GstCapabilities` init is async; caller blocks on a condition variable until ready. If it never unblocks, `gst_init` failed — check `RIALTO_SERVER_LOG_ERROR("Failed to create the gstreamer capabilities")` at startup.
  - Pipeline stuck in PAUSED, never transitions to PLAYING → `FlushOnPrerollController` waiting with no matching state signal → If a `Flush` was enqueued during pipeline preroll but the GStreamer pipeline did not reach PAUSED before the flush resolved, the controller may wait indefinitely. Check `GstDispatcherThread` for `GST_STATE_CHANGE_PAUSED` message. See gstreamer/gstreamer#150.
  - Audio cuts during playback rate change (trick play) → Broadcom decoder bug with audio output at non-1.0 rates → Workaround is present in `GenericPlayerContext` (`pendingPlaybackRate` held and applied only after pipeline is ready). If audio cuts return after a rate-change, verify the rate-change task sequence is not racing with a pipeline state change.

---

## Tribal Knowledge

### Gotchas
Things that surprise people.
- `CheckAudioUnderflow` fires on a **timer**, not on a GStreamer bus message. The threshold is hardcoded at `350 ms` (`kAudioUnderflowMarginNs = 350 * 1000000` in `CheckAudioUnderflow.cpp`). You cannot tune it via config without a code change.
- `GstRialtoSrc` is a **singleton** (`GstSrcFactory::m_gstSrc` is a `std::weak_ptr` protected by a mutex). All `GstGenericPlayer` instances share one `GstRialtoSrc` object. Destroying one player while another is active does not destroy `GstRialtoSrc`; it is only released when the last `shared_ptr` holder drops it.
- `GstCapabilities::getSupportedMimeTypes` for `MediaSourceType::SUBTITLE` **always returns a hardcoded list** (`{"text/vtt", "text/ttml", "text/cc"}`) — it never queries GStreamer for subtitle support. If a new subtitle format needs to be added, it requires a code change here, not a plugin install.
- `appsrc` buffer sizes are **hardcoded** in `GstSrc::setupAndAddAppSrc`: 8 MB for video, 512 KB for audio, 256 KB for subtitle. If a stream needs larger buffers (e.g., very high bitrate video), these constants need to change.
- The `WorkerThread` and `GstDispatcherThread` are **separate threads by design** — never call GStreamer pipeline APIs from the dispatcher thread or from signal callbacks outside these threads. The dispatcher only enqueues a `HandleBusMessage` task to the worker.
- `GstGenericPlayer` constructor/destructor run on the **main thread**, but the destructor must wait for the `WorkerThread` task queue to drain (via the `Shutdown` task). Do not assume destruction is instant.
- Pending state (e.g., `pendingGeometry`, `pendingPlaybackRate`, `pendingLowLatency`) accumulates in `GenericPlayerContext` and is applied lazily inside tasks. If you change these fields outside the worker thread, the result is undefined behavior.
- The `GST_DEBUG_BIN_TO_DOT_FILE` call is gated on `GST_DEBUG_DUMP_DOT_DIR` being set — if it is not set, the pipeline graph is silently discarded. Always set this environment variable when debugging pipeline topology on a device.
- `rtkv1sink` must never be initialized during `GstCapabilities::fillSupportedMimeTypes` on platforms where doing so blackens video — there is an explicit WORKAROUND comment in the code. If new platform sinks are added to the registry, verify each one for this side effect before enabling capability queries.
- The `FlushOnPrerollController` is a workaround for an **unresolved upstream GStreamer bug** (gstreamer/gstreamer#150). The flush path in `Flush.cpp` calls `flushOnPrerollController->waitIfRequired(type)` which can block the worker thread for the duration of a preroll. This is expected behavior but will show up as a stalled worker thread in diagnostic traces.
- Audio first-frame detection uses **both** a decoder signal callback (connected in `SetupElement`) and a sink pad probe fallback. If the decoder does not emit its first-frame signal (platform-dependent), the probe fires instead. The one-shot guard in `GstGenericPlayer` prevents double reporting.

### Historical Context
Why things are the way they are.
- `GstRialtoSrc` is implemented as a custom `GstBin` (not just a raw appsrc) because each pipeline may need **multiple appsrc elements** (one per source type: audio, video, subtitle) behind a single URI source entry point. GStreamer's URI handler mechanism expects a single source element; wrapping in a `GstBin` satisfies this.
- All GStreamer API calls are routed through `IGstWrapper` and `IGlibWrapper` interfaces rather than called directly so that **unit tests can mock the entire GStreamer surface** without a real GStreamer installation or hardware. This wrapper indirection is mandatory — never add direct GStreamer calls in `gstplayer` source.
- `GstProtectionMetadata` is a custom `GstMeta` type (registered with `gst_meta_register`) rather than using GStreamer's built-in protection metadata because Rialto's decryptor needs additional fields not present in the standard type: `crypt/skip` pattern, `initWithLast15`, and a back-pointer to `IDecryptionService`.
- The `FlushOnPrerollController` exists because of gstreamer/gstreamer#150 — a race in GStreamer 1.x where sending flush events during preroll can leave the pipeline in an inconsistent state. The workaround predates a fix being merged upstream; it may eventually be removable if a sufficiently new GStreamer version is adopted platform-wide.
- The Broadcom audio-cut workaround (`pendingPlaybackRate` deferred application) was introduced for a specific platform decoder defect where setting a non-1.0 playback rate via `gst_element_send_event` while audio was in an intermediate state caused audible glitches. The behavior is documented with a comment in `GenericPlayerContext.h`.
- `LLDEV-31012` (referenced in `CheckAudioUnderflow.cpp` and `GenericPlayerContext.h`) is a tracked ticket for improving audio underflow detection. The current implementation uses a heuristic comparison between pipeline clock position and `lastAudioSampleTimestamps`; a more precise solution would use a GStreamer buffer-level probe directly on the audio sink.

### Workarounds
Known issues and their workarounds.
- **GStreamer preroll flush race (gstreamer/gstreamer#150)**: `FlushOnPrerollController` is present in every pipeline. If flush-during-seek causes the pipeline to stall, check `IFlushOnPrerollController::waitIfRequired` is being called correctly from `Flush.cpp` and that the dispatcher thread is delivering `GST_STATE_PAUSED` in time to unblock it.
- **`rtkv1sink` blacks video**: `GstCapabilities::fillSupportedMimeTypes` skips instantiating `rtkv1sink`. If a new platform adds a similarly side-effectful sink, add it to the exclusion guard in that function.
- **H.264/H.265 without `stream-format` or `codec_data`**: `GstSrc::setDefaultStreamFormatIfNeeded` automatically adds `stream-format=byte-stream`. If a stream still fails caps negotiation, check whether the source is providing `codec_data` and whether the platform decoder requires `avc` format.
- **No decryptor element on some platforms**: If `gst-inspect-1.0` shows no decryptor, confirm OpenCDM plugin path is in `GST_PLUGIN_PATH`. `GstSrc` logs `GST_WARNING "Could not create decryptor element"` when this happens but does not fail hard.
- **`svppay` payloader not found**: `GstSrc.cpp` logs `GST_WARNING("svppay not found")` on platforms without this RDK-specific payloader. If it is required for a source type and absent, that stream type will not connect correctly.
- **Audio underflow false positives under high scheduler load**: The 350 ms margin in `CheckAudioUnderflow` can produce false positives on heavily loaded systems. Temporarily increase `kAudioUnderflowMarginNs` or reduce the check timer interval if investigating whether an underflow is real or scheduling noise.
- **Worker thread appears deadlocked**: Use the `ping` / `IGstGenericPlayer::isWorkerThreadNotDeadlocked` path (called via heartbeat handler) to confirm whether the `WorkerThread` task queue is draining. If `ping` never returns, the task at the head of the queue is blocking — attach a debugger and look at the worker thread stack.

---

## Lessons Learned
What we've learned from incidents.

- Incident: WPEWebProcess crash (SIGABRT) caused by sustained audio pipeline stall.
  - What happened: `RialtoServer` PID 6311 accumulated 85+ minutes CPU time and 629 MB VIRT (from 75 MB baseline). `WPEWebProcess` entered `D` state (uninterruptible sleep). `CheckAudioUnderflow` fired every 300 ms for at least 90 seconds with `lastAudioSampleTimestamps` frozen. WPE internal watchdog timed out and fired `SIGABRT` (signal 6) from thread 355.
  - Root cause area: Hardware audio decoder (`omxeac3dec`) stopped consuming samples, likely after a large PTS rollback (`[ADEC] Rollback PTS` logged). Accumulated shared memory segments were not unmapped as the pipeline stalled.
  - What we learned:
    - The 350 ms underflow margin and per-heartbeat underflow check are not sufficient to detect a deep decoder stall before the WPE watchdog fires. A faster detection path or lower threshold is needed for live streams under scheduling pressure.
    - VIRT growth in `RialtoServer` is a leading indicator of stalled shared memory release — monitor it before audio underflow logs appear.
    - PTS rollback in audio samples is a reliable pre-crash signal; log it at MIL level and treat the next 10 seconds as a warning window.
  - Validation checklist:
    - Confirm `lastAudioSampleTimestamps` advances every second during active playback.
    - Confirm `RialtoServer` VIRT stays within ±20% of baseline during steady playback.
    - Confirm no `[ADEC] Rollback PTS` within 60 seconds of a `SIGABRT` or unexpected crash.

- Incident: Black video after second concurrent playback started.
  - What happened: On RTK platform, starting a second `RialtoServer` instance caused the first instance's video plane to go black. Not reproducible on non-RTK platforms.
  - Root cause area: Initializing `rtkv1sink` as part of `GstCapabilities::fillSupportedMimeTypes` produced a side effect on the active video plane of the first session.
  - What we learned: Any GStreamer element that has platform side effects when instantiated (even for capability queries) must be identified and excluded from `fillSupportedMimeTypes`. Add a platform-specific exclusion list rather than blanket skipping.
  - Validation checklist:
    - After bringing up two concurrent playback sessions on RTK platform, confirm both video planes remain active.
    - Confirm `fillSupportedMimeTypes` does not instantiate `rtkv1sink`.

- Incident: H.264 stream plays garbled audio/video due to absent decryptor.
  - What happened: DRM-protected stream played corrupted content. No error logged at WARNING level visible to operations.
  - Root cause area: `GstDecryptorElementFactory::createDecryptorElement` returned null silently (OpenCDM plugin absent). `GstSrc` logs only a `GST_WARNING` (GStreamer-level, not Rialto logging level), which is below the default log threshold.
  - What we learned: Decryptor creation failure must be escalated to `RIALTO_SERVER_LOG_ERROR` and should report an error state to the session; currently it silently allows playback to continue with a broken pipeline.
  - Validation checklist:
    - For any DRM stream test, check pipeline `.dot` dump confirms `rialtodecryptorvideo` and `rialtodecryptoraudio` elements are present.
    - Confirm `gst-inspect-1.0 | grep -i decrypt` returns a valid OpenCDM decryptor on the target before test execution.

- Incident: Pipeline stalled permanently during seek on preroll.
  - What happened: A seek was initiated while the pipeline was in the middle of transitioning from NULL to PAUSED (preroll). The `Flush.cpp` task sent `flush-start`/`flush-stop` before the pipeline had completed preroll. `FlushOnPrerollController::waitIfRequired` blocked the worker thread waiting for the preroll state to be reached; it was never reached because the pipeline was already going to a different state.
  - Root cause area: Race between seek (flush task enqueued) and pipeline startup (preroll not yet complete). This is the gstreamer/gstreamer#150 scenario.
  - What we learned: The `FlushOnPrerollController` is correct but does not handle the case where the pipeline target state changes underneath a pending wait. `setTargetState` must be called with the new target before `waitIfRequired` can unblock.
  - Validation checklist:
    - Run seek tests during the first 2 seconds of pipeline startup (before preroll completes).
    - Confirm the worker thread unblocks within 500 ms of a flush after preroll completes.

---

## PR-Derived Incidents and Lessons Learned

The following lessons are derived from 16 merged PRs, each representing a real bug, workaround, or incident fix in the gstplayer module.

---

- Incident: PR #414 — GStreamer queue missing from server pipeline (RDKEMW-7049).
  - What happened: The RialtoServer pipeline lacked a `queue` element between source and downstream, causing timing and scheduling problems under load.
  - What we learned: A GStreamer `queue` element decouples upstream and downstream threading boundaries. Without it, the pipeline operates in lockstep and can stall under scheduling pressure.
  - Validation checklist: Confirm `queue` element is present in pipeline `.dot` dump after `AttachSource`.

- Incident: PR #422 — Flush during GStreamer state change caused pipeline hang (RDKEMW-9891).
  - What happened: A flush task executed while the pipeline was still transitioning states. GStreamer's internal state machine could not process flush events reliably during transitions, causing the pipeline to hang indefinitely.
  - Root cause area: `FlushOnPrerollController` — `waitIfRequired` added to block flush until pipeline state is stable. This is the GStreamer bug gstreamer/gstreamer#150 manifesting in a seek-during-startup scenario.
  - What we learned: Never allow flush events to be sent while the pipeline is mid-transition. Track ongoing state changes and defer flush until state is stable. The `FlushOnPrerollController` was introduced specifically for this.
  - Validation checklist: Verify seek during the first 2 seconds of pipeline startup unblocks cleanly. Confirm the AAMP 2000 test case passes.

- Incident: PR #426 — GstMessage memory leak in HandleBusMessage (ENTDAI-2129).
  - What happened: `HandleBusMessage` task consumed bus messages but did not call `gst_message_unref` in all code paths. On error/warning message paths, the `GstMessage` was leaked.
  - What we learned: Every GStreamer object received from bus polling must be explicitly unreffed. Error-path code is easy to miss when reviewing GStreamer message handling. Valgrind/ASAN runs on the UT suite are essential for catching these.
  - Validation checklist: Run UT with valgrind or ASAN; confirm no GstMessage leaks in HandleBusMessage tests.

- Incident: PR #427 — FlushOnPrerollController loop condition caused AAMP 2000 test failure (NO-JIRA).
  - What happened: The `FlushOnPrerollController` internal condition check was incorrect in an edge case, causing the AAMP 2000 integration test to fail. The issue was a loop escape condition not being met.
  - What we learned: `FlushOnPrerollController` is complex timing-sensitive code; any change to its state machine must be validated against the AAMP 2000 test case.
  - Validation checklist: Run AAMP 2000 testcase after any change to `FlushOnPrerollController`, `Flush`, `Play`, or `Pause` tasks.

- Incident: PR #434 — Subtitles/TextTrack failed on Wayland display because it used WAYLAND_DISPLAY env var (ENTDAI-2218).
  - What happened: `GstTextTrackSink` used `getenv("WAYLAND_DISPLAY")` to find the Wayland display for subtitle rendering. On platforms where `WAYLAND_DISPLAY` is not set or points to the wrong display, subtitle rendering failed silently.
  - Root cause area: `GstTextTrackSink.cpp` — the env var was replaced with the hardcoded display name `westeros-asplayer-subtitles`.
  - What we learned: TextTrack uses a fixed Wayland display name (`westeros-asplayer-subtitles`), not the generic `WAYLAND_DISPLAY`. If subtitles don't render, check that the Westeros compositor exposes this display socket.
  - Validation checklist: Confirm subtitle display socket `westeros-asplayer-subtitles` is available on the target before testing subtitle rendering.

- Incident: PR #445 — GStreamer interactions during RemoveSource caused audio codec re-attach failures (LLAMA-18057).
  - What happened: `RemoveSource` task flushed pipeline elements (EOS + state changes) when audio was being removed for a codec change. This conflicted with the subsequent `AttachSource` call, which expected a clean state.
  - Root cause area: Server-side `removeSource` path was doing too much GStreamer pipeline manipulation that interfered with re-attach.
  - What we learned: `RemoveSource` must not touch pipeline state or flush. Codec switching relies on `AttachSource` performing the re-attach cleanly without prior GStreamer state interference. The `audioSourceRemoved` context flag was later removed once the correct approach was established.
  - Validation checklist: Run audio codec switch tests (AC3→AAC) and confirm no pipeline stall between removeSource and subsequent attachSource.

- Incident: PR #448 — Audio codec switch via re-attach failed with "cannot update caps" (LLAMA-18057).
  - What happened: When audio source was re-attached to switch codec (e.g., AC3 to AAC), `AttachSource::execute()` detected the source already existed in `streamInfo` and rejected the re-attach attempt. The existing GstCaps could not be updated in-place.
  - Root cause area: `AttachSource.cpp` — the re-attach path now calls `m_player.reattachSource()` explicitly when audio is already present.
  - What we learned: Audio codec re-attach is a legitimate use case (e.g., mid-stream codec change on ad insertion). `AttachSource` must detect this case and delegate to `reattachSource` rather than rejecting the call.
  - Validation checklist: Test AC3→AAC codec switch via remove+reattach flow. Confirm `RIALTO_SERVER_LOG_MIL("... source reattached")` appears in logs and no "cannot update caps" error.

- Incident: PR #466 — Flush order incorrect during multiple rapid flushes causing corrupted output (No-JIRA, cherry-pick of flush ordering fix).
  - What happened: When multiple seek/flush operations occurred in rapid succession, data injected after the first flush could arrive before the flush-stop event was processed, resulting in corrupted decode output.
  - Root cause area: `Flush.cpp` / flush event ordering — data and flush events must be strictly ordered in the worker thread queue.
  - What we learned: Flush ordering is critical: flush-start → discard pending data → flush-stop → new data. Any interleaving breaks decoder state. After a flush, `invalidateActiveRequests` must complete before new `NeedData` requests are issued.
  - Validation checklist: Run repeated rapid-seek stress tests. Confirm no decoder artifacts or corrupted output after 10+ consecutive seeks.

- Incident: PR #480 — Blocking `gst_element_get_state` calls inside AmLogic audio codec switch caused deadlocks.
  - What happened: The AmLogic audio codec switch path (`performAudioTrackCodecChannelSwitch`) called `gst_element_get_state` with a long timeout. This is a blocking call that can stall the worker thread for seconds, blocking all other pipeline tasks.
  - Root cause area: `GstGenericPlayer.cpp` — the blocking `get_state` calls in `configAudioCap`, `haltAudioPlayback`, and `resumeAudioPlayback` were ported from `rdk_gstreamer_utils_soc` without accounting for the single-threaded task queue model.
  - What we learned: Never call `gst_element_get_state` with a non-zero timeout from inside a worker thread task. Use `GST_CLOCK_TIME_NONE` only if the element is known to be in a stable state with no pending async operation.
  - Validation checklist: Confirm no calls to `gstElementGetState` with timeout > 0 inside task `execute()` methods. Run audio codec switch on AmLogic platform and confirm worker thread does not stall.

- Incident: PR #484 — PlaybackGroup element tracking incorrect after audio codec switch (RDKEMW-16807).
  - What happened: `GenericPlayerContext.playbackGroup` held stale element pointers after an audio codec switch. `DeepElementAdded` and `UpdatePlaybackGroup` were not correctly identifying the new parser/decoder elements using factory-type checks, and `typefind` was not re-linked to the audio parser after switch.
  - Root cause area: `DeepElementAdded.cpp` and `UpdatePlaybackGroup.cpp` — element detection logic and typefind-to-parser link.
  - What we learned: After an audio codec switch, `playbackGroup.m_curAudioTypefind`, `m_curAudioDecodeBin`, `m_curAudioParse`, and `m_curAudioDecoder` must all be refreshed. The `typefind → parser` link must be re-established explicitly in `UpdatePlaybackGroup`.
  - Validation checklist: Confirm `.dot` pipeline dump after audio codec switch shows `typefind` correctly linked to the new audio parser. Confirm no stale element pointer dereferences.

- Incident: PR #501 — Synchronous `play()` call on main thread caused race conditions (RDKEMW-18702).
  - What happened: When no other state changes were pending, `GstGenericPlayer::play()` executed `gst_element_set_state(PLAYING)` directly on the main thread as a "fast path". This bypassed the worker thread serialization and could race with tasks already in the worker queue that modify pipeline state.
  - Root cause area: `GstGenericPlayer.cpp` — the `m_ongoingStateChangesNumber` counter-based fast-path that called `changePipelineState` directly from the main thread.
  - What we learned: There is no safe "fast path" for direct GStreamer pipeline state changes from the main thread. All GStreamer state changes must go through the worker thread task queue. The `enqueuePriorityTaskAndWait` mechanism for `play()` was also removed in favour of the standard task path.
  - Validation checklist: Confirm `play()` always enqueues a `Play` task to the worker thread and never calls `gst_element_set_state` directly from the main thread. Run race-condition stress tests with rapid play/pause cycles.

- Incident: PR #531 — Audio re-attach after dynamic codec change left EOS flag stale (RDKEMW-17771).
  - What happened: When audio was removed and re-attached for a dynamic codec change, `RemoveSource` did not reset the `eosNotified` flag for the audio stream. On re-attach, the EOS state from the previous codec was carried forward, causing underflow and stall behavior immediately after re-attach.
  - Root cause area: `RemoveSource.cpp` — EOS state not cleared; `AttachSource.cpp` — `audioSourceRemoved` flag and `NeedData` triggering on re-attach.
  - What we learned: Every `RemoveSource` for audio must reset the per-stream EOS state. Every `AttachSource` for re-attach must set `isDataNeeded=true` and immediately trigger `NeedData` for the new codec.
  - Validation checklist: After AC3→AAC switch, confirm `NeedData` fires for the new audio source within one healthcheck cycle. Confirm no `eosNotified` state leaks from the old codec.

- Incident: PR #546 — AmLogic audio sink position reset incorrectly after playback rate change (RDKEMW-20535).
  - What happened: On AmLogic, a new-segment event sent after a playback rate change used segment start position 0, causing the platform audio sink to report position 0 instead of the actual current position. This manifested as bogus position reporting after trick-play.
  - Root cause area: `SetPlaybackRate.cpp` / `FinishSetupSource.cpp` — `pushSampleIfRequired` did not track the last audio segment start position.
  - What we learned: When emitting a new-segment event for an AmLogic-specific rate-change path, the segment start must be the last injected audio sample position, not 0. Track this in `GenericPlayerContext`.
  - Validation checklist: After playback rate change on AmLogic platform, confirm position reporting resumes from the correct position immediately.

- Incident: PR #556 — RialtoServer crash on channel switch due to position-reporting timer not cancelled on teardown (RDKEMW-18434).
  - Problem signature: RialtoServer process crash observed while switching between Xumo FAST channels during closed caption verification on BCM Rogers Monarch.
  - Root cause area: `GstGenericPlayer::termPipeline()` in `GstGenericPlayer.cpp` — `m_playbackInfoTimer` was not cancelled and reset before pipeline teardown. If the timer fired after the pipeline was already torn down, it accessed freed memory.
  - What we learned: Every timer created in `GstGenericPlayer` must be explicitly cancelled and reset in `termPipeline()`. Timer cancellation order in teardown matters; add each new timer to the teardown checklist.
  - Validation checklist: Confirm `m_playbackInfoTimer->cancel()` and `m_playbackInfoTimer.reset()` are called in `termPipeline()` before pipeline stream info is cleared. Run rapid channel-switch stress test on BCM platform.

- Incident: PR #558 — Subtitles out of sync because `SynchroniseSubtitleClock` used video-sink position instead of pipeline position.
  - What happened: `SynchroniseSubtitleClock` queried position from `m_context.videoSink` using `gstElementQueryPosition`. On some platforms, the video sink's reported position lags the pipeline clock, causing subtitle timestamps to be sent behind the actual playback position.
  - Root cause area: `SynchroniseSubtitleClock.cpp` — position query changed from `videoSink` to `m_player.getPosition(m_context.pipeline)` which queries the pipeline clock directly.
  - What we learned: Always use pipeline-clock position (`IGstGenericPlayerPrivate::getPosition`) for subtitle synchronization. Sink-level position queries are unreliable and platform-specific.
  - Validation checklist: Confirm subtitles are in sync across platforms after rapid channel switch. Confirm `SynchroniseSubtitleClock` logs pipeline position, not videoSink position.

- Incident: PR #563 — Async pipeline setup callbacks (deepElementAdded, updatePlaybackGroup) executed after pipeline teardown (RDKEMW-21388).
  - Problem signature: Use-after-free / crash triggered when GStreamer signals (`deep-element-added`, typefind `have-type`) fired after `GstGenericPlayer` had already called `termPipeline()` and the worker thread was null.
  - Root cause area: `GstGenericPlayer.cpp` — `deepElementAdded` and `updatePlaybackGroup` callbacks did not guard against null `m_workerThread`. Additionally, element and caps pointers passed into tasks were non-owned; if GStreamer freed them before the task executed, the task used dangling pointers.
  - What we learned:
    1. Always guard `enqueueTask` with a null check on `m_workerThread` — this is the standard teardown guard pattern.
    2. Take `gstObjectRef` on any GStreamer element or `gstCapsCopy` on any `GstCaps` pointer passed into an async task. Release the owned reference in the task destructor.
  - Validation checklist: Run rapid teardown stress tests (create/destroy player in rapid succession). Confirm no crash or sanitizer report from `deepElementAdded` or `updatePlaybackGroup` callbacks arriving post-teardown.
