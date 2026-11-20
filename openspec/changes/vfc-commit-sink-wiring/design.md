# Design

## Context

See proposal.md for the motivation. Today `MediaPipelineServerInternal::allSourcesAttachedInternal()` calls `m_gstPlayer->allSourcesAttached()` and sets `m_wasAllSourcesAttachedCalled`. `GstGenericPlayer::setWesterossinkSecondaryVideo()` creates a westerossink, sets `res-usage = 0` and assigns it to `video-sink`; `applyPlaybinSinkOverride()` applies `RIALTO_PLAYBIN_VIDEO_SINK`. `MediaSourceVideoCapsBuilder` builds appsrc caps from the attached source. GStreamer work runs on the player's worker thread; pipeline methods run on the server main thread. The N4 proposal (accepted) fixes `codec` as `GST_TYPE_CAPS` and `resolution` as a maximum-size `"WxH"` string; the vendor sink rejects bad values by failing NULL→READY.

## Goals / Non-Goals

**Goals:**
- Steps 1–6 and rollback in a fixed order, synchronous for the caller.
- A clean hand-over to admission (R8) that R7 can test without R8.

**Non-Goals:**
- Capabilities, slot count, `createPool`, `getPool` and the size check (R8, A2).
- Frame delivery and teardown after commit (R9, R10).

## Decisions

### D1. Orchestration in the pipeline, GStreamer work in the player
`allSourcesAttachedInternal()` (main thread): if a capture is attached, run `CaptureCommit::run()`; on `false`, return `false` without setting `m_wasAllSourcesAttachedCalled`. `CaptureCommit` performs the checks, creates the session (R2 loader) and calls `m_gstPlayer->configureCaptureSink(session, codecCaps, resolution, timeout)`, which runs on the worker thread and blocks the main thread until done. Then it calls `ICaptureAdmission::admit(session)` (R8). On success, the session is moved into the R9 `CaptureSessionRuntime`.

*Why:* keeps the synchronous contract (proposal §3.3) while respecting the existing thread ownership of GStreamer objects.

### D2. One sink creation path
Refactor `setWesterossinkSecondaryVideo()` into `createWesterossink(bool secondaryVideo)`; the capture path calls it, sets `res-usage = 0` when secondary, then the three properties, then assigns `video-sink`. The shipping secondary path calls the same function, so Q3's "one sink" holds by construction. If a shipping secondary sink was already assigned, the capture path replaces it with a new instance carrying both settings.

### D3. READY wait
`gst_element_set_state(sink, READY)` then `gst_element_get_state(sink, ..., timeout)`. Timeout from `RIALTO_VIDEO_FRAME_CAPTURE_READY_TIMEOUT_MS` (default 5000), read at each commit like the A3 setting, so tests can vary it.

### D4. Rollback discards the sink instance
On any failure: set the sink to NULL and wait, set playbin `video-sink` to NULL, unref the sink, then delete the session. A sink that has received a session cannot take a different one (SoC spec §4.2), so a retry always creates a new sink. For a secondary-video pipeline, the retry recreates it through D2.

### D5. Codec caps and resolution
`codec` caps come from `MediaSourceVideoCapsBuilder::buildCaps()` for the attached video source (the same caps appsrc gets); the reference is released after `g_object_set`. `resolution` is formatted from `VideoRequirements::maxWidth/maxHeight`.

## Risks / Trade-offs

- [The proprietary playbin replaces our sink or cycles it to NULL during READY→PAUSED] → capture would disconnect permanently. Not detectable in unit tests; task 4.2 checks it on target with the vendor playbin.
- [Blocking the main thread for up to the READY timeout] → bounded and configurable; only pipelines with a capture are affected.
- [Vendor sink without the N4 properties] → `g_object_class_find_property` check before setting; missing properties fail the commit with a clear log.

## Migration Plan

None. Pipelines without a capture take the existing path unchanged.
