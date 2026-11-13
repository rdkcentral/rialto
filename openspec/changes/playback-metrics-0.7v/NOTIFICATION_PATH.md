# Complete Metric Notification Event Path - Updated Structure Flow

This document outlines the complete path that metric events take through the Rialto system, and identifies all points where the updated `MetricEvent` struct (with new 0.7 contract field names) must be used.

## Full Event Flow with Task Mapping

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ Stage 1: GStreamer Bus Message Reception                                   │
│ Task: 3.1 - RialtoMetrics::OnGstMessage()                                  │
└─────────────────────────────────────────────────────────────────────────────┘
    │
    └─> GStreamer message with "media-pipeline-metric" structure
        (contains SoC producer data with 0.7 contract fields)
    
    ✓ Updated struct fields: metric, observation_kind, pts_ns, duration_ns, count
    ✓ Task 3.1: Route to ValidateMediaPipelineMetric()

┌─────────────────────────────────────────────────────────────────────────────┐
│ Stage 2: Validation and Parsing                                             │
│ Tasks: 1.2-1.10 - ValidateMediaPipelineMetric()                            │
└─────────────────────────────────────────────────────────────────────────────┘
    │
    ├─> Task 1.3: Validate structure name = "media-pipeline-metric"
    ├─> Task 1.4: Validate metric field (uint32, range 1-7)
    ├─> Task 1.5: Validate observation-kind field (uint32, range 1-3)
    ├─> Task 1.6: Validate pts-ns field (optional, int64)
    ├─> Task 1.7: Validate duration-ns field (optional, uint64)
    ├─> Task 1.8: Validate count field (optional, uint64)
    ├─> Task 1.9: Reject legacy field names
    ├─> Task 1.10: Validate semantic field combinations
    │
    └─> Returns: MetricEvent with validated data
        (struct fields: metric, observation_kind, pts_ns, duration_ns, count)
    
    ✓ Output struct uses NEW field names (not old type/phase/ptsNs)

┌─────────────────────────────────────────────────────────────────────────────┐
│ Stage 3: Source Enrichment                                                  │
│ Tasks: 2.1-2.3 - ElementToSourceMapper Integration                         │
└─────────────────────────────────────────────────────────────────────────────┘
    │
    ├─> Task 2.1: ElementToSourceMapper interface defined
    ├─> Task 2.2: ElementToSourceMapper implemented (concurrent map)
    ├─> Task 2.3: Extract GST_MESSAGE_SRC() and map to (session_id, source_id)
    │
    └─> Returns: MetricEvent enriched with Rialto context
        (struct fields: metric, observation_kind, pts_ns, duration_ns, count,
                        session_id, source_id)
    
    ✓ Enrichment context added without modifying SoC message contract

┌─────────────────────────────────────────────────────────────────────────────┐
│ Stage 4: Dispatcher Pass-Through                                            │
│ Task: 3.4 - GstDispatcherThread                                            │
└─────────────────────────────────────────────────────────────────────────────┘
    │
    ├─> Task 3.4: Forward MetricEvent without modification
    │   (pass-through with no field transformations)
    │
    └─> Output: MetricEvent (unchanged)
        (struct fields: metric, observation_kind, pts_ns, duration_ns, count,
                        session_id, source_id)
    
    ✓ Dispatcher preserves all struct data

┌─────────────────────────────────────────────────────────────────────────────┐
│ Stage 5: Metrics Handler and Event Notification                             │
│ Tasks: 3.5-3.6 - MediaPipelineServerInternal                               │
└─────────────────────────────────────────────────────────────────────────────┘
    │
    ├─> Task 3.5: MediaPipelineServerInternal::onMetricMessage()
    │   Receives MetricEvent and maps to internal event format
    │   (handles optional fields: pts_ns, duration_ns, count)
    │
    └─> Task 3.6: Notification event generation
        (creates event payload with new field names:
         metric, observation_kind, pts_ns, duration_ns, count,
         session_id, source_id)
    
    ✓ Event payload uses NEW field names from updated struct

┌─────────────────────────────────────────────────────────────────────────────┐
│ Stage 6: IPC Proto Serialization                                            │
│ Tasks: 4.1-4.3 - MediaPipelineClient IPC Layer                             │
└─────────────────────────────────────────────────────────────────────────────┘
    │
    ├─> Task 4.1: Proto schema updated with MediaPipelineMetric message
    │   (proto fields: metric, observation_kind, pts_ns, duration_ns, count,
    │                  session_id, source_id - all with underscores per proto)
    │
    ├─> Task 4.2: Serialize MetricEvent → proto MediaPipelineMetricEvent
    │   (map struct fields to proto fields, handle optional fields)
    │
    └─> Task 4.3: Verify deserialization path
        (proto → internal representation, field mapping correct)
    
    ✓ IPC payload carries struct data with NEW field names

┌─────────────────────────────────────────────────────────────────────────────┐
│ Stage 7: Event Notification to Clients                                      │
│ Task: 4.4 - MediaPipelineClient Event Notification                         │
└─────────────────────────────────────────────────────────────────────────────┘
    │
    ├─> Task 4.4: Notification event payload uses proto MediaPipelineMetric
    │   (clients receive event with fields:
    │    metric, observation_kind, pts_ns, duration_ns, count,
    │    session_id, source_id)
    │
    └─> Clients receive event notification with NEW field names
    
    ✓ Event contract updated to match new struct

┌─────────────────────────────────────────────────────────────────────────────┐
│ Stage 8: Downstream Handlers and Consumers                                  │
│ Task: 4.5 - Downstream Event/Notification Handlers                         │
└─────────────────────────────────────────────────────────────────────────────┘
    │
    ├─> Analytics services consume event with NEW field names
    ├─> Logging services consume event with NEW field names
    ├─> Metrics aggregation services consume event with NEW field names
    ├─> Other clients consuming MediaPipelineMetric proto message
    │
    └─> All handlers updated to:
        ✓ Use new field names (metric, observation_kind, pts_ns, etc.)
        ✓ Handle optional fields correctly
        ✓ No regressions in downstream processing

```

## Struct Evolution Summary

### Before (Legacy)
```cpp
struct MetricEvent {
    int32_t sessionId;
    uint32_t type;           // ← RENAMED TO: metric
    uint32_t phase;          // ← RENAMED TO: observation_kind
    int32_t sourceId;
    uint64_t ptsNs;          // ← RENAMED TO: pts_ns (and made optional)
    FrameCounters counters;  // ← REMOVED
    uint64_t count;          // ← MADE OPTIONAL
    uint64_t durationNs;     // ← RENAMED TO: duration_ns (and made optional)
};
```

### After (0.7 Contract)
```cpp
struct MetricEvent {
    uint32_t metric;                            // required
    uint32_t observation_kind;                  // required
    std::optional<int64_t> pts_ns;             // optional
    std::optional<uint64_t> duration_ns;       // optional
    std::optional<uint64_t> count;             // optional
    uint32_t session_id;                        // enriched by Rialto
    uint32_t source_id;                         // enriched by Rialto
};
```

## Key Checkpoints for Integration

| Task | Component | Field Name Updates | Optional Field Handling | Test Coverage |
|------|-----------|-------------------|------------------------|---|
| 3.1  | RialtoMetrics parser | metric, observation_kind, pts_ns, duration_ns, count | Handle std::optional extraction | Unit tests for message routing |
| 3.4  | GstDispatcherThread | Pass-through (no changes) | Preserve optional field state | Unit tests for dispatcher flow |
| 3.5  | MediaPipelineServerInternal::onMetricMessage() | Map to internal event fields | Handle absent fields gracefully | Unit tests for all optional combos |
| 3.6  | Notification event generation | Use new field names in payload | Include/exclude optional fields based on presence | Integration tests for event data |
| 4.2  | IPC serialization | Map struct fields → proto fields | Serialize optional fields correctly | Round-trip serialization tests |
| 4.4  | IPC client notifications | Event payload uses proto schema | Clients receive correct data | Integration tests for event delivery |
| 4.5  | Downstream handlers | All handlers use new field names | Process optional fields correctly | Tests per downstream consumer |

## Validation Strategy

The complete path validation includes:

1. **Struct Definition (Task 1.1)**
   - ✓ Correct field names and types
   - ✓ std::optional usage for conditional fields
   - ✓ Integration with existing code

2. **Parser Validation (Tasks 1.2-1.10)**
   - ✓ Message conforms to 0.7 contract
   - ✓ All required fields present and correct type
   - ✓ Optional fields validated when present
   - ✓ Semantic constraints enforced
   - ✓ Legacy fields rejected

3. **End-to-End Flow (Tasks 3.1-3.6, 4.1-4.5)**
   - ✓ Updated struct flows through entire pipeline
   - ✓ Dispatcher passes data unchanged
   - ✓ Event handler uses new field names
   - ✓ Notification carries correct data
   - ✓ Clients receive events with new schema
   - ✓ Downstream handlers process new fields

## Backward Compatibility Considerations

- Existing downstream tests must pass (Task 3.2)
- Proto schema update may require client updates
- Field name changes require coordination with all consumers (Task 4.5)
- Optional field presence requires defensive coding in handlers

## Total Task Count Impact

- **Before**: 33 tasks (with separate ValidatedMediaPipelineMetric struct)
- **After**: 38 tasks (updating existing struct + complete notification path)
  - Group 1: 10 tasks (validation layer)
  - Group 2: 4 tasks (source enrichment)
  - Group 3: 6 tasks (message flow - **3 new**)
  - Group 4: 5 tasks (IPC schema - **2 new**)
  - Group 5: 2 tasks (unit tests)
  - Group 6: 4 tasks (component tests)
  - Group 7: 3 tasks (documentation)
  - Group 8: 4 tasks (integration verification)
