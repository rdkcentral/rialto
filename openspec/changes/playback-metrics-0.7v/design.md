# Design

## Context

Rialto's Southbound metric parser currently expects legacy GStreamer message structures with field names like `metric_type`, `phase`, `pts_ns` (underscore), and payload-embedded `source_id`/`session_id`. The SoC producer (native Broadcom/Nexus stack) now emits a normalized 0.7 contract with `media-pipeline-metric` structure, fields `metric`, `observation-kind`, `pts-ns` (hyphen), and optional `duration-ns`, `count`.

The parser does not validate the message structure name and continues to accept legacy field shapes. This creates a contract mismatch: messages conform to the 0.7 spec but the parser cannot correctly validate or forward them because it still listens for the old names and semantics.

The RialtoMetrics parser in `rialto/media/server/metrics/source/RialtoMetrics.cpp` is the primary handler for incoming bus messages. It currently:
- Does not check structure name
- Reads `metric_type` instead of `metric`
- Reads `phase` instead of `observation-kind`
- Does not validate field types or enum ranges
- Optionally reads `source_id` and `session_id` from the message payload (but 0.7 moves these to Rialto enrichment via GST_MESSAGE_SRC)
- Does not reject messages with unknown or legacy field names

The downstream flow through GstDispatcherThread, MediaPipelineServerInternal, and the IPC serializer follows the parser's output.

## Goals / Non-Goals

**Goals:**

- Align the parser to the normative 0.7 `media-pipeline-metric` contract: structure name, field names, types, and semantics.
- Validate every incoming message for conformance: required fields present, correct GTypes, authorized enum values, semantically valid field combinations.
- Reject legacy field names and unknown structures before they reach downstream consumers.
- Extract and map the GStreamer message source element (GST_MESSAGE_SRC) to Rialto's internal session/source context without modifying the normative SoC message structure.
- Ensure the parser state remains valid after rejecting any malformed message (no corruption or leakage to subsequent messages).
- Maintain backward compatibility in the downstream forwarding path: once a metric passes validation, it is forwarded unchanged to the existing MediaPipelineServerInternal and IPC serialization paths.

**Non-Goals:**

- Redesign the playback-metrics architecture or event flow.
- Add new metric types or observation kinds beyond the 0.7 contract.
- Broaden the change into subscription management, notification-loss handling, or client-side analytics.
- Introduce a parallel metric schema or alternate validation path.
- Migrate the northbound or V1.1 API.

## Decisions

### Decision 1: Validation Layer - Strict Contract Enforcement at Parser Entry

**Chosen Approach:** Implement a dedicated validation layer in RialtoMetrics that checks message structure name, field presence, GTypes, enum ranges, and semantic consistency before the metric is interpreted or forwarded.

**Rationale:**
- The parser is the first point where the SoC bus message arrives; validation here prevents invalid data from propagating downstream.
- A single, centralized validation function is easier to test and audit for contract compliance.
- Rejecting invalid messages at the boundary is cleaner than scattering checks throughout the downstream flow.

**Alternatives Considered:**
- *Lenient parsing + downstream validation:* Accept both legacy and 0.7 field names, validate downstream. Risk: invalid messages leak into the pipeline; harder to debug when they fail mid-flow.
- *Runtime fallback:* Try to read 0.7 fields first, then legacy fields if missing. Risk: ambiguity and silent degradation; no clear error signal if a message is malformed.
- *Chosen approach wins:* Clear, fast rejection; no silent failures.

### Decision 2: Source Element Extraction and Enrichment

**Chosen Approach:** Extract GST_MESSAGE_SRC() to identify the producing GStreamer element, maintain a mapping of elements to Rialto (session_id, source_id) pairs, and enrich the parsed metric object with Rialto context before forwarding. The normative SoC `media-pipeline-metric` structure remains unmodified; no `source_id` or `session_id` fields are added to it.

**Rationale:**
- The SoC producer does not carry source/session identifiers in the message; the 0.7 contract is minimal and contract-agnostic of Rialto's internal naming.
- Rialto owns the element-to-source mapping; enrichment at the parser boundary is the natural place to add this context.
- Keeping the SoC message unmodified preserves the contract fidelity and makes it easier to validate and debug.

**Alternatives Considered:**
- *Modify the SoC message to add Rialto source_id/session_id:* Violates the normative contract; makes the message Rialto-specific instead of contract-compliant.
- *Extract source in downstream paths (MediaPipelineServerInternal or IPC):* Defers the mapping responsibility, risking inconsistency and making tracing harder.
- *Chosen approach wins:* Clear separation of concerns; contract unmodified; enrichment at the right boundary.

### Decision 3: Legacy Field Name Rejection

**Chosen Approach:** Explicitly check for and reject any message containing legacy field names (`metric_type`, `phase`, `pts_ns`, `source_id`, `session_id`) before the metric enters the processing path. Log a validation error and discard the message.

**Rationale:**
- A message with legacy field names is a clear signal of a non-conformant or old producer.
- Rejecting such messages prevents silent data corruption (e.g., reading the wrong field).
- Explicit rejection makes it obvious in logs when a mismatch occurs, aiding debugging.

**Alternatives Considered:**
- *Ignore legacy fields silently:* Risk of silent corruption if old and new fields coexist or if the producer uses wrong names.
- *Support both field name sets:* Creates ambiguity and makes the parser test matrix complex.
- *Chosen approach wins:* Explicit, auditable, and prevents subtle bugs.

### Decision 4: Field Type Validation Using GStreamer GValue API

**Chosen Approach:** Use GStreamer's GValue type checking functions (e.g., `g_value_type() == G_TYPE_UINT`, `g_value_get_uint()`) to validate field types and extract values. Reject any message with a field that has an unexpected GType.

**Rationale:**
- GStreamer structures are weakly typed at the GValue level; a field named `metric` could be any GType.
- Explicit type checking ensures the contract is enforced; a `G_TYPE_INT64` field masquerading as `metric` will not be accepted.
- Type mismatches are often a sign of a producer or message-generation bug; rejecting them helps catch such issues early.

**Alternatives Considered:**
- *Type coercion:* Cast or convert types if they are "close" (e.g., G_TYPE_INT64 → G_TYPE_UINT). Risk: masks bugs and introduces silent data loss.
- *Chosen approach wins:* No silent conversions; strict validation.

### Decision 5: Semantic Validation per Observation-Kind

**Chosen Approach:** After confirming field presence and types, validate observation-kind-specific rules (e.g., OCCURRENCE messages MUST NOT carry `duration-ns` or `count`; BUFFER_UNDERFLOW EPISODE_RESOLVED MUST NOT carry `count`). Reject messages that violate these rules.

**Rationale:**
- The 0.7 spec defines field semantics per observation-kind; a message that violates these semantics is non-conformant.
- Semantic validation ensures that downstream consumers (e.g., analytics, metrics aggregation) can rely on the structure.

**Alternatives Considered:**
- *Permissive semantics:* Accept any field combination and let the downstream handle inconsistencies. Risk: lost information, harder to debug.
- *Documentation-only semantics:* No validation, rely on producer correctness. Risk: silent failures and data loss.
- *Chosen approach wins:* Explicit, unambiguous.

### Decision 6: Parser State Recovery After Rejection

**Chosen Approach:** Ensure that rejecting a message does not corrupt the parser's state. Use localized, exception-safe parsing (no partial updates to shared state). After a rejection, the parser is ready to process the next message without side effects.

**Rationale:**
- A malformed message should not poison the entire parser.
- Robust error handling is critical for a long-running service.

**Alternatives Considered:**
- *State rollback:* Attempt to undo any parsing state changes. Complex and error-prone.
- *Chosen approach wins:* Avoid state changes until validation succeeds.

## Risks / Trade-offs

| Risk | Mitigation |
|------|-----------|
| **Unknown GStreamer element source in enrichment** — A metric arrives from a GStreamer element not yet in the session/source mapping (e.g., element was created but not registered with Rialto). | Behavior is implementation-defined: reject the message, log a warning and queue for retry, or enrich with a placeholder. Document the chosen behavior in MediaPipelineServerInternal and add tests to cover this case. If frequent, add a debug callback to register elements at creation time. |
| **Performance impact of strict validation** — Validating every field on every message may add latency. | Validation is O(field count) and metrics are not high-frequency (typically < 100/sec). Benchmark after implementation; profile-guided optimization if needed. |
| **Backward compatibility during rollout** — Old producers still emit legacy messages; new parser rejects them. | Transition strategy: add a compatibility mode flag (temporarily accept legacy names with a deprecation warning), then remove after a grace period. Or coordinate producer/consumer release cycle. Document in rollback plan. |
| **Enum value extensions in the future** — If the 0.7 spec adds metric types (values > 7) or observation kinds (values > 3), the current range checks will reject them prematurely. | The 0.7 spec is dated 2026-11-12; this design is locked to that version. Future spec versions warrant new proposal/change. For now, document the locked range and re-review at spec update. |
| **Validation error logging verbosity** — Excessive logging of rejected messages in production (e.g., if a misconfigured producer floods the bus). | Use adaptive logging: log the first N validation errors, then summarize ("N more validation errors of this type"). Add a metric counter for rejected messages. |

## Migration Plan

1. **Implementation phase:** Update RialtoMetrics parser to validate 0.7 contract; update GstDispatcherThread and MediaPipelineServerInternal to use the new parser interface.

2. **Unit test phase:** Write unit tests for all validation rules, both passing and failing cases. Test all metric types, observation kinds, and field combinations.

3. **Component/integration test phase:** Update GStreamerStub to emit 0.7-compliant messages; run end-to-end tests with both valid and invalid messages to verify rejection and forwarding.

4. **Rollback strategy:** If issues arise, revert the parser changes to restore legacy field-name acceptance. Keep the new structure validation in place (it should not break 0.7 messages). Document any discovered incompatibilities in a new proposal.

## Open Questions

1. **Unknown element source enrichment:** What is the precise behavior when GST_MESSAGE_SRC() maps to an unknown element? Should we reject, retry, or use a placeholder source_id? (Answer needed before task implementation.)
2. **Compatibility flag:** Should we support a temporary compatibility mode to accept legacy field names during rollout, or does the release cycle guarantee producer/consumer sync? (Affects validation implementation complexity.)
3. **Logging strategy for validation errors:** How verbose should rejection logging be? Per-error or summarized? (Answer needed for operational observability.)

These questions can be answered in the design-review phase before task execution without changing the specification or approach.
