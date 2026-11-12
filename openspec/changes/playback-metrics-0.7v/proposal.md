Proposal: Compatibility Update for Southbound SoC Metric Contract (0.7v)

## Implementation Guidance

**IMPORTANT:** Do not make assumptions or infer behavior if there is any ambiguity in this proposal or the specification. If you have any doubt about the correct implementation approach, field semantics, validation rules, or contract details, ask for clarification before proceeding. Incorrect assumptions can lead to subtle compatibility issues that are difficult to debug.

## Specification Reference

**Baseline specification:** SPECIFICATION-soc-media-metrics_new-soc.md, version 0.7 (dated 2026-11-12)

This proposal is written against and must remain aligned with the normative SoC media metrics specification version 0.7. If the specification is updated, all validation rules and contract definitions in this proposal must be re-reviewed for continued alignment.

Why

The current Southbound metric implementation still validates and forwards a legacy GStreamer message shape that does not match the normative SoC contract. The parser and producer logic continue to rely on older names and semantics such as `phase`, `pts`, `source_id`, and `session_id`, and they do not consistently enforce the required `observation-kind` model defined by the 0.7 SoC specification.

This change is required because the server emits the SoC metric payload and the client must consume the same contract exactly. A mismatch in field names, types, or validation semantics is not a harmless compatibility issue; it breaks the contract boundary between the native SoC producer and the Rialto-facing pipeline implementation. The implementation therefore needs a strict compatibility fix that aligns the server-side parser and forwarded message model to the authoritative contract without broadening the scope into unrelated metrics redesign.

This proposal is intentionally a compatibility-layer fix. It preserves the current playback-metrics structure and signaling flow while aligning the Southbound payload to the 0.7 specification and the existing codebase baseline.

What Changes

- Align the Southbound message payload to the normative `media-pipeline-metric` envelope.
- Require the current contract fields: `metric`, `observation-kind`, and the valid optional fields `pts-ns`, `duration-ns`, and `count`.
- Replace the legacy `phase` semantics with the required observation-kind model: `occurrence`, `episode-started`, and `episode-resolved`.
- Enforce the exact field names and GTypes from the 0.7 specification so the server and client validate the same contract.
- Capture `source_id` and `session_id` from the GStreamer message source element (`GST_MESSAGE_SRC()`) rather than expecting them as fields in the SoC metric payload; the normative media-pipeline-metric structure does not carry these as data fields.
- Ensure the parser rejects invalid or malformed metric payloads before they are accepted or forwarded.
- Preserve the existing playback-metrics flow and implementation boundaries, while narrowing the scope to the contract fix required by 0.7v.
- Keep this change limited to the Southbound SoC contract alignment; no change is introduced to northbound API semantics, V1.1 API behavior, or broader analytics processing.

Normative Contract

The Southbound message contract to be enforced is the following:

- Message name: `media-pipeline-metric`
- Required field: `metric` with type `G_TYPE_UINT`
- Required field: `observation-kind` with type `G_TYPE_UINT`
- Optional field: `pts-ns` with type `G_TYPE_INT64`
- Optional field: `duration-ns` with type `G_TYPE_UINT64`
- Optional field: `count` with type `G_TYPE_UINT64`
- **NOT present in normative SoC message:** `source_id` and `session_id` (captured by Rialto from GStreamer message source element, NOT from SoC payload)
- Legacy field names rejected by this change: `metric_type`, `phase`, `pts_ns`, `source_id`, and `session_id`
- Authorized observation-kind values: `occurrence` (value 1), `episode-started` (value 2), and `episode-resolved` (value 3)
- Any unknown or malformed metric message must be rejected before forwarding, processing, or publishing to the downstream consumer path.

### Field semantics by observation kind

**`pts-ns` (optional, G_TYPE_INT64):**
- **OCCURRENCE:** The affected frame or sample PTS when known; otherwise the best available stream position at detection time. Zero is a valid PTS and is not a sentinel for "unavailable"; omit the field entirely if PTS is genuinely unknown.
- **EPISODE_STARTED:** The PTS at which the episode condition was first detected.
- **EPISODE_RESOLVED:** The episode-start PTS (repeated), allowing the resolution message to be self-describing without requiring the correlated start message.

**`duration-ns` (required in EPISODE_RESOLVED):**
- Monotonic elapsed time (in nanoseconds) from episode start observation to natural or boundary resolution.
- Measured in GStreamer's monotonic clock (`gst_util_get_timestamp()`), NOT pipeline running time, media PTS, or STC.
- Always non-negative; represents actual elapsed wall-clock time even when media PTS/STC is stationary.
- An active episode that is paused while waiting for input continues accumulating duration.

**`count` (optional, G_TYPE_UINT64, used only in EPISODE_RESOLVED for specific metrics):**
- Only `MEDIA_METRIC_VIDEO_FRAME_REPEAT` (value 5) and `MEDIA_METRIC_AUDIO_GAP` (value 6) use `count` in `EPISODE_RESOLVED` messages.
- For `VIDEO_FRAME_REPEAT`: number of unintended repeated presentations of the same video frame in the episode.
- For `AUDIO_GAP`: number of substitute/null audio frames emitted in the episode.
- Counted in frames (not audio samples for audio metrics).
- `MEDIA_METRIC_BUFFER_UNDERFLOW` (value 7) does NOT carry `count` in resolution messages.
- All OCCURRENCE messages have NO `count` field.

### Metric type and observation kind enum values

| Metric | Value | Observation kinds used |
|---|---|---|
| `MEDIA_METRIC_VIDEO_FRAME_DROP` | 1 | OCCURRENCE only |
| `MEDIA_METRIC_AUDIO_FRAME_DROP` | 2 | OCCURRENCE only |
| `MEDIA_METRIC_VIDEO_DECODE_ERROR` | 3 | OCCURRENCE only |
| `MEDIA_METRIC_AUDIO_DECODE_ERROR` | 4 | OCCURRENCE only |
| `MEDIA_METRIC_VIDEO_FRAME_REPEAT` | 5 | EPISODE_STARTED, EPISODE_RESOLVED (with `count`) |
| `MEDIA_METRIC_AUDIO_GAP` | 6 | EPISODE_STARTED, EPISODE_RESOLVED (with `count`) |
| `MEDIA_METRIC_BUFFER_UNDERFLOW` | 7 | EPISODE_STARTED, EPISODE_RESOLVED (no `count`) |

Validation rules

- The parser must validate the structure name before any metric interpretation is attempted.
- `metric` must be present and must be a `G_TYPE_UINT`.
- `observation-kind` must be present and must be a `G_TYPE_UINT`.
- `pts-ns` is accepted only as `G_TYPE_INT64` when present.
- `duration-ns` is accepted only as `G_TYPE_UINT64` when present.
- `count` is accepted only as `G_TYPE_UINT64` when present.
- Legacy field names must not be treated as valid data-bearing contract fields.
- Unknown enum values must be rejected rather than coalesced into a fallback case.
- Messages that are structurally valid but semantically inconsistent with the 0.7 contract must be rejected in the validation layer, not silently ignored downstream.

### Source element identification and enrichment

The Rialto server parser does NOT expect `source_id` or `session_id` as fields in the SoC metric payload. Instead, the server enriches the metric with source tracking at the message-parsing stage:

- **Message source element:** Extract the authoritative producing element from `GST_MESSAGE_SRC(message)`, which identifies which SoC element (video decoder, audio decoder, video output, etc.) posted the metric.
- **Source-to-session mapping:** Maintain Rialto's internal session/source association to map the GStreamer element source to the corresponding Rialto source identifier and session context.
- **Enrichment without payload change:** The normalized metric event forwarded downstream carries the Rialto session and source identifiers, but the normative SoC message structure remains unchanged; no `source_id` or `session_id` fields are added to the media-pipeline-metric message itself.

This separation keeps the SoC bus contract minimal while allowing Rialto to track source/session context at the integration boundary.

### Data flow: Source ID and Session ID tracking

To avoid confusion about when source_id and session_id are present:

| Stage | Contains source_id/session_id? | Details |
|---|---|---|
| **SoC Producer → Rialto (GStreamer bus)** | ❌ NO | Raw `media-pipeline-metric` message contains only: `metric`, `observation-kind`, optional `pts-ns`, `duration-ns`, `count` |
| **Rialto Parser (after enrichment)** | ✅ YES | Rialto extracts `GST_MESSAGE_SRC()`, maps to internal source/session context, enriches the metric object |
| **Rialto Server → Client (via IPC proto)** | ✅ YES | Enriched metric sent to client includes the Rialto-tracked source_id and session_id; client receives fully context-aware event |

**Key principle:** The SoC bus contract is minimal and does not carry producer-scoped fields. Rialto adds source/session enrichment at its integration boundary before forwarding to the client.

Non-Goals

- Will not redesign the broader playback-metrics architecture.
- Will not replace the northbound or V1.1 API work.
- Will not add new metric fields, semantics, or producer behavior beyond the normative Southbound contract.
- Will not broaden this change into subscription management, notification-loss handling, or client-side analytics behavior.
- Will not introduce a parallel or alternate metric schema for the same SoC bus contract.

Impacted Areas

This change is intentionally scoped to the Rialto-side Southbound contract alignment. The table below lists the concrete implementation files expected to be updated so the design and task generation can follow the revised 0.7 specification.

| Component | Impact Type | Location / Path | Details |
|-----------|------------|-----------------|---------|
| **SoC specification source** | Reference | `SPECIFICATION-soc-media-metrics_new-soc.md` | Authoritative 0.7 contract for the Southbound `media-pipeline-metric` payload: `metric`, `observation-kind`, `pts-ns`, `duration-ns`, and `count` |
| **Metric parser** | Implementation change | `rialto/media/server/metrics/source/RialtoMetrics.cpp` | Replace legacy validation that expects `metric_type`, `phase`, `source_id`, and `session_id` with the 0.7 field names, types, and validation rules. Extract and map the GStreamer message source element (GST_MESSAGE_SRC) to Rialto's source/session context without modifying the normative SoC message structure. |
| **Metric contract model** | Implementation change | `rialto/media/server/metrics/include/IRialtoMetrics.h` | Align the internal parsed metric structure with the 0.7 contract and remove legacy phase-based assumptions. Add methods to track message source element to Rialto source/session mapping (e.g., which GStreamer decoder element corresponds to which Rialto source). |
| **GStreamer dispatch path** | Implementation change | `rialto/media/server/gstplayer/source/GstDispatcherThread.cpp`, `rialto/media/server/gstplayer/source/GstGenericPlayer.cpp` | Ensure the incoming bus message flow remains compatible with the 0.7 message contract and does not keep forwarding the legacy payload shape |
| **Playback metric relay** | Implementation change | `rialto/media/server/main/source/MediaPipelineServerInternal.cpp`, `rialto/media/server/main/include/MediaPipelineServerInternal.h` | Forward the normalized SoC metrics data without broadening scope beyond the compatibility fix |
| **IPC schema** | Implementation change | `rialto/proto/mediapipelinemodule.proto` | Define or update the `MediaPipelineMetric` proto message to mirror the SoC `media-pipeline-metric` GStreamer structure: required `metric` (uint32), required `observation_kind` (uint32), optional `pts_ns` (int64), optional `duration_ns` (uint64), optional `count` (uint64). Field names use underscores per proto convention. |
| **IPC client serialization** | Implementation change | `rialto/media/server/ipc/source/MediaPipelineClient.cpp`, `rialto/media/server/ipc/include/MediaPipelineClient.h` | Keep the serialized event payload aligned with the same field names and validation semantics used on the producer side |
| **Metrics documentation** | Documentation update | `rialto/docs/PlaybackMetrics.md` | Update the documented field semantics to the accepted 0.7 contract wording; include observation-kind-specific pts-ns, duration-ns, and count semantics |
| **Rialto unit tests** | Regression coverage | `rialto/tests/unittests/media/server/metrics/RialtoMetricsTest.cpp` | Update tests to reject legacy names and validate the 0.7 contract under different observation kinds, metric types, and field presence rules |
| **Component test stub** | Regression coverage | `rialto/tests/componenttests/server/stubs/GstreamerStub.cpp`, `rialto/tests/componenttests/server/stubs/GstreamerStub.h` | Update synthetic message generation so it reflects the migrated SoC contract instead of the legacy input format; include test cases for all metric types and observation kinds |

Validation and conformance requirements

A conforming implementation must satisfy the following:

- The structure name is exactly `media-pipeline-metric`.
- `metric` is present and encoded as `G_TYPE_UINT`.
- `observation-kind` is present and encoded as `G_TYPE_UINT`.
- `pts-ns`, `duration-ns`, and `count` use the exact names and GTypes defined by the SoC contract.
- Legacy names such as `metric_type`, `phase`, `pts_ns`, `source_id`, and `session_id` are rejected as non-normative.
- The validation logic rejects malformed or unknown message values before the event is passed downstream.
- Observation-kind handling is based on the 0.7 semantics and not on the legacy phase state machine.
- The server and client remain aligned on the same message contract, permitting no divergent interpretation across the pipeline boundary.- `duration-ns` in EPISODE_RESOLVED uses monotonic clock elapsed time (gst_util_get_timestamp() delta), not media PTS or pipeline running time.
- `count` is present in EPISODE_RESOLVED for VIDEO_FRAME_REPEAT and AUDIO_GAP only; absent for BUFFER_UNDERFLOW and all OCCURRENCE messages.
## Success Criteria

- The SoC producer message uses `media-pipeline-metric` and the required field names.
- `metric` is emitted as a numeric enum value and validated as such.
- `observation-kind` is used instead of the legacy `phase` model.
- `pts-ns`, `duration-ns`, and `count` use the exact names and GTypes required by the 0.7 contract.
- Invalid or malformed messages are rejected before they are processed.
- Unknown or legacy field names are rejected and never treated as accepted contract fields.
- `source_id` and `session_id` are NOT expected in the SoC metric payload; instead, the Rialto parser extracts the message source element (GST_MESSAGE_SRC) and maps it to the corresponding Rialto source/session context.
- The server and client remain aligned on the same Southbound payload contract.
- The current implementation remains intact as a compatibility layer while the contract is corrected.
- The GStreamer consumer path validates the same message contract expected from the SoC producer and does not accept legacy payload variants.

## Test Coverage Requirements

**Unit tests — Message Parsing and Extraction:**
- ✅ `ShouldParseValidMediaPipelineMetricFromGStreamerBus` — Parse valid 0.7v metric messages with all required fields (metric, observation-kind) and optional fields (pts-ns, duration-ns, count) from GStreamer bus element messages
- ✅ `ShouldExtractSourceElementAndMapToSessionContext` — Extract source element from `GST_MESSAGE_SRC()` and correctly map to internal Rialto source_id and session_id context
- ✅ `ShouldIgnoreMalformedMetricMessages` — Reject messages with incorrect structure name, missing required fields, or unparseable types before enrichment
- ✅ `ShouldRejectInvalidEnumValues` — Reject metric values outside 1-7 range and observation-kind values outside 1-3 range
- ✅ `ShouldRejectLegacyFieldNames` — Reject legacy field names (`metric_type`, `phase`, `pts_ns` with underscore, `source_id`, `session_id` in payload) in the validation layer

**Unit tests — Field Semantics and Validation:**
- ✅ `ShouldAcceptValidFieldCombinationsPerObservationKind` — Parse and accept valid combinations of optional fields per observation-kind (OCCURRENCE: no duration/count; EPISODE_STARTED: duration optional; EPISODE_RESOLVED: duration required; count for VIDEO_FRAME_REPEAT/AUDIO_GAP only)
- ✅ `ShouldAcceptPtsNsZeroAndNegativeValues` — Accept zero and negative pts-ns as valid (zero is valid PTS, not a sentinel); validate GType as G_TYPE_INT64
- ✅ `ShouldValidateDurationNsIsMonotonic` — Validate duration-ns as monotonic elapsed time (non-negative, measured in GStreamer monotonic clock)
- ✅ `ShouldEnforceCountRulesPerMetricType` — Reject count field for OCCURRENCE/EPISODE_STARTED/BUFFER_UNDERFLOW; require count for EPISODE_RESOLVED VIDEO_FRAME_REPEAT and AUDIO_GAP

**Unit tests — Source Enrichment:**
- ✅ `ShouldMapMessageSourceToRialtoSourceAndSessionContext` — Verify GST_MESSAGE_SRC() extraction, element-to-source mapping, and session context association
- ✅ `ShouldPreserveEnrichmentAcrossIpcBoundary` — Verify enriched source_id and session_id are correctly serialized to proto and passed downstream to IPC client path

**Component tests — End-to-end Metric Flow:**
- ✅ `ShouldAcceptAllMetricTypesWithCorrectObservationKindCombinations` — Parse all 7 metric types with their valid observation kinds per spec without corruption
- ✅ `ShouldMaintainSourceCounterMonotonicity` — Verify frame counter fields (rendered, dropped, corrupted) remain monotonically increasing per source_id across multiple metric messages
- ✅ `ShouldRejectNonMonotonicCountersAndRecoverGracefully` — Detect and reject counter regressions for the same source without crashing parser or blocking subsequent messages
- ✅ `ShouldDispatchEnrichedMetricsToDownstream` — Verify end-to-end flow: GStreamer bus message → parser → enrichment with source/session → IPC serialization → downstream delivery

Rollback Plan

- Revert the contract-alignment changes in the affected producer/parser implementation and restore the previous validation logic if the 0.7 contract is not confirmed.
- Keep the scope limited to the Southbound contract fix so a rollback does not broaden into unrelated playback-metrics work.
- If the implementation cannot be validated cleanly, revert the branch to the last known-good revision and re-apply only the exact contract-alignment changes.
- Rollback is limited to the compatibility fix and does not affect unrelated playback or media pipeline behavior.
