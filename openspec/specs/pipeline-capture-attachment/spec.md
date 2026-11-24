# pipeline-capture-attachment Specification

## Purpose

Defines how a Video Frame Capture is bound to a media pipeline and unbound from it, which pipeline states allow each, and how a binding changes other pipeline operations.

## Requirements

### Requirement: Attach only before commit
`attachVideoFrameCapture()` SHALL succeed only when the pipeline has been loaded and `allSourcesAttached()` has not yet been called, and SHALL return `false` in every other state. Attaching SHALL allocate no capture buffers.

#### Scenario: Attach after load
- **WHEN** a capture is attached after `load()` and before `allSourcesAttached()`
- **THEN** it returns `true`

#### Scenario: Attach before load
- **WHEN** a capture is attached to a pipeline that has not been loaded
- **THEN** it returns `false`

#### Scenario: Attach after commit
- **WHEN** a capture is attached after `allSourcesAttached()`, or in IDLE, PAUSED, PLAYING, SEEKING, SEEK_DONE, END_OF_STREAM, STOPPED or FAILURE
- **THEN** it returns `false` and the pipeline is unchanged

#### Scenario: Source order does not matter
- **WHEN** the video source is attached after the capture, and then `allSourcesAttached()` is called
- **THEN** the capture remains attached

### Requirement: Invalid captures are rejected
`attachVideoFrameCapture()` SHALL return `false` for a null capture, for a capture object not created by Rialto's capture factory, and when the server does not support the operation.

#### Scenario: Null capture
- **WHEN** `attachVideoFrameCapture(nullptr)` is called
- **THEN** it returns `false`

#### Scenario: Foreign capture
- **WHEN** an application passes its own implementation of `IVideoFrameCapture`
- **THEN** it returns `false`

#### Scenario: Old server
- **WHEN** the server does not implement the attach operation
- **THEN** it returns `false`

### Requirement: One capture per pipeline, one pipeline per capture
A pipeline SHALL have at most one attached capture, and a capture SHALL be attached to at most one pipeline at a time.

#### Scenario: Second capture on the same pipeline
- **WHEN** a second capture is attached to a pipeline that already has one
- **THEN** it returns `false` and the first stays attached

#### Scenario: Same capture on a second pipeline
- **WHEN** a capture attached to pipeline A is attached to pipeline B
- **THEN** it returns `false` and stays attached to A

### Requirement: The pipeline holds the capture
While a capture is attached, the pipeline SHALL keep a reference to it, so the capture stays alive even if the application drops its own references.

#### Scenario: Application drops its reference
- **WHEN** the application releases its last reference to an attached capture
- **THEN** the capture is not destroyed until it is detached or the pipeline is destroyed

### Requirement: Detach only before commit or after stop
`detachVideoFrameCapture()` SHALL succeed in the loaded state before `allSourcesAttached()` and in STOPPED, and SHALL return `false` in IDLE, PAUSED, PLAYING, SEEKING, SEEK_DONE, END_OF_STREAM and FAILURE, and when no capture is attached. Frames already acquired SHALL remain valid after detach.

#### Scenario: Detach before commit
- **WHEN** a capture is detached after `load()` and before `allSourcesAttached()`
- **THEN** it returns `true` and the pipeline no longer has a capture

#### Scenario: Detach after stop
- **WHEN** a capture is detached in STOPPED
- **THEN** it returns `true`

#### Scenario: Detach during playback
- **WHEN** a capture is detached in PLAYING
- **THEN** it returns `false` and the capture stays attached

#### Scenario: Nothing attached
- **WHEN** `detachVideoFrameCapture()` is called on a pipeline without a capture
- **THEN** it returns `false`

### Requirement: Implicit detach
Destroying a pipeline SHALL detach its capture. Destroying a capture's server registration while it is bound SHALL remove the binding as a detach would. A detached capture SHALL be attachable to another pipeline.

#### Scenario: Pipeline destroyed
- **WHEN** a pipeline with an attached capture is destroyed
- **THEN** the capture is no longer attached and can be attached to a new loaded pipeline

#### Scenario: Reuse after detach
- **WHEN** a capture detached from a stopped pipeline is attached to a new loaded pipeline
- **THEN** it returns `true`

### Requirement: No video window while capturing
`setVideoWindow()` SHALL return `false` while a capture is attached to the pipeline, and behave as before otherwise.

#### Scenario: Window set with a capture
- **WHEN** `setVideoWindow()` is called on a pipeline with an attached capture
- **THEN** it returns `false`

#### Scenario: Window set after detach
- **WHEN** the capture is detached and `setVideoWindow()` is called
- **THEN** it behaves exactly as without capture
