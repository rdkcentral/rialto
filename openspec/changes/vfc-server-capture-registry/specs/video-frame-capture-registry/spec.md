# Spec Delta

## Purpose

Defines how Video Frame Capture objects are created and destroyed, how the server identifies them, how their registrations are scoped to a client's IPC connection, and when creation fails.

## ADDED Requirements

### Requirement: Capture creation
When a client creates a GL capture, Rialto SHALL register it with the server and return a capture object whose `apiType()` is `GL`. Creation SHALL allocate no capture buffers and no graphics resources.

#### Scenario: Successful creation
- **WHEN** the server supports capture, the capture HAL library is available, and `libEGL.so.1` can be loaded
- **THEN** `createGLVideoFrameCapture()` returns a non-null capture whose `apiType()` is `GL`

#### Scenario: Nothing allocated
- **WHEN** a capture is created
- **THEN** the HAL is not asked to create a session or pool

### Requirement: Creation fails cleanly
`createGLVideoFrameCapture()` SHALL return null, without affecting other client objects, when the server is unreachable, the server does not provide capture, the capture HAL library is unavailable on the server, or `libEGL.so.1` cannot be loaded in the client.

#### Scenario: HAL library missing on the server
- **WHEN** the server cannot load the capture HAL library
- **THEN** the server answers creation with `UNSUPPORTED` and the client returns null

#### Scenario: Server without capture
- **WHEN** the server does not provide `VideoFrameCaptureModule`
- **THEN** the client returns null

#### Scenario: EGL not loadable
- **WHEN** `libEGL.so.1` cannot be loaded in the client process
- **THEN** the client returns null and sends no registration to the server

#### Scenario: Unknown API type
- **WHEN** the server receives a creation request with an unknown API type
- **THEN** it answers `INVALID_ARGUMENT` and registers nothing

### Requirement: Capture identity
The server SHALL give each registered capture a non-zero `capture_id` that is unique on its IPC connection and never reused on that connection. A request naming a `capture_id` that this connection did not create, or that was destroyed, SHALL be rejected.

#### Scenario: Two captures on one connection
- **WHEN** a client creates two captures
- **THEN** they have different non-zero IDs

#### Scenario: ID from another connection
- **WHEN** a client names a `capture_id` created on a different connection
- **THEN** the request is rejected and the other connection's capture is unaffected

#### Scenario: Destroyed ID
- **WHEN** a client names a `capture_id` it has already destroyed
- **THEN** the request is rejected

### Requirement: Capture destruction
Destroying the client's capture object SHALL remove its server registration. The server SHALL accept destruction of a registered capture exactly once.

#### Scenario: Capture object released
- **WHEN** the application drops its last reference to a capture
- **THEN** the server registration is removed

### Requirement: Registrations are scoped to the IPC connection
When a client's IPC connection closes, the server SHALL remove every capture registration of that connection and SHALL leave other connections' registrations unchanged.

#### Scenario: Client process exits
- **WHEN** a client with two registered captures disconnects
- **THEN** both registrations are removed and the registry for that connection is empty

#### Scenario: Other client unaffected
- **WHEN** one client disconnects while another has a registered capture
- **THEN** the other client's capture remains registered and usable

### Requirement: Acquisition before frame delivery exists
Until a capture is attached to a committed pipeline that delivers frames, `acquireCurrentFrame()` SHALL return `changed = false` and a null frame.

#### Scenario: Newly created capture
- **WHEN** an application calls `acquireCurrentFrame()` on a capture that is not attached
- **THEN** it returns `changed = false` and a null frame
