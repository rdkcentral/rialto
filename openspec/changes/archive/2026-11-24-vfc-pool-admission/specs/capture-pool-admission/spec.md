# Spec Delta

## Purpose

Defines how the Rialto server admits a capture HAL session configured at READY: it queries and validates the capabilities, checks the pipeline's size, chooses the slot count, creates the pool once, and reports success or failure to the capture commit.

## ADDED Requirements

### Requirement: Admission follows configured READY
For every capture commit whose sink reached configured READY, the server SHALL run admission synchronously on the same `allSourcesAttached()` call, before shipping setup is scheduled. `allSourcesAttached()` SHALL return `true` only if admission succeeds.

#### Scenario: Successful admission
- **WHEN** a clear video pipeline with an attached capture calls `allSourcesAttached()` and the fake HAL reports valid capabilities
- **THEN** the HAL `createPool()` is called once and `allSourcesAttached()` returns `true`

#### Scenario: Admission refused
- **WHEN** any admission step fails
- **THEN** `allSourcesAttached()` returns `false`, the sink is returned to NULL, the HAL session is deleted afterwards and the application may call `allSourcesAttached()` again

### Requirement: Capabilities query with a retry window
Admission SHALL call the HAL `getCapabilities()`. While the HAL answers "capabilities not available yet", admission SHALL retry until the READY timeout expires (`RIALTO_VIDEO_FRAME_CAPTURE_READY_TIMEOUT_MS`, default 5000 ms), and then fail with an error log that names the case. Any other non-`OK` result SHALL fail at once.

#### Scenario: HAL ready on a later try
- **WHEN** the HAL answers "not available yet" twice and then `OK`
- **THEN** admission continues with the third answer

#### Scenario: HAL never ready
- **WHEN** the HAL answers "not available yet" until the timeout, set to 200 ms
- **THEN** admission fails after about 200 ms and the log says the HAL never became ready at configured READY

#### Scenario: HAL error
- **WHEN** the HAL `getCapabilities()` returns `FATAL_ERROR`
- **THEN** admission fails without retrying and `createPool()` is not called

### Requirement: Capabilities are validated
Admission SHALL reject an `OK` capabilities record that breaks the record rules: a record smaller than Rialto's, `minimumSlots` of 0 or greater than `maximumSlots`, or a zero `maximumContentSize`.

#### Scenario: Inverted slot limits
- **WHEN** the HAL reports `minimumSlots = 4` and `maximumSlots = 2`
- **THEN** admission fails and `createPool()` is not called

### Requirement: Pipeline size fits the capture
Admission SHALL fail if the pipeline's `VideoRequirements::maxWidth` exceeds the width of `maximumContentSize`, or if `maxHeight` exceeds its height.

#### Scenario: Larger than the capture supports
- **WHEN** the pipeline was created with a maximum of 3840x2160 and the HAL reports a maximum content size of 1920x1088
- **THEN** admission fails and `createPool()` is not called

#### Scenario: Within the capture limits
- **WHEN** the pipeline was created with a maximum of 1920x1080 and the HAL reports 1920x1088
- **THEN** admission continues

### Requirement: Slot count selection
Admission SHALL choose N = clamp(T, max(2, minimumSlots), maximumSlots). T SHALL be read from `RIALTO_VIDEO_FRAME_CAPTURE_TARGET_SLOTS` at each admission. A missing value SHALL mean 3, and an unparsable value or a value below 2 SHALL mean 3 with a warning. Admission SHALL fail if `maximumSlots` is below 2.

#### Scenario: Slot count table
- **WHEN** the HAL range and T are, in turn, [1,4] with 3; [3,4] with 3; [1,2] with 3; [4,6] with 3; [2,2] with 3; [5,5] with 3; [1,4] with 2; [1,4] with 99
- **THEN** `createPool()` is called with 3, 3, 2, 4, 2, 5, 2 and 4 respectively

#### Scenario: Invalid target
- **WHEN** T is `"abc"` or `1` and the HAL range is [1,4]
- **THEN** a warning is logged and `createPool()` is called with 3

#### Scenario: Single-slot HAL
- **WHEN** the HAL range is [1,1]
- **THEN** admission fails and `createPool()` is not called

#### Scenario: Admission log
- **WHEN** admission chooses N
- **THEN** one info line records `minimumSlots`, `maximumSlots`, T and N

### Requirement: Pool created once and validated
Admission SHALL call `createPool(N)` exactly once per HAL session. It SHALL fail if the call returns non-`OK`, or if the pool breaks the pool rules: a slot layout count other than N, a slot index out of position, a zero or oversized backing size, or a non-zero template object ID.

#### Scenario: HAL refuses the pool
- **WHEN** `createPool()` returns `NO_RESOURCES`
- **THEN** admission fails and `createPool()` is not called again for that session

#### Scenario: Wrong slot count returned
- **WHEN** `createPool(3)` returns `OK` with two slot layouts
- **THEN** admission fails

### Requirement: Admitted session identity
A successful admission SHALL give the session a pool ID that is non-zero and unique among the server's sessions. The committed session SHALL then carry that pool ID, the admitted pool, N and the capabilities used to admit it.

#### Scenario: Two pipelines
- **WHEN** two pipelines each admit a capture
- **THEN** their pool IDs are non-zero and different, and each acquired frame reports its own pipeline's pool ID
