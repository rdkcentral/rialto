# Spec Delta

## Purpose

Defines how the client acquires captured frames, exposes them to the application, uses the shared frame reference as the lease on a capture slot, and caches graphics imports across acquisitions.

## ADDED Requirements

### Requirement: Acquisition results
`acquireCurrentFrame()` SHALL return `changed = true` and a new frame for a new selection, and make it the current frame. For no new selection it SHALL return `changed = false` and the current frame. For no selection it SHALL clear the current frame and return `changed = false` and null.

#### Scenario: New frame
- **WHEN** the server returns a new frame
- **THEN** the call returns `changed = true` and that frame

#### Scenario: Same selection again
- **WHEN** the next call finds no new selection
- **THEN** it returns `changed = false` and the same frame object, and no additional slot is locked

#### Scenario: No selection
- **WHEN** the server reports no selection, for example during a seek
- **THEN** the call returns `changed = false` and null, and earlier frames the application holds stay valid

### Requirement: Errors do not break the capture object
A transient failure (no resources, or an IPC failure) SHALL return `changed = false` and the current frame. A fatal or unsupported result SHALL clear the current frame and return `changed = false` and null.

#### Scenario: Transient failure
- **WHEN** the server reports no resources
- **THEN** the call returns `changed = false` and the current frame

#### Scenario: Fatal capture
- **WHEN** the server reports a fatal capture error
- **THEN** the call returns `changed = false` and null, and later calls do not crash

### Requirement: The shared frame reference is the lease
Dropping the last reference to a frame SHALL release its capture slot exactly once, from whichever thread drops it, and releases of one capture SHALL be serialized. The release SHALL NOT wait for graphics work.

#### Scenario: Last reference dropped
- **WHEN** the application drops the last of several references to a frame
- **THEN** exactly one release is sent, carrying the frame's slot and acquisition token

#### Scenario: References dropped on two threads
- **WHEN** two threads drop the last two references to two different frames at the same time
- **THEN** each frame is released exactly once and the releases do not overlap

#### Scenario: Late release
- **WHEN** the application releases a frame after the pipeline was stopped or destroyed
- **THEN** the release succeeds

### Requirement: Frame accessors
A frame SHALL report its slot index from `frameIndex()`, its presentation time, its padded backing size as `textureWidth/textureHeight`, its visible size as `visibleWidth/visibleHeight`, `layout()` as `RGBA` and `numResources()` as 1. The visible region SHALL start at the texture origin.

#### Scenario: Frame after a resolution change
- **WHEN** a frame with a 1280x720 visible size in a 1920x1088 backing is acquired
- **THEN** it reports texture size 1920x1088, visible size 1280x720, layout `RGBA` and one resource

### Requirement: Import cache
The client SHALL reuse an existing import when the ordered object IDs and planes, the full import description including colour, the invalidation period and the graphics context all match, and SHALL otherwise create a new import. It SHALL keep at most the admitted number of reusable entries, evicting unused ones. On a reuse it SHALL close the descriptors received with the frame.

#### Scenario: Cache hit
- **WHEN** a frame arrives with the same objects, planes and description as a cached import
- **THEN** the cached import is reused and the frame's received descriptors are closed

#### Scenario: Colour change
- **WHEN** a frame reuses the same object IDs with a different colour description
- **THEN** a new import is created and older frames keep their own

#### Scenario: Cache bound
- **WHEN** more distinct imports are needed than the admitted count
- **THEN** unused entries are evicted so reusable entries never exceed the admitted count

### Requirement: Invalidation before lookup
When a frame carries the invalidation flag, the client SHALL invalidate all existing cache entries before looking up the frame's objects. Entries still used by held frames SHALL be retired, destroyed only after those frames are released, and SHALL never be reused. An invalidation SHALL NOT be undone if the following import fails.

#### Scenario: Same IDs after invalidation
- **WHEN** a flushed frame reuses object IDs that are in the cache
- **THEN** a new import is created instead of reusing the old entry

#### Scenario: Retired entry in use
- **WHEN** an invalidated entry is still used by a held frame
- **THEN** it is destroyed only after that frame is released

#### Scenario: Import fails after invalidation
- **WHEN** a flushed frame's import fails
- **THEN** the old entries stay invalidated and are not restored

### Requirement: Failed import
If importing a new frame fails, the client SHALL close the frame's received descriptors, release its slot, keep the previous current frame, and return `changed = false`.

#### Scenario: Import error
- **WHEN** the graphics import of a new frame fails
- **THEN** its descriptors are closed, one release is sent, and the call returns `changed = false` with the previous current frame
