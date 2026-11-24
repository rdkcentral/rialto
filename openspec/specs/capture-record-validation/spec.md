# capture-record-validation Specification

## Purpose

Defines how Rialto checks the capability, pool and frame records returned by the SoC capture HAL before using them, and how it reacts when a record is invalid.

## Requirements

### Requirement: Capability records are checked
Rialto SHALL accept a successful capability record only if `1 <= minimumSlots <= maximumSlots`, both dimensions of `maximumContentSize` are non-zero, and the record's `size` field is at least the size Rialto was built with. Otherwise Rialto SHALL treat the query as `FATAL_ERROR` and log which rule failed.

#### Scenario: Valid limits
- **WHEN** the HAL reports `minimumSlots = 2` and `maximumSlots = 4` with a non-zero maximum content size
- **THEN** the record is accepted

#### Scenario: Equal limits
- **WHEN** the HAL reports `minimumSlots = maximumSlots = 3`
- **THEN** the record is accepted

#### Scenario: Inverted or zero limits
- **WHEN** the HAL reports `minimumSlots = 0`, or `minimumSlots` greater than `maximumSlots`
- **THEN** the record is rejected as `FATAL_ERROR` and the failing rule is logged

### Requirement: Pool records are checked
Rialto SHALL accept a successful pool record only if it holds exactly the requested number of slot layouts, each slot's `slotIndex` equals its position, `backingSize` does not exceed `maximumContentSize`, and every plane object ID in the templates is zero.

#### Scenario: Valid pool
- **WHEN** `createPool(3)` returns three layouts with slot indices 0, 1 and 2, zero object IDs, and a backing size within the maximum
- **THEN** the record is accepted

#### Scenario: Wrong slot count
- **WHEN** `createPool(3)` returns two layouts
- **THEN** the record is rejected as `FATAL_ERROR`

#### Scenario: Slot index out of position
- **WHEN** the second layout reports `slotIndex = 5`
- **THEN** the record is rejected as `FATAL_ERROR`

#### Scenario: Non-zero template object ID
- **WHEN** a template plane carries a non-zero object ID
- **THEN** the record is rejected as `FATAL_ERROR`

#### Scenario: Backing larger than the maximum
- **WHEN** the pool backing size exceeds the capability's maximum content size
- **THEN** the record is rejected as `FATAL_ERROR`

### Requirement: Frame identity and descriptors are consistent
Rialto SHALL accept a successful frame only if its slot index is within the admitted pool, its acquisition token is non-zero, every object ID is non-zero, every plane references exactly one listed object, and the exported descriptors contain exactly one valid descriptor for each distinct referenced object, with no duplicates and no unreferenced objects.

#### Scenario: Shared object, one descriptor
- **WHEN** a frame has two planes in one object and one exported descriptor for that object (spec example B)
- **THEN** the frame is accepted

#### Scenario: Separate objects
- **WHEN** a frame has two planes in two objects and one descriptor for each (spec example C)
- **THEN** the frame is accepted

#### Scenario: Duplicate descriptor
- **WHEN** a frame exports two descriptors for the same object
- **THEN** the frame is rejected as `FATAL_ERROR`

#### Scenario: Unrelated descriptor
- **WHEN** a frame exports a descriptor for an object no plane references
- **THEN** the frame is rejected as `FATAL_ERROR`

#### Scenario: Missing descriptor
- **WHEN** a plane references an object that has no exported descriptor
- **THEN** the frame is rejected as `FATAL_ERROR`

#### Scenario: Zero object ID or token
- **WHEN** a frame uses object ID 0 or acquisition token 0
- **THEN** the frame is rejected as `FATAL_ERROR`

### Requirement: Frame ranges are inside their bounds
Rialto SHALL check, without arithmetic overflow, that every plane satisfies `offsetBytes <= sizeBytes` and `lengthBytes <= sizeBytes - offsetBytes` for its object, and that the visible region's width and height fit inside the backing size from its origin. Rialto SHALL also reject a frame whose plane overlaps a plane of another currently locked slot bound to the same object.

#### Scenario: Range at the exact end
- **WHEN** a plane ends exactly at the end of its object
- **THEN** the frame is accepted

#### Scenario: Overflowing range
- **WHEN** a plane's offset plus length would exceed the object size or wrap around 64 bits
- **THEN** the frame is rejected as `FATAL_ERROR`

#### Scenario: Visible region too large
- **WHEN** the visible width exceeds the backing width
- **THEN** the frame is rejected as `FATAL_ERROR`

#### Scenario: Overlap with another locked slot
- **WHEN** a new frame's plane overlaps a plane of a different slot that is still locked in the same object
- **THEN** the frame is rejected as `FATAL_ERROR`

### Requirement: YUV frames carry a complete colour description
For a YUV format, Rialto SHALL reject a successful frame whose matrix, range, horizontal siting or vertical siting is `UNKNOWN`. RGB formats MAY leave these fields `UNKNOWN`.

#### Scenario: NV12 with unknown matrix
- **WHEN** an NV12 frame reports an `UNKNOWN` matrix
- **THEN** the frame is rejected as `FATAL_ERROR`

#### Scenario: Complete NV12 description
- **WHEN** an NV12 frame reports BT709, narrow range and co-sited/midpoint siting
- **THEN** the frame is accepted

### Requirement: Non-zero visible origin is logged, not rejected
Rialto SHALL accept a frame whose visible region has a non-zero `x` or `y` and whose region is otherwise in bounds, SHALL log an error the first time this happens in a session, and SHALL expose only its width and height.

#### Scenario: First non-zero origin
- **WHEN** a frame reports a visible region starting at (0, 8)
- **THEN** the frame is accepted and one error is logged for the session

#### Scenario: Repeated non-zero origin
- **WHEN** later frames in the same session also report a non-zero origin
- **THEN** they are accepted and no further error is logged
