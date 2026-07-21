## 1. GstGenericPlayer teardown timer safety

- [x] 1.1 Update `GstGenericPlayer::termPipeline()` to stop playback-info timer activity in teardown flow by calling stopNotifyPlaybackInfoTimer()
- [x] 1.2 Add guarded logic so teardown checks playback-info timer existence and active state before cancellation.
- [x] 1.3 Ensure cancellation ordering prevents playback-info timer callbacks from outliving teardown-in-progress pipeline state.
- [x] 1.4 Verify teardown remains safe when playback-info timer is missing or already inactive.

## 2. Unit-test expectation refactor

- [x] 2.1 Remove broad playback-info timer factory expectations (for example `.Times(AnyNumber())`) from shared teardown/common helpers.
- [x] 2.2 Add focused helper expectations for playback-info timer creation only in tests that exercise timer startup.
- [x] 2.3 Add focused helper expectations for playback-info timer cancellation only in tests that exercise stop/teardown behavior.

## 3. Crash-fix validation tests

- [x] 3.1 Add or update a teardown-focused unit test that models an active playback-info timer at teardown start (`isActive() == true`).
- [x] 3.2 Assert active-timer teardown invokes cancellation as part of `termPipeline()` behavior.
- [x] 3.3 Keep or add a separate inactive-timer teardown test to validate safe no-op behavior without replacing active-timer validation.

## 4. Verification and non-regression

- [ ] 4.1 Run gstplayer unit tests covering playback-info timer lifecycle and teardown paths.
- [x] 4.2 Confirm strict OpenSpec validation passes for `termlinefix` after artifact updates.
- [x] 4.3 Review changes to ensure no public API, IPC contract, or payload format changes were introduced.
