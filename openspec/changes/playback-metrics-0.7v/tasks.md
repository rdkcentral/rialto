# Tasks

## 1. RialtoMetrics Parser Interface and Validation Layer

- [x] 1.1 Update existing `ParsedMetricMessage` struct in `IRialtoMetrics.h` to match the 0.7 contract schema: rename `type` → `metric`, `phase` → `observation_kind`, `ptsNs` → `pts_ns`, `durationNs` → `duration_ns`; remove `counters` field (FrameCounters moved to separate tracking); convert `pts_ns`, `duration_ns`, `count` to `std::optional<>` for conditional presence. **IMPORTANT: `session_id` and `source_id` are NOT part of the SoC media-pipeline-metric message—they are added during Rialto enrichment only.** The struct should have fields for session_id and source_id to store enriched context, but validation must NOT expect them in the incoming GstStructure. Remove legacy field aliases. Write unit tests verifying struct compiles, integrates with existing code, and field types/optionality match spec.

- [x] 1.2 Add validation function signature `GstFlowReturn ValidateMediaPipelineMetric(const GstStructure* msg, ParsedMetricMessage& out)` to `IRialtoMetrics.h`. This function returns GST_FLOW_OK if the message passes all 0.7 contract checks, otherwise returns an error flow and logs the validation failure.

- [x] 1.3 Implement message structure-name validation in `RialtoMetrics.cpp::ValidateMediaPipelineMetric()`. Check that the structure name is exactly `"media-pipeline-metric"`, reject otherwise. Write unit tests verifying rejection of structures named `"metric"`, `"legacy-metric"`, and acceptance of `"media-pipeline-metric"`.

- [x] 1.4 Implement `metric` field validation: require presence as `G_TYPE_UINT`, validate value in range 1–7, reject values 0, 8, or outside range. Write unit tests for all metric values 1–7 (accept), 0, 8, 255 (reject), and missing field (reject).

- [x] 1.5 Implement `observation-kind` field validation: require presence as `G_TYPE_UINT`, validate value in {1, 2, 3}, reject values 0, 4, or outside set. Write unit tests for values 1, 2, 3 (accept), 0, 4, 255 (reject), and missing field (reject).

- [x] 1.6 Implement `pts-ns` field validation: when present, must be `G_TYPE_INT64`, accept any value (positive, zero, negative) as valid PTS. Write unit tests for positive (1000000000), zero (0), negative (-1), missing (accept), and wrong type `G_TYPE_UINT64` (reject).

- [x] 1.7 Implement `duration-ns` field validation: when present, must be `G_TYPE_UINT64`, value must be non-negative (≥ 0). Write unit tests for non-negative values (0, 8000000), negative value rejection, missing field (accept), and wrong type `G_TYPE_INT64` (reject).

- [x] 1.8 Implement `count` field validation: when present, must be `G_TYPE_UINT64`. Do not enforce semantic rules here (those belong in 1.10). Write unit tests for valid values (0, 5, 1000), missing field (accept), and wrong type `G_TYPE_INT64` (reject).

- [x] 1.9 Implement legacy field-name rejection: scan the GstStructure for fields named `"metric_type"`, `"phase"`, `"pts_ns"` (underscore), `"source_id"`, or `"session_id"` and reject the message if any are found. Log validation error. **NOTE: `source_id` and `session_id` are NOT sent by the SoC producer (westeros-sink); they are only added during enrichment in Rialto.** Write unit tests for each legacy name present (reject), and absence (accept). **Update: Remove `source_id` and `session_id` from legacy field rejection as they are not part of the SoC media-pipeline-metric message.** Fields to reject: `"metric_type"`, `"phase"`, `"pts_ns"` (underscore) only.

- [x] 1.10 Implement semantic field-combination validation: reject OCCURRENCE (observation-kind=1) with `duration-ns` or `count` present; reject BUFFER_UNDERFLOW (metric=7) in EPISODE_RESOLVED (observation-kind=3) with `count` present. Write unit tests for all invalid combinations (reject) and valid combinations (VIDEO_FRAME_REPEAT/AUDIO_GAP with count in EPISODE_RESOLVED, etc.).

## 2. GStreamer Message Source Extraction and Enrichment

- [x] 2.1 Add `ElementToSourceMapper` interface to `IRialtoMetrics.h` with methods: `RegisterElement(GstElement* element, uint32_t session_id, uint32_t source_id)` and `MapElement(GstElement* element, uint32_t& out_session_id, uint32_t& out_source_id)`. This mapper maintains the GStreamer element-to-Rialto-context association.

- [x] 2.2 Implement `ElementToSourceMapper` in `RialtoMetrics.cpp` using a concurrent map (e.g., `std::unordered_map` with mutex or thread-safe equivalent) keyed by element pointer. Implement `RegisterElement()` and `MapElement()` with proper locking and error handling (return failure if element not found).

- [x] 2.3 Integrate `ElementToSourceMapper::MapElement()` into `ValidateMediaPipelineMetric()`. Extract the producing element via `GST_MESSAGE_SRC(message)`, call `MapElement()` to retrieve Rialto (session_id, source_id), and populate the enrichment context in the returned `ParsedMetricMessage`. **IMPORTANT:** The incoming GstStructure from the SoC producer does NOT contain source_id or session_id fields. These must be looked up from the GStreamer message source element and mapped via ElementToSourceMapper. If mapping fails, follow the design decision: currently logs a warning and continues with default/placeholder values. Write unit tests for both successful and failed enrichment scenarios.

- [x] 2.4 Add registration hooks to connect GStreamer element creation with `ElementToSourceMapper::RegisterElement()`. This occurs when a decoder or output element is instantiated and linked to a Rialto source. Identify where in the codebase (likely `GstGenericPlayer` or similar) this registration should be called and add the call. Write integration tests to verify registration occurs and enrichment succeeds.

## 3. Parser Integration into Message Flow

- [x] 3.1 Update `RialtoMetrics::OnGstMessage()` or equivalent bus message handler to call the new `ValidateMediaPipelineMetric()` function for all incoming `"media-pipeline-metric"` structures. Reject and discard invalid messages; forward validated messages. Write unit tests simulating bus messages with valid and invalid payloads.

- [x] 3.2 Ensure the enriched `ParsedMetricMessage` from `ValidateMediaPipelineMetric()` is passed to downstream consumers (MediaPipelineServerInternal, IPC serializer). Modify the existing metrics-forwarding interface to carry the enriched data. Verify compilation and existing downstream tests still pass (backward compatibility).

- [x] 3.3 Update logging/debugging in the parser to report validation failures with full context (field name, expected type/value, actual type/value) to aid troubleshooting. Verify log output is readable and consistent for rejection scenarios. Test via unit tests that log output is generated for each failure case.

- [x] 3.4 Update `GstDispatcherThread` to receive `ParsedMetricMessage` with the new field names and optional-field semantics. Ensure the dispatcher forwards the struct without modification (pass-through). Write unit tests for metric flow through the dispatcher.

- [x] 3.5 Update `MediaPipelineServerInternal::onMetricMessage()` (or equivalent handler) to accept the new `ParsedMetricMessage` struct and map its fields to internal event/notification formats. Handle optional fields correctly (pts_ns, duration_ns, count may be absent). Write unit tests for all combinations of optional field presence.

- [x] 3.6 Update the notification event generation in `MediaPipelineServerInternal` to use the new `ParsedMetricMessage` fields (`metric`, `observation_kind`, `pts_ns`, `duration_ns`, `count`) when constructing the event payload. Ensure the event contract matches the new spec field names and types. Write integration tests verifying event notification carries correct data.

## 4. IPC Schema and Serialization

- [x] 4.1 Update `rialto/proto/mediapipelinemodule.proto` with a `MediaPipelineMetric` message containing: required `metric` (uint32), required `observation_kind` (uint32), optional `pts_ns` (int64), optional `duration_ns` (uint64), optional `count` (uint64), optional `session_id` (uint32), optional `source_id` (uint32). Field names use underscores per proto convention. **NOTE:** The SoC producer sends only the core fields (`metric`, `observation_kind`, `pts_ns`, `duration_ns`, `count`). The `session_id` and `source_id` are added by Rialto during enrichment via `ElementToSourceMapper`. The proto message includes all fields for the enriched payload sent to clients.

- [x] 4.2 Implement serialization from `ParsedMetricMessage` to the proto `MediaPipelineMetric` message in `MediaPipelineClient.cpp`. Map the enriched metric fields and optional fields correctly. Write unit tests for all metric types and observation kinds, verifying round-trip serialization/deserialization.

- [x] 4.3 Verify the IPC client deserialization path correctly reads the proto `MediaPipelineMetric` message and reconstructs the metric data for downstream consumers. Write integration tests ensuring end-to-end proto serialization works.

- [x] 4.4 Update `MediaPipelineClient` notification event payload to use the proto `MediaPipelineMetric` message with all fields including the enriched context: `metric`, `observation_kind`, `pts_ns`, `duration_ns`, `count`, `session_id` (enriched), `source_id` (enriched). Ensure clients consuming the event use the new field names. Write integration tests for notification delivery with the updated schema. **NOTE:** The `session_id` and `source_id` should be populated by Rialto before serialization to proto.

- [x] 4.5 Update all downstream event/notification handlers that consume the `ParsedMetricMessage` or proto event (e.g., analytics, logging, metrics aggregation services) to use the new field names and handle optional fields correctly. Verify no regressions in downstream processing. Write tests for each handler consuming the updated message.

## 5. Unit Tests for Validation and Parser

- [ ] 5.1 Add comprehensive unit test suite `RialtoMetricsTest.cpp` with test cases for:
  - Structure-name validation (valid, invalid)
  - Metric field validation (all 1–7 valid, 0/8 invalid, missing, wrong type)
  - Observation-kind field validation (1, 2, 3 valid, 0/4 invalid, missing, wrong type)
  - pts-ns field validation (positive, zero, negative valid, missing, wrong type)
  - duration-ns field validation (non-negative valid, negative invalid, missing, wrong type)
  - count field validation (valid unsigned, missing, wrong type)
  - Legacy field rejection (each legacy name present/absent)
  - Semantic field-combination validation (all invalid combinations, all valid combinations)
  - Source enrichment success and failure cases
  - Parser state recovery after rejection

- [ ] 5.2 Create component-level tests that exercise the parser in the context of the GStreamer bus message loop (if feasible). Include tests for rapid successive messages (valid and invalid), verifying no state corruption.

## 6. Component Test Stubs and End-to-End Integration

- [ ] 6.1 Update `GstreamerStub.cpp` to emit 0.7-compliant `"media-pipeline-metric"` GStreamer structures with the correct field names, types, and enum values. Generate synthetic messages for all 7 metric types with all valid observation-kind combinations.

- [ ] 6.2 Write component tests that feed stub-generated 0.7 metric messages through the full parser-to-IPC pipeline (GStreamer bus → RialtoMetrics::ValidateMediaPipelineMetric() → MediaPipelineServerInternal → IPC serialization). Verify messages are correctly parsed, enriched, and serialized.

- [ ] 6.3 Write regression tests that feed legacy (0.6 or earlier) metric message structures through the parser and verify they are rejected without corrupting state. Include tests for old field names (`metric_type`, `phase`, etc.) and old structure names (`"metric"`).

- [ ] 6.4 Test counter monotonicity: feed multiple `VIDEO_FRAME_DROP` OCCURRENCE messages with incrementing PTS values and verify the parser does not introduce regressions or duplicate counts.

## 7. Documentation and Validation

- [ ] 7.1 Update `rialto/docs/PlaybackMetrics.md` to document the 0.7 `media-pipeline-metric` contract: structure name, required fields (`metric`, `observation-kind`), optional fields (`pts-ns`, `duration-ns`, `count`), observation-kind enum values (1=OCCURRENCE, 2=EPISODE_STARTED, 3=EPISODE_RESOLVED), metric type enum values (1–7), field type requirements, and field semantics per observation-kind. Include note that `source_id` and `session_id` are enriched by Rialto from GST_MESSAGE_SRC(), not present in the SoC payload.

- [ ] 7.2 Add inline code comments in `RialtoMetrics.cpp` explaining the validation flow, error conditions, and design decisions. Ensure future maintainers can understand the contract enforcement and enrichment steps.

- [ ] 7.3 Run `openspec validate --change "playback-metrics-0.7v" --strict` to check spec compliance (requirement descriptions length, scenario format, etc.). Fix any validation warnings.

## 8. Final Integration Verification

- [ ] 8.1 Run all unit tests for RialtoMetrics parser, validation, enrichment, and IPC serialization to ensure they execute successfully.

- [ ] 8.2 Run all component tests for end-to-end metric flow (GStreamer bus → parser → enrichment → IPC). Verify messages reach the downstream consumer correctly and legacy messages are rejected.

- [ ] 8.3 Perform a code review of the parser changes, focusing on contract validation completeness, error handling, and state safety. Address any review feedback and verify follow-up tests pass.

- [ ] 8.4 Verify that the existing downstream tests (MediaPipelineServerInternal, IPC client, etc.) still pass with the new parser output. No backward-compatibility breakage.

## Workflow follow-up

- After all tasks are complete and tested, the change is ready for review and archive.
- Archive the change to create the main spec `openspec/specs/southbound/soc-media-pipeline-metric/spec.md` and finalize the planning.
- Coordinate producer/consumer release to ensure the SoC producer has been updated to emit 0.7-compliant messages before the Rialto parser is deployed.
