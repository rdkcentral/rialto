## Why

Rialto lifecycle handling documents that `ACTIVE -> INACTIVE` should release active playback, CDM, and shared-memory resources, but there is no standardized, automated memory verification contract proving release happened for each session server process.

A log-first memory observability feature is needed so operations and CI can validate resource release deterministically without routine manual inspection.

## What Changes

- Add per-session-server memory measurement at lifecycle checkpoints using Linux `/proc` data.
- Define transition-time snapshots around `ACTIVE -> INACTIVE` and compute release metrics from baseline, pre-deactivate, and post-deactivate samples.
- Add cleanup-completeness counters to deactivation flow and include them in the outcome.
- Emit one structured memory verification log per deactivation flow.
- Apply configurable policy thresholds for residual memory and release ratio, including escalation behavior for repeated failures.
- Keep this as an additive observability feature with no external API breakage.

## Capabilities

### New Capabilities

- `session-server-memory-observability`: Collect and evaluate per-session-server memory metrics across lifecycle transitions.
- `inactive-release-verification`: Verify `ACTIVE -> INACTIVE` cleanup and memory release against policy thresholds.

### Modified Capabilities

None.

## Impact

Affected areas include:
- ServerManager lifecycle orchestration and state-transition handling.
- Session server internals that perform deactivation cleanup and report cleanup completeness.
- Logging/telemetry pipelines that parse structured transition outcomes.
- Unit/component tests for threshold evaluation, cleanup barrier correctness, and escalation paths.
