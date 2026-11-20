## Why

Attaching a capture (R6) only records intent. At `allSourcesAttached()` the server must turn it into a working capture path before any media flows: create a HAL session, wire it into westerossink, configure the sink's codec and resolution, and reach configured READY, rolling everything back if any step fails so the application can retry (proposal §3.3 steps 1–6, SoC spec §4.1–§4.4). With the N4 proposal accepted, the `codec` and `resolution` properties have a defined type and meaning, so this step can now be specified exactly.

## What Changes

- With a capture attached, `allSourcesAttached()` runs a synchronous configuration step before shipping setup is scheduled:
  1. check that a video source is attached and is clear (not encrypted);
  2. check that no `RIALTO_PLAYBIN_VIDEO_SINK` override selects a sink other than westerossink;
  3. create a new HAL session for this pipeline (R2);
  4. create a westerossink and assign it to playbin's `video-sink`, or, for a secondary-video pipeline, use the one westerossink that carries `res-usage = 0` (Q3);
  5. set `rdk-capture-session` on that sink before any state change;
  6. set `codec` (`GST_TYPE_CAPS`, the caps `MediaSourceVideoCapsBuilder` builds for appsrc) and `resolution` (`"<maxWidth>x<maxHeight>"` from `VideoRequirements`), then drive the sink to READY and wait, with a timeout (default 5 s, configurable).
- Then the server hands over to admission (R8: capabilities, slot count, `createPool`, commit record). This change defines that hand-over as an interface.
- If any step, or admission, fails: the sink goes back to NULL, the `video-sink` assignment is cleared, the sink instance that received the session is discarded, the session is deleted after the sink is at NULL, `allSourcesAttached()` returns `false`, and the commit is not recorded, so the application may call it again.
- Without a capture, `allSourcesAttached()` is unchanged.
- **Not in this change:** the `maxWidth/maxHeight` size check. Under the A2 proposal it moves to step 7 (compare against the HAL's `maximumContentSize`), which is R8's.

## Capabilities

### New Capabilities

- `capture-commit-configuration`: What `allSourcesAttached()` does when a capture is attached up to configured READY: preconditions, HAL session creation, sink wiring and configuration, the READY wait, and rollback.

### Modified Capabilities

None.

## Impact

- **Server:** `MediaPipelineServerInternal::allSourcesAttachedInternal()` runs the step before `m_gstPlayer->allSourcesAttached()` and sets `m_wasAllSourcesAttachedCalled` only on success; `GstGenericPlayer` gains a capture-sink setup that shares the creation path of `setWesterossinkSecondaryVideo()`, sets the three sink properties, drives the sink to READY and rolls back.
- **Interfaces:** an `ICaptureAdmission` hook implemented by R8; R7 tests use a mock.
- **Tests:** server unit tests with mocked gst/glib wrappers and the fake HAL; component tests.
- **Dependencies:** R2 (sessions), R6 (attachment), N4 proposal (property format). Vendor westerossink must implement the N4 properties for on-target use.
- **Open, on target:** the proprietary playbin must keep the assigned westerossink instance and not cycle it to NULL during READY→PAUSED (roadmap playbin concern); an on-target check is a task here.
