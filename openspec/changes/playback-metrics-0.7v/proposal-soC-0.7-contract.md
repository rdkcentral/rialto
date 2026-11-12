# Proposal: Compatibility Update for Southbound SoC Metric Contract

**Proposal: Add a compatibility-layer update to the current playback-metrics implementation so the Southbound GStreamer SoC metric producer and parser conform to the normative metric contract, without changing the existing broader playback-metrics design or replacing the northbound V1.1 API work.**

---

## Why

The implementation currently emits and consumes a legacy Southbound metric message shape that differs from the normative SoC contract. In particular, it still uses older field names and semantics such as `phase`, `pts`, `source_id`, and `session_id` on the producer-side metric message, and it does not consistently enforce the required `observation-kind` model.

This proposal is intentionally a compatibility fix layered on top of the current implementation. It does not rewrite the existing playback-metrics design. It updates the Southbound GStreamer SoC message contract and the validation/parsing behavior so the current design can interoperate with the required metric contract while preserving the broader implementation as-is.

The northbound work already captured in the V1.1 proposal remains separate and should continue to follow its own requirements. This proposal addresses the Southbound contract alignment only.

## No hallucination / no scope creep

This proposal is intentionally strict and evidence-based. It does not invent new field names, new semantics, or new SoC behaviors beyond the normative contract and the current implementation baseline. If a requirement is unclear, ambiguous, or not explicitly supported by the spec, the correct action is to stop and ask for clarification rather than guessing or expanding the scope.

This proposal must remain a compatibility fix layered on top of the existing design. It does not redefine the broader playback-metrics architecture, does not replace the northbound V1.1 work, and does not introduce additional Southbound behaviors beyond the validated metric contract and the current implementation constraints.

## What changes

### 1. Require the normative `media-pipeline-metric` envelope

The SoC metric producer message must conform to the required `GST_MESSAGE_ELEMENT` structure name:

- `media-pipeline-metric`

The message must carry the common fields required by the specification:

- `metric` with `G_TYPE_UINT`
- `observation-kind` with `G_TYPE_UINT`

Optional fields are allowed only when required by the observation kind:

- `pts-ns` with `G_TYPE_INT64`
- `duration-ns` with `G_TYPE_UINT64`
- `count` with `G_TYPE_UINT64`

The legacy shape using `metric_type`, `phase`, `pts_ns`, `source_id`, and `session_id` on the producer-side message is not compliant and must be updated to the normative contract.

### 2. Require numeric enum values for `metric`

The `metric` field must be emitted as a numeric enum value, not as a string label.

The required numeric metric values are:

- `MEDIA_METRIC_VIDEO_FRAME_DROP = 1`
- `MEDIA_METRIC_AUDIO_FRAME_DROP = 2`
- `MEDIA_METRIC_VIDEO_DECODE_ERROR = 3`
- `MEDIA_METRIC_AUDIO_DECODE_ERROR = 4`
- `MEDIA_METRIC_VIDEO_FRAME_REPEAT = 5`
- `MEDIA_METRIC_AUDIO_GAP = 6`
- `MEDIA_METRIC_BUFFER_UNDERFLOW = 7`

The parser must accept only these valid numeric identifiers and reject malformed or unknown values.

### 3. Require `observation-kind` instead of `phase`

The producer and parser must use the explicit observation kinds:

- `MEDIA_METRIC_OBSERVATION_OCCURRENCE`
- `MEDIA_METRIC_OBSERVATION_EPISODE_STARTED`
- `MEDIA_METRIC_OBSERVATION_EPISODE_RESOLVED`

This replaces the old `phase` convention and must be enforced in the validation logic.

Required semantics:

- OCCURRENCE: a single frame drop or decode error; no extra required fields
- EPISODE_STARTED: no `count` and no `duration-ns`
- EPISODE_RESOLVED: requires `duration-ns`; repeat/gap resolved messages also require `count`

### 4. Require exact field names and types

The implementation must use the exact field names and GTypes from the normative spec:

- `pts-ns` instead of `pts`
- `duration-ns` instead of `duration`

Type rules:

- `pts-ns` must be `G_TYPE_INT64`
- `duration-ns` must be `G_TYPE_UINT64`
- `count` must be `G_TYPE_UINT64` when present

A zero PTS value remains valid and must not be treated as a missing value.

### 5. Do not emit producer-side `source_id` or `session_id`

The SoC producer message must not include `source_id` or `session_id` in the raw GStreamer message. These identifiers belong to the layer above the SoC producer contract and should not be carried in the Southbound producer payload.

### 6. Derive the corrupted count from decode-error observations

The corrupted counter must be derived from the authoritative decode-error metric observations, not inferred through ad hoc or local-only logic.

Required behavior:

- each valid frame-level decode-error observation contributes to the corrupted count once for a distinct affected frame
- repeated hardware indications for the same frame must not increase the count again
- the corrupted count must remain monotonic over the active source lifetime

### 7. Validate the `GstStructure` before accepting or forwarding it

The parser and validation layer must enforce the exact contract before the event is processed or forwarded further upstream.

Required validation rules:

- `metric` exists and is `G_TYPE_UINT`
- `observation-kind` exists and is `G_TYPE_UINT`
- `pts-ns` is `G_TYPE_INT64` when present
- `duration-ns` is `G_TYPE_UINT64` when present
- `count` is `G_TYPE_UINT64` when present
- messages that do not match the observation-kind requirements must be rejected

## Northbound scope

This proposal does not replace the existing northbound or V1.1 API work. It is a compatibility fix for the Southbound SoC metric producer/consumer contract while preserving the existing broader playback-metrics design and the related northbound requirements already defined elsewhere.

The northbound changes described in the V1.1 proposal remain a separate contract layer and should be evaluated independently from this Southbound compatibility fix.

## Non-goals

This proposal intentionally does not expand into unrelated subscription flows, notification-loss handling, Northbound API redesign, or broader client-side metrics behavior beyond the SoC producer message contract and its validation logic.

## Success criteria

- The SoC producer message uses `media-pipeline-metric` and the required field names.
- `metric` is emitted as a numeric enum value.
- `observation-kind` is used instead of `phase`.
- `pts-ns` and `duration-ns` are emitted with the exact names and GTypes required by the specification.
- `source_id` and `session_id` are not present in the producer-side SoC metric message.
- `count` appears only when the observation kind requires it.
- Invalid or malformed SoC metric messages are rejected before they are accepted or forwarded.
- The corrupted-frame counter is derived from decode-error observations and remains monotonic.
- The current implementation remains intact as a compatibility layer while the Southbound contract is aligned to the normative spec.

## Implementation note

This proposal is intentionally written as a concrete compatibility update, not as a rewrite of the broader playback-metrics design. It states the exact Southbound requirements that must be satisfied while preserving the current architecture and layered implementation model.
