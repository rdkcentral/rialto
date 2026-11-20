# Spec Delta

## Purpose

Defines how the Rialto server acquires frames from the capture HAL, delivers them and their file descriptors to the client, recovers when delivery fails, and releases frame locks.

## ADDED Requirements

### Requirement: Acquisition result mapping
For a capture attached to a committed pipeline, the server SHALL call the HAL once per `acquireCurrentFrame` request and SHALL answer with the matching status: `OK`, `NO_NEW_FRAME`, `NO_FRAME`, `UNSUPPORTED`, `NO_RESOURCES` or `FATAL_ERROR`. A HAL result with no wire equivalent SHALL be answered as `FATAL_ERROR` and logged. A capture with no committed session SHALL be answered `NO_FRAME` without calling the HAL.

#### Scenario: New, repeated and missing selection
- **WHEN** the HAL returns `OK`, then `NO_NEW_FRAME`, then `NO_FRAME`
- **THEN** the client receives `OK` with a frame, then `NO_NEW_FRAME`, then `NO_FRAME`, the last two carrying no frame and no descriptors

#### Scenario: Capture not committed
- **WHEN** the client acquires on a capture whose pipeline has not committed
- **THEN** it receives `NO_FRAME` and the HAL is not called

#### Scenario: Pool exhausted
- **WHEN** every admitted slot is locked and the HAL returns `NO_NEW_FRAME`
- **THEN** the client receives `NO_NEW_FRAME` and playback continues

### Requirement: Successful frames are delivered complete
An `OK` reply SHALL carry the pool ID, slot index, presentation time in nanoseconds, visible region, acquisition token, frame metadata and exactly one descriptor per distinct DMA-BUF object. Each descriptor the client receives SHALL refer to the same file as the HAL exported and SHALL be independent of the server's copy.

#### Scenario: Two planes in one object
- **WHEN** the HAL returns a frame with two planes in one object
- **THEN** the client receives one descriptor, and both planes reference its object ID

#### Scenario: Descriptors independent
- **WHEN** the server has sent a frame and closed its own copies
- **THEN** the client's descriptors still refer to the open DMA-BUF objects

### Requirement: Server copies are always closed
The server SHALL close every descriptor the HAL exported for an acquisition exactly once, whether delivery succeeds or fails.

#### Scenario: Server descriptor count
- **WHEN** a frame is delivered successfully
- **THEN** the server's open descriptor count returns to its value before the acquisition

### Requirement: Invalid frames end capture for the session
If an acquired frame fails validation, the server SHALL release it exactly once, close its descriptors, log the failing rule, answer `FATAL_ERROR`, and answer `FATAL_ERROR` to every later acquisition in that session, while still accepting releases of earlier frames.

#### Scenario: Invalid frame after a valid one
- **WHEN** a valid frame is held and the next frame fails validation
- **THEN** the failed frame is released and its descriptors closed, later acquisitions return `FATAL_ERROR`, and releasing the earlier frame returns `OK`

### Requirement: Failed delivery releases the frame and keeps invalidation
If an `OK` reply cannot be delivered to the client, the server SHALL release that frame exactly once and SHALL keep the invalidation notice pending. The next successful `OK` delivery SHALL carry the invalidation flag set, even if the HAL's flag for that frame is false.

#### Scenario: Send fails after a flushed frame
- **WHEN** a frame with the invalidation flag set cannot be delivered, and the next frame from the HAL has the flag clear
- **THEN** the first frame is released once, and the next delivered frame carries the flag set

#### Scenario: Client gone
- **WHEN** the client disconnects before an `OK` reply is sent
- **THEN** the frame is released once and no descriptor is leaked

### Requirement: Pool description updates
When an `OK` frame's DRM format, modifier, backing size or colour description differs from the description last delivered to the client for that session, the reply SHALL include updated pool metadata, which SHALL contain no descriptors. Earlier frames keep their own descriptions.

#### Scenario: Resolution change
- **WHEN** the first frame after an ABR resolution change is delivered
- **THEN** the reply includes pool metadata with the new backing size

#### Scenario: No change
- **WHEN** a frame matches the last delivered description
- **THEN** the reply includes no pool metadata

### Requirement: Release
A `releaseFrame` request naming a locked slot and its acquisition token SHALL release that lock in the HAL and answer `OK`. A wrong slot, an unlocked slot or a wrong token SHALL be answered `INVALID_ARGUMENT` and leave the lock in place.

#### Scenario: Matching release
- **WHEN** the client releases a held frame with its slot and token
- **THEN** the HAL lock is released and the slot can carry a later frame

#### Scenario: Wrong token
- **WHEN** the client releases a held slot with a different token
- **THEN** it receives `INVALID_ARGUMENT` and the frame stays locked

### Requirement: HAL calls are serialized and non-blocking
The server SHALL never run two HAL calls of the same session at once, including releases it starts itself, and SHALL NOT wait for decoding or presentation inside an acquisition. Acquisition order SHALL be preserved through delivery.

#### Scenario: Concurrent acquire and release
- **WHEN** acquisitions and releases for one session arrive concurrently from several threads
- **THEN** the HAL observes no overlapping calls and every acquired frame is released exactly once

### Requirement: Delivery is not gated by application state
Frame delivery SHALL NOT depend on the client's application state or on the server's active or inactive state while the pipeline exists.

#### Scenario: Application not running
- **WHEN** the application state is not RUNNING and the pipeline is playing
- **THEN** acquisitions still return frames
