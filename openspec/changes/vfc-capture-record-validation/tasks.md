# Tasks

Unit tests run with `./build_ut.py -s servercapture` (the suite added in 1.1). "Spec" means `specs/capture-record-validation/spec.md`; "D#" means a decision in `design.md`.

## 1. Setup

- [x] 1.1 Add a `servercapture` unit-test suite (`tests/unittests/media/server/capture/`) and register it in `build_ut.py`. Verify `./build_ut.py -s servercapture -l` lists it.
- [ ] 1.2 Add test fixtures that build SoC spec examples A–D (§5.2) and the NV12 example (§5.3) as `CapturedFrame` and `CapturePool` records with real `memfd_create` FDs. Verify the fixtures compile and the FDs are closed by the fixture teardown.

## 2. Capabilities and pools (D1, D2)

- [ ] 2.1 Implement `validateCapabilities` and `validatePool` returning `ValidationResult` with a rule name. Verify with table-driven tests: valid and equal limits pass; `min = 0`, `min > max`, zero maximum size and a short `size` fail; a pool with the wrong count, an out-of-position index, a non-zero template ID or an oversized backing fails.

## 3. Frames (D1–D4)

- [ ] 3.1 Implement identity and descriptor checks in `validateFrame`: slot within the pool, non-zero token and IDs, plane-to-object matching, exactly one FD per distinct referenced object. Verify: examples A–D pass; duplicate, unrelated and missing FDs fail; zero ID and zero token fail.
- [ ] 3.2 Implement overflow-safe plane range and visible-region checks. Verify: a range ending exactly at the object end passes; overflowing and 64-bit-wrapping ranges fail; an oversized visible region fails.
- [ ] 3.3 Implement `LockedRanges` and the cross-slot overlap check, cleared on `dmaBufObjectsFlushed`. Verify: overlap with another locked slot in the same object fails; the same range after that slot is released passes; a flushed frame reusing an ID passes.
- [ ] 3.4 Implement the YUV colour check with the fourcc table. Verify: NV12 with an `UNKNOWN` matrix, range or siting fails; complete NV12 passes; an RGB format with `UNKNOWN` fields passes.

## 4. Visible origin (D5)

- [ ] 4.1 Return `ok` with a warning for a non-zero visible origin, and add the per-session "logged once" flag helper that R9 will own. Verify: a (0, 8) origin passes with a warning; two such frames produce one log line through a mocked logger.

## 5. Integration check

- [ ] 5.1 Run `./build_ut.py -s servercapture -val` and confirm no leaks or FD leaks across all fixtures.

## Workflow follow-up

- Run `openspec validate vfc-capture-record-validation --strict` before review.
- If the spec authors reject the A21 proposal, change D5 so a non-zero origin fails validation.
