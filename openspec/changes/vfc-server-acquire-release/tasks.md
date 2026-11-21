# Tasks

"Spec" means `specs/capture-frame-delivery/spec.md`; "D#" means a decision in `design.md`. Requires R1 merged; R8 interfaces may be stubbed with the fake HAL until R8 lands.

## 1. Session runtime (D1, D6)

- [x] 1.1 Add `CaptureSessionRuntime` with the per-session lock, slot-indexed record storage, `pendingFlush`, `lastSentDescription`, `fatal` and `LockedRanges`, attached to the registry entry by commit. Verify with `./build_ut.py -s servermain` construction and lookup tests.
- [x] 1.2 Add the HAL-to-wire status mapping, including `CAPABILITIES_NOT_AVAILABLE_YET` → `FATAL_ERROR` with a log. Verify with a table test.

## 2. Acquire (D2–D4)

- [x] 2.1 Implement the acquire handler in `VideoFrameCaptureModuleService` and the runtime's acquire flow: `NO_FRAME` without a session, HAL call under the lock, validation, reply filling, owned FDs and delivery callback. Verify with server IPC unit tests for `OK`, `NO_NEW_FRAME`, `NO_FRAME` and `NO_RESOURCES`.
- [x] 2.2 Implement the delivery callback (D3). Verify with unit tests: delivered clears the latch and records the description; not delivered releases once and keeps the latch.
- [x] 2.3 Implement invalid-frame handling: release, close, set `fatal`, later acquisitions `FATAL_ERROR`, earlier releases still `OK`. Verify with a scripted fake-HAL test.
- [x] 2.4 Implement pool-metadata updates on description change. Verify: a backing-size change adds metadata once; an unchanged frame adds none; no FDs in the metadata.

## 3. Release (D5)

- [x] 3.1 Implement the release handler: matching release `OK`; wrong slot, unlocked slot and wrong token `INVALID_ARGUMENT` with the lock kept. Verify with server IPC unit tests.

## 4. Component tests

- [x] 4.1 Scripted fake-HAL component tests: `OK`/`NO_NEW_FRAME`/`NO_FRAME`; FD count equals distinct objects; client FDs refer to the same files (`fstat` inode) and stay valid after the server closes its copies; server open-FD count returns to baseline. Verify with `./build_ct.py`.
- [x] 4.2 Failure-path component tests: an injected send failure calls HAL release once, and the next delivery carries the flush flag although the HAL flag is false; client disconnect before the reply leaks no FD. Verify with `./build_ct.py`.
- [ ] 4.3 Concurrency stress test: several threads acquiring and releasing on one session; the fake HAL asserts it never sees overlapping calls; every acquisition released once. Verify with `./build_ct.py` and once under valgrind (`-val`).
- [ ] 4.4 Q7 test: frames still delivered with the application state not RUNNING. Verify with `./build_ct.py`.

## Workflow follow-up

- Run `openspec validate vfc-server-acquire-release --strict` before review.
- Merge after R1 and R8; rebase the stubbed R8 interfaces onto R8's final ones.
