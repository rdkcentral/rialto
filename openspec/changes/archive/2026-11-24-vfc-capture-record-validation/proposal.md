## Why

The SoC capture spec (§5.1, §3.1, §3.2, §6.2) requires Middleware, not only the vendor, to validate everything the HAL returns before using it: capability limits, pool templates and every acquired frame. A malformed record (an overflowing plane range, a duplicated or missing FD, a zero object ID) would otherwise reach the IPC layer and the client's graphics import, where it fails in hard-to-diagnose ways or reads another frame's memory. These checks are pure functions of the records, so they can be built and tested before any capture flow exists.

## What Changes

- Add validation of HAL capability records: `1 <= minimumSlots <= maximumSlots`, a non-zero `maximumContentSize`, and a correct record `size`.
- Add validation of pool records from `createPool`: slot count equal to the requested count and within the limits, `slotIndex` equal to its position, `backingSize` within `maximumContentSize`, and zero (unbound) object IDs in the templates.
- Add validation of acquired frames: valid slot and non-zero token; non-zero object IDs; object, plane and exported-FD IDs that match exactly, with one FD per distinct referenced object and no duplicates or extras; plane ranges and the visible region inside their bounds without overflow; no overlap with planes of other currently locked slots in the same object; and effective YUV colour fields that are not `UNKNOWN`.
- A record that fails validation is reported as `FATAL_ERROR` with the failing rule for the log. Releasing a failed frame, closing its FDs and ending capture for the session belong to the acquisition path (R9).
- Following the A21 proposal: a non-zero visible-region origin logs an error once per session and the frame is still accepted, using its width and height.

## Capabilities

### New Capabilities

- `capture-record-validation`: The checks Rialto applies to capability, pool and frame records returned by the capture HAL, and what happens when a record fails them.

### Modified Capabilities

None.

## Impact

- **Code:** a validator in the server capture directory added by R2 (`media/server/capture/`), with no IPC or GStreamer dependency.
- **Tests:** table-driven unit tests built from SoC spec examples A–D (§5.2) and the NV12 example (§5.3); a new unit-test suite entry in `build_ut.py`.
- **Dependencies:** R2 (HAL types).
- **Downstream:** R8 validates capabilities and pools with it; R9 validates every acquired frame before sending it.
