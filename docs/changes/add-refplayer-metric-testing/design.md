## Context
The public `MetricEvent` and `IMediaPipelineClient::notifyMetric` callback already carry playback metric type, phase, source, timestamps, and optional event data. `docs/PlaybackMetrics.md` documents the current server producer, IPC relay, and client callback contract. The reference player implements `IMediaPipelineClient` in `PipelineCallbacks`, but currently does not handle `notifyMetric`; its CLI waits for player states and has no metric assertion or deterministic trigger.

This change is intended to verify the existing notification contract with a real player/server pair. The reference player controls the media input and can create fault stimuli; the downstream decoder/renderer and Rialto metric producer remain responsible for detecting and reporting actual faults.

## Goals / Non-Goals
- Goals: receive and record `notifyMetric`; make the CLI fail when an expected type is absent; create controlled media-input fault stimuli in the reference player; exercise the ordinary server-to-client notification path.
- Non-Goals: change metric type/phase semantics, alter QoS or frame-counter behavior, fabricate `MetricEvent`s in the client, replace `notifyBufferUnderflow()`, or make the standalone mock host test proof of IPC delivery.

## Decisions
- Decision: Match `--expect-metric` by metric type and require at least one matching callback before the metric wait expires. Phases and optional fields remain available in the event and logs, but do not complicate this initial CLI assertion.
- Decision: Add `--inject-fault <type>` as a reference-player input stimulus, active only during loaded playback. It changes feeder timing or one encoded access unit; it does not construct or send a `MetricEvent` itself.
- Decision: Support the requested video frame drop, video decode error, video frame repeat, and buffer underflow stimuli. Require `--expect-metric <type>` to observe the corresponding actual callback, using the existing `MetricType` and `MetricPhase` contract.
- Decision: Preserve frame identity and intent in each stimulus. A late-frame stimulus delays delivery past its presentation deadline rather than omitting a sample; a decode-error stimulus corrupts one selected access unit; a repeat stimulus stalls the next frame while the previous frame is available; an underflow stimulus pauses feeding while playback is active until decoder input starvation occurs.
- Decision: Do not add a server injection hook or public media API for fault creation. The existing server metric producer, validation, IPC routing, and `notifyMetric` callback stay authoritative.
- Decision: Keep metric assertions separate from the legacy `notifyBufferUnderflow()` callback and from `notifyNeedMediaData()` flow control. A `BUFFER_UNDERFLOW` metric represents decoder input starvation and uses the metric event's episode phases; it is not inferred from either callback.
- Decision: Require a URL and active pipeline for injection and metric expectation. Start the expectation before triggering injection so asynchronous delivery cannot race past the test.

### Fault meanings covered
- Video frame drop: a presentation-eligible video frame irreversibly discarded before presentation, whether shed before decode or decoded but past its presentation deadline. Seek removal, source flush, EOS cleanup, and source reconfiguration are not drops.
- Video decode error: an authoritative decoder's frame-level decode failure/corruption report, counted once per affected frame.
- Video frame repeat: renderer reuse of the preceding frame because the next frame is unavailable. Structured cadence such as 3:2 or 2:2 is excluded.
- Buffer underflow: decoder input starvation, represented as an episode that resolves when starvation ends; independent of need-data flow control and the retained callback.

These are input stimuli, not guarantees that every platform will create or report the intended fault. The test passes only when the expected metric actually arrives. In particular, skipping an access unit in the reference player is not a valid video-frame-drop stimulus: the pipeline did not receive a presentation-eligible frame to discard.

## Risks / Trade-offs
- Some devices may not expose all four fault metrics, and input corruption/timing may lead to a different playback outcome. Mitigation: use target-qualified test media and bounded stimuli, report the requested and observed metric types, and treat absence as a failed metric test rather than fabricating success.
- Client-side sample omission could be mistaken for a pipeline frame drop. Mitigation: delay a valid frame beyond its presentation deadline; do not skip it in the feeder for this case.
- Corrupting compressed media can terminate playback on some decoders instead of generating a frame-level metric. Mitigation: use a known test access unit and validate each supported decoder/platform combination.
- Metric delivery is asynchronous. Mitigation: arm the expectation before injection and use the CLI's bounded timeout; report the missing type on failure.
- The meaning of “a single repeat still produces a pair” needs clarification against the existing event schema. Occurrence messages currently disallow `count`, while episode-resolved messages can carry it. Resolve whether “pair” means a rendered/repeated frame pair, two notifications, or a particular count before asserting cardinality or payload fields. Type-only expectation is unaffected.

## Migration Plan
1. Add callback recording and the CLI expected-metric wait.
2. Add controlled feeder stimuli and the CLI `--inject-fault` option.
3. Validate each supported stimulus against a real Rialto client/server on target platforms that produce the corresponding metric.

No data migration or rollback is required. Without fault-stimulus options, the player continues feeding media as before.

## Open Questions
- What does “a single repeat still produces a pair” require at the event/payload level? The expected-metric CLI checks type only, but injection tests that assert count or event cardinality need an agreed interpretation.
- Which supported target/decoder combinations can reliably emit each metric in response to the corresponding feeder stimulus?