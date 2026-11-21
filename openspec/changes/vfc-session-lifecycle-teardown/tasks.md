# Tasks

"Spec" means `specs/capture-session-lifecycle/spec.md`; "D#" means a decision in `design.md`.

## 1. Retirement (D1, D2)

- [x] 1.1 Add `retire()` to `CaptureSessionRuntime` and `retiredSessions` to `CaptureEntry`; acquisitions on a retired or missing session answer `NO_FRAME` without a HAL call. Verify with `./build_ut.py -s servermain` tests.
- [ ] 1.2 Call `retire()` from stop, FAILURE, detach in STOPPED, pipeline destruction and capture destruction. Verify with unit tests, one per trigger.
- [ ] 1.3 Route releases to retired runtimes by `capture_id`, `pool_id` and slot. Verify: release after stop, after destruction and on a capture-fatal session return `OK`.

## 2. Deletion gate (D3, D5)

- [ ] 2.1 Track `sinkAtNull`, `lockedCount` and `inFlight`; delete the HAL session outside the lock when all are satisfied. Set `sinkAtNull` from the pipeline when playbin reaches NULL. Verify with unit tests for each ordering of the three events.
- [ ] 2.2 Make R9's delivery callback hold a `shared_ptr` and count in `inFlight`. Verify with a test that retires during a pending send and checks the session is deleted only after the callback.

## 3. Disconnect (D4)

- [ ] 3.1 On `clientDisconnected`, release every stored record once, retire all runtimes, and remove the registry when empty. Verify with `./build_ut.py -s serveripc` tests.

## 4. Component tests

- [ ] 4.1 Stop, then a late release returns `OK`, and the fake HAL records the session deletion after it. Verify with `./build_ct.py`.
- [ ] 4.2 Destroy the pipeline while holding two frames; both releases succeed; deletion follows the second. Verify with `./build_ct.py`.
- [ ] 4.3 Kill the client while holding N locks: the fake HAL sees N releases, then deletion, then library unload last (fake HAL logs destructor and an `atexit`/unload marker). Verify with `./build_ct.py`.
- [ ] 4.4 A capture-fatal session still accepts releases and is deleted afterwards. Verify with `./build_ct.py`.
- [ ] 4.5 Run the capture component tests under valgrind (`-val`) and confirm no leaked sessions, FDs or memory.

## Workflow follow-up

- Run `openspec validate vfc-session-lifecycle-teardown --strict` before review.
- On target, confirm with the vendor's plumbing example that capture writes stop during the sink's READY→NULL before the session is deleted.
