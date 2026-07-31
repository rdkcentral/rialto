## ADDED Requirements

### Requirement: Per-session-server memory snapshots
The system SHALL collect memory snapshots for each session server process at lifecycle checkpoints needed for deactivation verification.

#### Scenario: Mandatory snapshots are captured
- **WHEN** an application session transitions through active/deactive lifecycle
- **THEN** the system captures `baselineInactive`, `preDeactivate`, and `postDeactivate` snapshots for the owning session server process
- **THEN** each snapshot includes process memory metrics required by policy evaluation

### Requirement: Deactivation cleanup completeness reporting
The system SHALL determine cleanup completeness for `ACTIVE -> INACTIVE` before final success outcome is published.

#### Scenario: Cleanup completes successfully
- **WHEN** deactivation cleanup has no remaining playback, web audio, CDM, or shared-memory resources
- **THEN** cleanup completeness is reported as successful for that transition

#### Scenario: Cleanup remains incomplete
- **WHEN** one or more required resource counters remain non-zero during deactivation
- **THEN** cleanup completeness is reported as failed
- **THEN** the transition follows failure/escalation policy

### Requirement: Memory release policy evaluation
The system SHALL evaluate memory release using configured thresholds and derived values from lifecycle snapshots.

#### Scenario: Memory release is within policy
- **WHEN** post-deactivation memory residual and release ratio satisfy configured thresholds
- **THEN** the transition is classified as memory-release success

#### Scenario: Memory release violates policy
- **WHEN** post-deactivation residual or release ratio violates configured thresholds
- **THEN** the system applies retry/remediation and re-evaluates
- **THEN** repeated violations follow escalation policy

### Requirement: Structured deactivation verification telemetry
The system SHALL emit one structured verification log per `ACTIVE -> INACTIVE` transition outcome.

#### Scenario: Structured log contains required fields
- **WHEN** deactivation verification completes
- **THEN** the emitted log includes app id, server id, pid, baseline/pre/post metrics, residual, release ratio, cleanup status, and action

#### Scenario: CI uses log-only pass/fail classification
- **WHEN** lifecycle verification is executed in CI
- **THEN** pass/fail classification can be derived from structured log fields without routine manual `/proc` inspection
