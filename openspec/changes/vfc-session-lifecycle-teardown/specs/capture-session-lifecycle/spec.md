# Spec Delta

## Purpose

Defines when the server retires a capture HAL session, what a retired session still does, and when and in what order the session is deleted and the vendor capture library unloaded.

## ADDED Requirements

### Requirement: Retirement triggers
The server SHALL retire a pipeline's capture session when the pipeline is stopped, enters FAILURE, has its capture detached, or is destroyed, when the capture is destroyed, and when the client disconnects.

#### Scenario: Stop
- **WHEN** a playing capture pipeline is stopped
- **THEN** its capture session is retired

#### Scenario: Playback failure
- **WHEN** a capture pipeline enters FAILURE
- **THEN** its capture session is retired

### Requirement: Retired sessions deliver no new frames
After retirement, acquisitions on the capture SHALL return `NO_FRAME` without calling the HAL, until the capture is attached to another pipeline and that pipeline commits. The final acquisition opportunity of a retired session SHALL be abandoned.

#### Scenario: Acquire after stop
- **WHEN** the application acquires after the pipeline was stopped
- **THEN** it receives a null frame, and the HAL session is not asked for a frame

#### Scenario: New pipeline creates a new session
- **WHEN** the capture is attached to a new pipeline that commits
- **THEN** a new HAL session and pool are created, and the retired session is not reused

### Requirement: Late releases are accepted
A retired session SHALL accept every matching release of a frame acquired before retirement, including after the pipeline has been destroyed and after the session became capture-fatal.

#### Scenario: Release after stop
- **WHEN** the application releases a frame after `stop()`
- **THEN** the release succeeds

#### Scenario: Release after pipeline destruction
- **WHEN** the pipeline is destroyed while the application holds two frames, and the application then releases them
- **THEN** both releases succeed and the frames' contents stayed readable until each release

#### Scenario: Release on a capture-fatal session
- **WHEN** a session became capture-fatal and the application releases a frame acquired earlier
- **THEN** the release succeeds

### Requirement: Deletion preconditions
The server SHALL delete a retired HAL session only after the sink has completed capture disconnection and reached NULL, every frame lock of the session has been released, every server-owned descriptor has been closed, and no HAL call or pending delivery result for the session remains. It SHALL keep the vendor library loaded until that deletion completes.

#### Scenario: Locks still held
- **WHEN** the pipeline is destroyed while the application holds a frame
- **THEN** the session is not deleted until that frame is released

#### Scenario: Last release deletes the session
- **WHEN** the last outstanding frame of a retired session is released
- **THEN** the session is deleted after that release completes, and only then can the library be unloaded

#### Scenario: Sink still tearing down
- **WHEN** no frames are held but the sink has not yet reached NULL
- **THEN** the session is not deleted until the sink has reached NULL

### Requirement: Disconnect releases remaining locks
When a client disconnects, the server SHALL release every frame lock still held for that client's captures exactly once, then delete their sessions under the deletion preconditions, then remove the registry.

#### Scenario: Client killed while holding frames
- **WHEN** a client holding N frames is killed
- **THEN** the HAL sees N releases, then each session is deleted, then the library may be unloaded, and the server keeps running

#### Scenario: Other clients unaffected
- **WHEN** one client disconnects while another client is capturing
- **THEN** the other client's capture continues
