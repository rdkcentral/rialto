## Why

RialtoServer can crash during rapid FAST channel switching because `GstGenericPlayer` teardown does not explicitly stop the playback-info timer before pipeline resources are released.

The playback-info timer is periodic and may still be active while `termPipeline()` is tearing down the pipeline, worker-thread-owned state, and sink-related resources. If the timer callback runs during this window, it can observe partially destroyed state and contribute to teardown-time race conditions and process crashes.

The implementation should guarantee that playback-info timer activity is stopped as part of pipeline termination. Unit tests should validate that this cancellation happens for an active timer during teardown and should not rely on broad or weak mock expectations that hide incorrect behavior.

## What Changes

- Update `GstGenericPlayer::termPipeline()` to stop playback-info timer activity during pipeline termination by calling `stopNotifyPlaybackInfoTimer()`.
- Ensure teardown ordering prevents playback-info timer callbacks from outliving the pipeline state they depend on.
- Preserve existing runtime behavior during normal playback; only teardown behavior changes.
- Refactor unit-test expectations so playback-info timer creation and cancellation are expressed explicitly in the tests that need them.
- Remove broad timer factory expectations from shared teardown helpers.
- Add or update unit tests so the crash-fix path is validated using an active playback-info timer during teardown.


## Capabilities

### New Capabilities

None.

### Modified Capabilities

- **gstplayer teardown timer safety**
  - `GstGenericPlayer` must stop the playback-info timer during `termPipeline()`.
  - An active playback-info timer must be canceled before pipeline teardown completes.
  - No playback-info timer callback may execute against pipeline state that is already being destroyed.

- **unit-test precision for playback-info timer lifecycle**
  - Playback-info timer creation expectations must be declared only in tests that exercise timer startup.
  - Playback-info timer cancellation expectations must be declared only in tests that exercise stop or teardown behavior.
  - Shared teardown helpers must not contain unconstrained timer factory expectations.

## Detailed Requirements

### Functional Requirements

- `GstGenericPlayer::termPipeline()` must invoke playback-info timer shutdown as part of teardown.
- If the playback-info timer exists and is active during teardown, teardown must cancel it before proceeding with the remaining pipeline cleanup.
- If the playback-info timer does not exist or is already inactive, teardown must remain safe and complete without error.
- The change must remain internal to gstplayer teardown behavior and must not modify public APIs, IPC contracts, or playback-info payload definitions.

### Unit Test Requirements

- Remove shared `EXPECT_CALL(*m_timerFactoryMock, createTimer(_, _, _)).Times(AnyNumber())` style expectations from common teardown helpers.
- Introduce dedicated helper methods for playback-info timer expectations, such as:
  - timer creation expectation helper
  - active timer cancellation expectation helper
  - optional inactive timer expectation helper where needed
- Do not use `.Times(AnyNumber())` for timer factory expectations related to this change.
- Add expectations only in tests that actually require timer creation or timer cancellation behavior.
- Update playback-info timer tests so they remain explicit about how many `isActive()` checks are expected for the exercised flow.
- Ensure at least one teardown-focused test validates the real crash-fix behavior:
  - a playback-info timer is present,
  - `isActive()` returns `true` during teardown,
  - `cancel()` is invoked as part of teardown.
- Do not model the crash-fix validation using only `isActive() == false` during player teardown, because that would bypass the new cancellation behavior.
- Keep inactive-timer coverage, if needed, as a separate behavior-specific test rather than the main validation for this fix.



## Suggested Test Refactoring Approach

- Move playback-info timer expectations out of generic destruction utilities and into dedicated helpers or directly into the tests that exercise playback-info timer behavior.
- Prefer helpers with clear intent, for example:
  - `expectPlaybackInfoTimerCreation(...)`
  - `expectPlaybackInfoTimerCancel(...)`
- Use those helpers only from tests that:
  - start the playback-info timer,
  - stop the playback-info timer,
  - destroy the player after the playback-info timer has been created.
- Ensure teardown tests are representative of the production bug by modeling an active periodic timer at the point teardown begins.

## Impact

### Affected Components

- `media/server/gstplayer/source/GstGenericPlayer.cpp`
- `tests/unittests/media/server/gstplayer/genericPlayer/GstGenericPlayerPrivateTest.cpp`

### Behavioral Impact

- Reduces teardown-time race conditions in `GstGenericPlayer`.
- Prevents playback-info timer callbacks from running while pipeline teardown is in progress.
- Improves stability during repeated or rapid channel switches such as Xumo FAST verification flows.
- Does not alter external interfaces or runtime playback-info semantics outside teardown.

### Testing Impact

- Unit tests must explicitly validate active playback-info timer cancellation during teardown.
- Unit tests must use precise, scenario-specific mock expectations.
- Shared teardown helpers must not mask missing or incorrect timer lifecycle behavior.

## Out of Scope

- Changing playback-info reporting frequency.
- Changing playback-info payload content or callback interfaces.
- Introducing new public APIs.
- Broad refactoring of unrelated timer behavior outside playback-info teardown coverage.

