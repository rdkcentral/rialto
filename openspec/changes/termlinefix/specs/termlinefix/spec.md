## MODIFIED Requirements

### Requirement: GstGenericPlayer teardown timer safety
The system MUST stop playback-info timer activity during `GstGenericPlayer::termPipeline()` so timer callbacks cannot run against teardown-in-progress pipeline state.

#### Scenario: Active playback-info timer during teardown
- **WHEN** `termPipeline()` begins while the playback-info timer exists and is active
- **THEN** the system checks timer activity and cancels the playback-info timer before pipeline teardown completes
- **THEN** no playback-info timer callback executes against pipeline state that is being destroyed

#### Scenario: Missing or inactive playback-info timer during teardown
- **WHEN** `termPipeline()` begins and the playback-info timer is missing or already inactive
- **THEN** teardown completes safely without timer-cancellation errors
- **THEN** existing non-timer teardown behavior remains unchanged

### Requirement: Unit-test precision for playback-info timer lifecycle
The unit-test suite MUST use explicit and scenario-specific timer lifecycle expectations for playback-info behavior during startup, stop, and teardown.

#### Scenario: Teardown crash-fix path is validated
- **WHEN** a teardown-focused unit test models a playback-info timer that is active at teardown start
- **THEN** the test expects `isActive()` to report active state during teardown
- **THEN** the test expects timer cancellation to occur as part of teardown validation

#### Scenario: Shared teardown helpers avoid broad timer expectations
- **WHEN** common teardown helper code is used by tests
- **THEN** it does not include unconstrained timer factory expectations such as `.Times(AnyNumber())` for playback-info timer creation
- **THEN** timer creation and cancellation expectations are declared only in tests that require those behaviors
