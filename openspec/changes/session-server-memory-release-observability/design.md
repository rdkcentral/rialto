## Context

Rialto uses `ACTIVE`, `INACTIVE`, and `NOT_RUNNING` lifecycle states for each session server process. Existing lifecycle behavior intends resource release on deactivation, but memory-release success is not currently represented as a machine-verifiable outcome.

This design introduces an internal memory observability pipeline tied to lifecycle transitions and emits a single structured outcome log for each deactivation verification.

## Goals / Non-Goals

**Goals:**
- Measure per-session-server process memory at deterministic lifecycle checkpoints.
- Validate memory release for `ACTIVE -> INACTIVE` using explicit thresholds.
- Enforce cleanup-completeness semantics before final deactivation success.
- Keep observability log-first for operations and CI.

**Non-Goals:**
- Introduce new public API methods for clients.
- Add platform-specific memory profiler dependencies.
- Replace existing healthcheck/restart framework.

## Decisions

### Use `/proc`-based process metrics with PSS as primary signal

Use `/proc/<pid>/smaps_rollup` for `Pss`, `Anonymous`, `Shmem` and `/proc/<pid>/status` for `VmRSS`; use fd-directory size for descriptor leak signal.

Rationale:
- Low overhead and available in target Linux runtime.
- PSS gives proportional shared-page accounting better than RSS alone.

### Tie snapshots to lifecycle checkpoints

Collect three mandatory snapshots per deactivation verification:
- `baselineInactive`
- `preDeactivate`
- `postDeactivate`

Compute:
- `activeDeltaKb = preDeactivatePssKb - baselineInactivePssKb`
- `residualKb = postDeactivatePssKb - baselineInactivePssKb`
- `releaseRatio = (preDeactivatePssKb - postDeactivatePssKb) / max(activeDeltaKb, 1)`

### Enforce cleanup barrier before final deactivation outcome

Deactivation path must report cleanup counters and only proceed to success path when cleanup obligations are complete.

Counters include:
- active playback sessions
- active web audio players
- residual CDM sessions
- shared-memory mappings

### Emit single structured verification outcome

For each deactivation flow, emit one canonical structured log record containing memory values, cleanup status, and action decision (`none`, `retry`, `restart`, `error`).

This is the default integration point for operations, CI, and dashboards.

## Risks / Trade-offs

- PSS collection may be unavailable on some kernels -> fallback to RSS and mark metric source.
- Allocator behavior can delay memory return -> include a short post-inactive probe delay.
- Overly strict thresholds may trigger false positives -> tune policy and keep one retry pass.

## Migration Plan

1. Add sampler plumbing and PID exposure.
2. Add cleanup counters and barrier semantics.
3. Add policy evaluation and structured log emission.
4. Add tests and tune thresholds with repeated lifecycle runs.

Rollback is straightforward because the feature is additive and can be disabled via policy/config if needed.
