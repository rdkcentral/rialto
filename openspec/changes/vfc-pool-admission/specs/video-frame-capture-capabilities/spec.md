# Spec Delta

## Purpose

Defines the public `getCapabilities()` query of GL and Vulkan captures and its wire form: which status it reports in each capture state, which values it reports once a capture is admitted, and the structural pool layout the server returns through `getPool`.

## ADDED Requirements

### Requirement: Status-returning capabilities query
`IGLVideoFrameCapture` and `IVulkanVideoFrameCapture` SHALL provide `getCapabilities(out, status)`, which returns a `VideoFrameCaptureStatus` and sets `status` to the same value. On any non-`OK` result, `out` SHALL be left unchanged. Calling it SHALL allocate no frame slots and create no graphics resources.

#### Scenario: Returned and out-parameter status agree
- **WHEN** an application calls `getCapabilities(out, status)` in any capture state
- **THEN** the returned value equals `status`

#### Scenario: Failure leaves the record untouched
- **WHEN** the query returns a non-`OK` status
- **THEN** every field of `out` keeps the value it had before the call

### Requirement: Status by capture state
The query SHALL return `SESSION_NOT_READY` when the capture has no active admitted session: it is not attached, it is attached but not committed, or its session was retired. It SHALL return `FATAL_ERROR` when the session became capture-fatal, and also when the server cannot be reached. It SHALL return `OK` for an admitted session whose HAL query succeeds.

#### Scenario: Before commit
- **WHEN** an application attaches a capture and calls `getCapabilities()` before `allSourcesAttached()`
- **THEN** it returns `SESSION_NOT_READY`

#### Scenario: Not attached
- **WHEN** a capture that was never attached is queried
- **THEN** it returns `SESSION_NOT_READY`

#### Scenario: After commit
- **WHEN** `allSourcesAttached()` returned `true` and the application calls `getCapabilities()`
- **THEN** it returns `OK`

#### Scenario: After stop
- **WHEN** the pipeline was stopped after a successful commit
- **THEN** the query returns `SESSION_NOT_READY`

#### Scenario: Capture-fatal session
- **WHEN** an acquisition made the session capture-fatal
- **THEN** the query returns `FATAL_ERROR`

#### Scenario: Server lost
- **WHEN** the IPC call fails
- **THEN** the query returns `FATAL_ERROR`

### Requirement: Query reaches the HAL of the capture's session
The server SHALL answer each capabilities request by calling the HAL `getCapabilities()` of the session admitted for the capture named in the request. This call SHALL be serialized with the session's acquisitions and releases. A non-`OK` HAL result SHALL be reported with the same status mapping as acquisition, and SHALL NOT change the session's state.

#### Scenario: Query during acquisitions
- **WHEN** the client queries capabilities while another thread acquires and releases frames
- **THEN** no two HAL calls on that session overlap

#### Scenario: HAL error on the query
- **WHEN** the HAL `getCapabilities()` returns `NO_RESOURCES` for an admitted session
- **THEN** the query returns `NO_RESOURCES`, and the next acquisition still calls the HAL

### Requirement: Reported values
On `OK`, the common capabilities SHALL report:
- `maxConcurrentFrames` = the admitted slot count N;
- `maxResourcesPerFrame` = 1;
- `supportsProtectedContent` = 0 and `supportsHdr` = 0.

Each `size` field SHALL equal the size of its struct. The wire reply SHALL list the HAL's DRM format and modifier.

#### Scenario: Admitted with three slots
- **WHEN** admission chose N = 3 from the HAL range [1,4]
- **THEN** `maxConcurrentFrames` is 3, `maxResourcesPerFrame` is 1, and protected content and HDR are 0

### Requirement: Vulkan formats come from the device
For a Vulkan capture, `supportedFormats` SHALL list the Vulkan format for each reported DRM format that Rialto can import and that the application's physical device supports with the reported modifier. Formats that Rialto cannot map, or that the device does not support, SHALL be omitted.

#### Scenario: NV12 supported
- **WHEN** the server reports NV12 linear and the device supports it
- **THEN** `supportedFormats` is `[VK_FORMAT_G8_B8R8_2PLANE_420_UNORM]`

#### Scenario: Unsupported by the device
- **WHEN** the device does not support the reported format and modifier
- **THEN** the query still returns `OK` and `supportedFormats` is empty

### Requirement: Capabilities request names the capture
The capabilities request SHALL carry the capture ID as well as the graphics API. A request for an unknown capture, or one whose API differs from the capture's own, SHALL be answered `INVALID_ARGUMENT`. The wire status SHALL include `SESSION_NOT_READY` with value 7. Existing status values and numbers SHALL be unchanged.

#### Scenario: Unknown capture
- **WHEN** a client sends a capabilities request for a capture ID it never created
- **THEN** the reply status is `INVALID_ARGUMENT`

#### Scenario: Wrong API
- **WHEN** a client sends a Vulkan capabilities request for a GL capture
- **THEN** the reply status is `INVALID_ARGUMENT`

### Requirement: Pool layout reply
For a capture with an admitted session, the server SHALL answer `getPool` with `OK` and the admitted layout:
- the pool ID, DRM format and modifier, backing width and height;
- one slot layout per admitted slot, whose planes have offsets, lengths and strides but no object IDs;
- the HAL slot limits.

It SHALL carry no colour description and no file descriptors.

#### Scenario: Pool after commit
- **WHEN** the client requests the pool of a capture admitted with N = 3
- **THEN** the reply has status `OK`, three slot layouts, the admitted pool ID and no descriptors

### Requirement: Pool layout outside an admitted session
The server SHALL answer `getPool` with `SESSION_NOT_READY` for a capture without an active admitted session, and with `INVALID_ARGUMENT` for an unknown capture.

#### Scenario: Unknown capture pool
- **WHEN** a client requests the pool of a capture ID it never created
- **THEN** the reply status is `INVALID_ARGUMENT`

#### Scenario: Pool before commit
- **WHEN** the client requests the pool of a capture that is attached but not committed
- **THEN** the reply status is `SESSION_NOT_READY`
