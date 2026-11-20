# Design

## Context

See proposal.md for the motivation. The HAL types come from R2 (`media/server/capture/include/ICaptureSession.h`). The rules are SoC spec §3.1, §3.2, §5.1 ("Identity", "Range validation", "FD export"), §6.2 (colour validity) and §7.3 (log every `FATAL_ERROR`). The A21 proposal changes one rule: a non-zero visible origin is a vendor contract violation that Rialto logs but tolerates.

## Goals / Non-Goals

**Goals:**
- Pure, stateless checks for capabilities and pools; one small stateful check for cross-slot overlap.
- A result that names the failing rule, for the log.

**Non-Goals:**
- Calling the HAL, releasing frames or closing FDs: the caller (R8, R9) does that, based on the result.
- Graphics-format checks that need a GPU (R12, R13).

## Decisions

### D1. Validator returns a result, not a bool
`ValidationResult { bool ok; std::string rule; }` from `validateCapabilities(caps, builtSize)`, `validatePool(pool, caps, requestedCount)` and `validateFrame(frame, pool, lockedRanges)`. The caller logs `rule` and decides the status.

*Why:* every rejection must be logged with its cause (§7.3), and table-driven tests can assert the exact rule.

### D2. Overflow-safe arithmetic only
All range checks use the subtraction forms from the spec (`length <= size - offset` after `offset <= size`); no addition of untrusted values.

### D3. Cross-slot overlap through a per-session range index
`LockedRanges` maps object ID to the byte ranges held by each locked slot. `validateFrame` checks the new frame's planes against ranges from other slots; R9 adds a frame's ranges on successful delivery and removes them on release. It is cleared when `dmaBufObjectsFlushed` is true, because object IDs may then be rebound.

*Alternative:* skip the cross-slot check and trust the vendor. Rejected: §5.1 says Middleware MUST validate it, and overlap would let one frame's pixels change while another is locked.

### D4. YUV detection by DRM fourcc
A small table of YUV fourccs (NV12, NV21, P010, YUV420 and so on) decides whether colour fields must be known. Unknown fourccs are treated as YUV, so the check errs on the strict side.

### D5. Visible origin warning is per session
`validateFrame` returns `ok` with a `warning` for a non-zero origin. The session object (R9) owns a `loggedNonZeroOrigin` flag so the error appears once per session.

## Risks / Trade-offs

- [A strict rule rejects a vendor that is otherwise usable] → every rule maps to a MUST in the SoC spec; the logged rule name makes the vendor bug obvious in the first integration run.
- [The A21 proposal is not accepted by the spec authors] → only D5 changes: a non-zero origin becomes a rejection, one line and one test.
- [Overlap index cost] → at most N slots × a few planes per session; linear scans are enough.

## Migration Plan

None. Nothing calls the validator until R8 and R9.
