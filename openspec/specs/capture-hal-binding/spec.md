# capture-hal-binding Specification

## Purpose

Defines how Rialto binds to the SoC capture HAL: the interface and value types it uses, how it finds and loads the vendor capture library, and how long that library and each session stay alive.

## Requirements

### Requirement: HAL interface matches the SoC specification
Rialto SHALL use the capture HAL interface exactly as the SoC capture specification defines it: `ICaptureSession` with `getCapabilities`, `createPool`, `acquireCurrentFrame` and `releaseFrame`, a virtual destructor, and the `CaptureStatus` values `OK`, `NO_NEW_FRAME`, `UNSUPPORTED`, `INVALID_ARGUMENT`, `NO_RESOURCES`, `NO_FRAME`, `FATAL_ERROR` and `CAPABILITIES_NOT_AVAILABLE_YET`, with the same value types and field layout.

#### Scenario: Vendor library built against the interface
- **WHEN** a vendor library is compiled against Rialto's capture HAL header
- **THEN** Rialto can call every operation of a session that library creates and receives each `CaptureStatus` value unchanged

### Requirement: Captured frame records cannot be copied or moved
A `CapturedFrame` record SHALL be neither copyable nor movable, and destroying one SHALL NOT close any file descriptor or release any frame lock.

#### Scenario: Copy or move is rejected at build time
- **WHEN** code attempts to copy-construct, copy-assign, move-construct or move-assign a `CapturedFrame`
- **THEN** the code does not compile

#### Scenario: Destroying a record leaves its descriptors open
- **WHEN** a `CapturedFrame` holding an open descriptor is destroyed
- **THEN** the descriptor is still open afterwards

### Requirement: Empty and reset records
A `CapturedFrame` SHALL be empty exactly when its slot index is `kInvalidCaptureSlotIndex`, its acquisition token is `kInvalidAcquisitionToken` and every exported descriptor is `-1`. A newly constructed record SHALL be empty. Resetting a released record with no open descriptors SHALL clear its flush flag and all of its frame metadata.

#### Scenario: New record is empty
- **WHEN** a `CapturedFrame` is default-constructed
- **THEN** it is empty

#### Scenario: Open descriptor makes a record non-empty
- **WHEN** a record has an invalid slot and token but one exported descriptor that is not `-1`
- **THEN** it is not empty

#### Scenario: Reset of a released record
- **WHEN** a released record with every descriptor set to `-1` and `dmaBufObjectsFlushed` true is reset
- **THEN** it is empty, its flush flag is false, and its planes, objects and colour description are cleared

### Requirement: Vendor library discovery
Rialto SHALL load the capture library named `lib_capture_hal.so` unless the platform configures a different name, and SHALL obtain sessions only through the unmangled `extern "C"` factory `createCaptureSession`. If the library or the factory cannot be found, Rialto SHALL report capture as unavailable and SHALL keep serving every other feature.

#### Scenario: Default library loaded
- **WHEN** `lib_capture_hal.so` is installed and exports `createCaptureSession`
- **THEN** Rialto reports capture as available and can create a session

#### Scenario: Platform overrides the library name
- **WHEN** the platform configures a different capture library name
- **THEN** Rialto loads that library instead of `lib_capture_hal.so`

#### Scenario: Library missing
- **WHEN** no capture library can be loaded
- **THEN** Rialto reports capture as unavailable, logs the reason once, and media playback without capture is unaffected

#### Scenario: Factory symbol missing
- **WHEN** the library loads but does not export `createCaptureSession`
- **THEN** Rialto reports capture as unavailable and logs the reason

#### Scenario: Factory returns no session
- **WHEN** the factory returns a null pointer
- **THEN** session creation fails without crashing and capture stays available for later attempts

### Requirement: Library stays loaded through session cleanup
Rialto SHALL delete each session through `ICaptureSession`'s virtual destructor and SHALL keep the vendor library loaded until every session created from it has been deleted.

#### Scenario: Session outlives its creator
- **WHEN** the component that loaded the library is destroyed while a session created from it still exists
- **THEN** the library stays loaded until that session has been deleted

#### Scenario: Deletion runs the vendor destructor
- **WHEN** Rialto deletes a session
- **THEN** the concrete vendor destructor runs, and only afterwards may the library be unloaded

### Requirement: No exceptions cross the HAL boundary
Rialto SHALL treat any exception escaping a HAL call as a `FATAL_ERROR` result for that call and SHALL log it; it SHALL NOT let the exception terminate the server.

#### Scenario: Vendor call throws
- **WHEN** a HAL call made by Rialto throws
- **THEN** Rialto logs the error and handles the call as `FATAL_ERROR`
