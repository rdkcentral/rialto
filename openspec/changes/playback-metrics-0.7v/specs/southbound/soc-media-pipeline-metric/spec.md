# Spec: SoC Media Pipeline Metric (0.7 Normative Contract)

## Overview

This spec defines the normative structure and semantics for the `media-pipeline-metric` message emitted by the SoC (System-on-Chip) producer and received by the Rialto Southbound metric parser. The 0.7 contract is a simplified, contract-agnostic schema that normalizes metric data into a canonical form for consumption by Rialto.

### Structure Name
- **Structure Name**: `media-pipeline-metric` (exact string match; must be in the message structure, not a type name)
- **Source**: SoC producer (Broadcom/Nexus native stack)
- **Receiver**: Rialto RialtoMetrics parser (via GStreamer bus)
- **Semantics**: Each message represents a discrete observation or event in the media playback pipeline.

---

## Requirements

### REQUIREMENT 1: Structure Presence and Name Validation [ADDED]

**Statement:** The parser MUST validate that every incoming GStreamer message contains a structure with the exact name `media-pipeline-metric`.

**Rationale:** The SoC producer must emit this specific structure name to signal contract compliance. Any message with a different structure name (e.g., legacy `metric`, custom structures, or malformed messages) is not 0.7-conformant and must be rejected.

**Acceptance Criteria:**

| Scenario | Input | Expected Behavior |
|----------|-------|-------------------|
| **1.1.1 Valid structure name** | Message with structure `media-pipeline-metric` | Accept message, proceed to field validation |
| **1.1.2 Legacy structure name** | Message with structure `metric` (pre-0.7) | Reject; log "invalid structure name" |
| **1.1.3 Unknown structure name** | Message with structure `custom-metric` | Reject; log "invalid structure name" |
| **1.1.4 No structure** | Message with no attached structure | Reject; log "no structure found" |
| **1.1.5 Null or empty name** | Structure with NULL or empty name | Reject; log "invalid structure name" |

---

### REQUIREMENT 2: Required Field Presence - "metric" [ADDED]

**Statement:** The parser MUST validate that every `media-pipeline-metric` structure contains a field named `metric` with type `G_TYPE_UINT`.

**Rationale:** The `metric` field identifies the type of observation (e.g., 1=PLAYBACK_STARTED, 7=BUFFER_UNDERFLOW). This field is mandatory and must be unsigned 32-bit integer per the SoC contract.

**Acceptance Criteria:**

| Scenario | Input | Expected Behavior |
|----------|-------|-------------------|
| **2.1.1 Valid metric field (value 1)** | Structure with `metric` = 1, type `G_TYPE_UINT` | Accept; extract value 1 |
| **2.1.2 Valid metric field (value 7)** | Structure with `metric` = 7, type `G_TYPE_UINT` | Accept; extract value 7 |
| **2.1.3 Missing metric field** | Structure without `metric` field | Reject; log "missing field: metric" |
| **2.1.4 Wrong type (int64)** | Structure with `metric` = 1, type `G_TYPE_INT64` | Reject; log "field metric has wrong type: G_TYPE_INT64, expected G_TYPE_UINT" |
| **2.1.5 Wrong type (double)** | Structure with `metric` = 1.5, type `G_TYPE_DOUBLE` | Reject; log "field metric has wrong type" |
| **2.1.6 Metric field named differently** | Structure with `metric_type` (legacy) instead of `metric` | Reject; log "missing field: metric"; also log warning for legacy field |

---

### REQUIREMENT 3: Required Field Presence - "observation-kind" [ADDED]

**Statement:** The parser MUST validate that every `media-pipeline-metric` structure contains a field named `observation-kind` with type `G_TYPE_UINT`.

**Rationale:** The `observation-kind` field specifies the semantic context of the metric (1=OCCURRENCE, 2=BUFFER_UNDERFLOW_START, 3=BUFFER_UNDERFLOW_RESOLVED). This field is mandatory.

**Acceptance Criteria:**

| Scenario | Input | Expected Behavior |
|----------|-------|-------------------|
| **3.1.1 Valid observation-kind (value 1)** | Structure with `observation-kind` = 1, type `G_TYPE_UINT` | Accept; extract value 1 |
| **3.1.2 Valid observation-kind (value 2)** | Structure with `observation-kind` = 2, type `G_TYPE_UINT` | Accept; extract value 2 |
| **3.1.3 Valid observation-kind (value 3)** | Structure with `observation-kind` = 3, type `G_TYPE_UINT` | Accept; extract value 3 |
| **3.1.4 Missing observation-kind field** | Structure without `observation-kind` field | Reject; log "missing field: observation-kind" |
| **3.1.5 Wrong type (int32)** | Structure with `observation-kind` = 1, type `G_TYPE_INT` | Reject; log "field observation-kind has wrong type" |
| **3.1.6 Legacy field name** | Structure with `phase` instead of `observation-kind` | Reject; log "missing field: observation-kind"; also log warning for legacy field |

---

### REQUIREMENT 4: Optional Field "pts-ns" Validation [ADDED]

**Statement:** If the `media-pipeline-metric` structure contains a field named `pts-ns`, it MUST have type `G_TYPE_INT64`. If present, the parser must extract and validate the presentation timestamp nanoseconds value. If absent, the parser must accept the message without this field.

**Rationale:** The `pts-ns` field carries presentation timestamp data for metrics that need precise timing. It is optional; some metrics (e.g., PLAYBACK_STARTED) may omit it. When present, it must be a signed 64-bit integer per the SoC contract.

**Acceptance Criteria:**

| Scenario | Input | Expected Behavior |
|----------|-------|-------------------|
| **4.1.1 Valid pts-ns (positive value)** | Structure with `pts-ns` = 1000000, type `G_TYPE_INT64` | Accept; extract value 1000000 |
| **4.1.2 Valid pts-ns (zero)** | Structure with `pts-ns` = 0, type `G_TYPE_INT64` | Accept; extract value 0 |
| **4.1.3 Valid pts-ns (negative value)** | Structure with `pts-ns` = -1000, type `G_TYPE_INT64` | Accept; extract value -1000 (valid per contract) |
| **4.1.4 Missing pts-ns (optional)** | Structure without `pts-ns` field | Accept; treat as not present (no error) |
| **4.1.5 Wrong type (uint64)** | Structure with `pts-ns` = 1000000, type `G_TYPE_UINT64` | Reject; log "field pts-ns has wrong type: G_TYPE_UINT64, expected G_TYPE_INT64" |
| **4.1.6 Wrong type (double)** | Structure with `pts-ns` = 1000000.5, type `G_TYPE_DOUBLE` | Reject; log "field pts-ns has wrong type" |
| **4.1.7 Legacy field name** | Structure with `pts_ns` (underscore, not hyphen) | Treat as legacy and reject entire message; log "legacy field detected: pts_ns" |

---

### REQUIREMENT 5: Optional Field "duration-ns" Validation [ADDED]

**Statement:** If the `media-pipeline-metric` structure contains a field named `duration-ns`, it MUST have type `G_TYPE_UINT64` and the value MUST be >= 0. If present, the parser must extract and validate this field. If absent, the parser must accept the message without this field.

**Rationale:** The `duration-ns` field indicates the duration of an observation in nanoseconds. It is optional and only meaningful for certain observation kinds (e.g., BUFFER_UNDERFLOW_START with a known duration). It must be unsigned (non-negative) per the SoC contract.

**Acceptance Criteria:**

| Scenario | Input | Expected Behavior |
|----------|-------|-------------------|
| **5.1.1 Valid duration-ns (positive value)** | Structure with `duration-ns` = 5000000, type `G_TYPE_UINT64` | Accept; extract value 5000000 |
| **5.1.2 Valid duration-ns (zero)** | Structure with `duration-ns` = 0, type `G_TYPE_UINT64` | Accept; extract value 0 |
| **5.1.3 Missing duration-ns (optional)** | Structure without `duration-ns` field | Accept; treat as not present |
| **5.1.4 Wrong type (int64)** | Structure with `duration-ns` = 5000000, type `G_TYPE_INT64` | Reject; log "field duration-ns has wrong type: G_TYPE_INT64, expected G_TYPE_UINT64" |
| **5.1.5 Wrong type (double)** | Structure with `duration-ns` = 5000000.5, type `G_TYPE_DOUBLE` | Reject; log "field duration-ns has wrong type" |

---

### REQUIREMENT 6: Optional Field "count" Validation [ADDED]

**Statement:** If the `media-pipeline-metric` structure contains a field named `count`, it MUST have type `G_TYPE_UINT64` and the value MUST be >= 1. If present, the parser must extract and validate this field. If absent, the parser must accept the message without this field.

**Rationale:** The `count` field indicates a repetition or aggregation count for certain observations (e.g., "underflow occurred N times"). It is optional and must be a positive unsigned integer (value >= 1) per the SoC contract.

**Acceptance Criteria:**

| Scenario | Input | Expected Behavior |
|----------|-------|-------------------|
| **6.1.1 Valid count (value 1)** | Structure with `count` = 1, type `G_TYPE_UINT64` | Accept; extract value 1 |
| **6.1.2 Valid count (large value)** | Structure with `count` = 1000000, type `G_TYPE_UINT64` | Accept; extract value 1000000 |
| **6.1.3 Missing count (optional)** | Structure without `count` field | Accept; treat as not present |
| **6.1.4 Wrong type (int64)** | Structure with `count` = 1, type `G_TYPE_INT64` | Reject; log "field count has wrong type: G_TYPE_INT64, expected G_TYPE_UINT64" |
| **6.1.5 Invalid value (zero)** | Structure with `count` = 0, type `G_TYPE_UINT64` | Reject; log "field count must be >= 1, got 0" |
| **6.1.6 Wrong type (double)** | Structure with `count` = 1.5, type `G_TYPE_DOUBLE` | Reject; log "field count has wrong type" |

---

### REQUIREMENT 7: Metric Enum Range Validation [ADDED]

**Statement:** The parser MUST validate that the `metric` field value is within the authorized range [1, 7], corresponding to defined SoC metric types.

**Rationale:** Metric types are enumerated in the SoC contract: 1=PLAYBACK_STARTED, 2=PLAYBACK_PAUSED, ..., 7=BUFFER_UNDERFLOW. Values outside this range are either reserved for future use or invalid. Rejecting out-of-range values prevents downstream processing of undefined metrics.

**Acceptance Criteria:**

| Scenario | Input | Expected Behavior |
|----------|-------|-------------------|
| **7.1.1 Valid metric (value 1)** | Structure with `metric` = 1 | Accept |
| **7.1.2 Valid metric (value 7)** | Structure with `metric` = 7 | Accept |
| **7.1.3 Invalid metric (value 0)** | Structure with `metric` = 0 | Reject; log "field metric must be in range [1, 7], got 0" |
| **7.1.4 Invalid metric (value 8)** | Structure with `metric` = 8 | Reject; log "field metric must be in range [1, 7], got 8" |
| **7.1.5 Invalid metric (value 255)** | Structure with `metric` = 255 | Reject; log "field metric out of range" |
| **7.1.6 Invalid metric (negative, wrapped)** | Structure with `metric` = 4294967295 (large uint) | Reject if interpreted outside [1, 7] |

---

### REQUIREMENT 8: Observation-Kind Enum Range Validation [ADDED]

**Statement:** The parser MUST validate that the `observation-kind` field value is within the authorized range [1, 3], corresponding to defined SoC observation types.

**Rationale:** Observation kinds are enumerated: 1=OCCURRENCE, 2=BUFFER_UNDERFLOW_START, 3=BUFFER_UNDERFLOW_RESOLVED. Values outside this range are invalid or unknown.

**Acceptance Criteria:**

| Scenario | Input | Expected Behavior |
|----------|-------|-------------------|
| **8.1.1 Valid observation-kind (value 1)** | Structure with `observation-kind` = 1 | Accept |
| **8.1.2 Valid observation-kind (value 2)** | Structure with `observation-kind` = 2 | Accept |
| **8.1.3 Valid observation-kind (value 3)** | Structure with `observation-kind` = 3 | Accept |
| **8.1.4 Invalid observation-kind (value 0)** | Structure with `observation-kind` = 0 | Reject; log "field observation-kind must be in range [1, 3], got 0" |
| **8.1.5 Invalid observation-kind (value 4)** | Structure with `observation-kind` = 4 | Reject; log "field observation-kind must be in range [1, 3], got 4" |
| **8.1.6 Invalid observation-kind (large value)** | Structure with `observation-kind` = 255 | Reject; log "field observation-kind out of range" |

---

### REQUIREMENT 9: Semantic Constraint - OCCURRENCE (kind=1) Must Not Have Duration or Count [ADDED]

**Statement:** If `observation-kind` = 1 (OCCURRENCE), the message MUST NOT contain `duration-ns` or `count` fields. The parser MUST validate this constraint.

**Rationale:** OCCURRENCE metrics represent a point-in-time event (e.g., PLAYBACK_STARTED), not a duration-bound or repeating event. Presence of duration or count fields in an OCCURRENCE message indicates a schema error or producer bug.

**Acceptance Criteria:**

| Scenario | Input | Expected Behavior |
|----------|-------|-------------------|
| **9.1.1 OCCURRENCE without duration or count** | Structure with `observation-kind` = 1, no `duration-ns`, no `count` | Accept |
| **9.1.2 OCCURRENCE with duration-ns** | Structure with `observation-kind` = 1, `duration-ns` = 1000 | Reject; log "semantic error: OCCURRENCE (kind=1) must not have duration-ns" |
| **9.1.3 OCCURRENCE with count** | Structure with `observation-kind` = 1, `count` = 2 | Reject; log "semantic error: OCCURRENCE (kind=1) must not have count" |
| **9.1.4 OCCURRENCE with both duration and count** | Structure with `observation-kind` = 1, `duration-ns` = 1000, `count` = 2 | Reject; log "semantic error: OCCURRENCE (kind=1) must not have duration-ns" (first violation) |

---

### REQUIREMENT 10: Semantic Constraint - BUFFER_UNDERFLOW EPISODE_RESOLVED (metric=7, kind=3) Must Not Have Count [ADDED]

**Statement:** If `metric` = 7 (BUFFER_UNDERFLOW) AND `observation-kind` = 3 (EPISODE_RESOLVED), the message MUST NOT contain a `count` field. The parser MUST validate this constraint.

**Rationale:** A BUFFER_UNDERFLOW EPISODE_RESOLVED message signals the end of an underflow event; it is a status change, not an aggregated count. The count applies only to the START (kind=2) of the underflow episode.

**Acceptance Criteria:**

| Scenario | Input | Expected Behavior |
|----------|-------|-------------------|
| **10.1.1 UNDERFLOW RESOLVED without count** | Structure with `metric` = 7, `observation-kind` = 3, no `count` | Accept |
| **10.1.2 UNDERFLOW RESOLVED with count** | Structure with `metric` = 7, `observation-kind` = 3, `count` = 1 | Reject; log "semantic error: BUFFER_UNDERFLOW EPISODE_RESOLVED (metric=7, kind=3) must not have count" |
| **10.1.3 UNDERFLOW START with count (allowed)** | Structure with `metric` = 7, `observation-kind` = 2, `count` = 2 | Accept (count is allowed in START) |
| **10.1.4 UNDERFLOW RESOLVED (kind=3, different metric)** | Structure with `metric` = 1, `observation-kind` = 3, `count` = 1 | Accept (constraint only applies to metric=7) |

---

### REQUIREMENT 11: Legacy Field Name Rejection [ADDED]

**Statement:** The parser MUST explicitly check for and reject any message that contains legacy field names: `metric_type`, `phase`, `pts_ns` (underscore), `source_id`, `session_id`. If any of these fields are present, the entire message MUST be rejected with an appropriate error log.

**Rationale:** Legacy field names indicate a pre-0.7 producer or a message generated by non-conformant software. Accepting such messages risks silent data corruption (reading the wrong field value or missing new fields). Explicit rejection signals a producer/consumer mismatch clearly to operators.

**Acceptance Criteria:**

| Scenario | Input | Expected Behavior |
|----------|-------|-------------------|
| **11.1.1 Legacy field metric_type** | Structure with `metric_type` field present (any value) | Reject; log "legacy field detected: metric_type" |
| **11.1.2 Legacy field phase** | Structure with `phase` field present | Reject; log "legacy field detected: phase" |
| **11.1.3 Legacy field pts_ns (underscore)** | Structure with `pts_ns` field (not `pts-ns`) | Reject; log "legacy field detected: pts_ns (use pts-ns instead)" |
| **11.1.4 Legacy field source_id** | Structure with `source_id` field in payload | Reject; log "legacy field detected: source_id (source enrichment is Rialto responsibility)" |
| **11.1.5 Legacy field session_id** | Structure with `session_id` field in payload | Reject; log "legacy field detected: session_id (session enrichment is Rialto responsibility)" |
| **11.1.6 No legacy fields present** | Structure with only 0.7-compliant fields | Accept (pass all other validations) |
| **11.1.7 Multiple legacy fields** | Structure with `metric_type` AND `phase` present | Reject; log first detected legacy field (or all, if time permits) |

---

## Summary

These 11 requirements ensure that the RialtoMetrics parser enforces strict compliance to the 0.7 `media-pipeline-metric` normative contract. Each requirement includes multiple test scenarios to cover:

- **Positive cases:** Valid messages that should be accepted.
- **Negative cases:** Messages with missing fields, wrong types, out-of-range values, or semantic violations that should be rejected.
- **Legacy cases:** Messages using pre-0.7 field names that should be explicitly rejected.

Implementation of all 11 requirements—with passing tests for all scenarios—constitutes full compliance to the 0.7 contract and readiness for production validation of SoC metrics in Rialto.
