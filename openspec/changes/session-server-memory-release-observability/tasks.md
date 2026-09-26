## 1. Lifecycle Sampling and Metrics

- [ ] 1.1 Expose session server PID to the manager-side memory sampler.
- [ ] 1.2 Implement process memory reader for `VmRSS`, `Pss`, `Anonymous`, `Shmem`, and `fdCount`.
- [ ] 1.3 Capture `baselineInactive` after successful startup to `INACTIVE`.
- [ ] 1.4 Capture `preDeactivate` before sending `SetState(INACTIVE)`.
- [ ] 1.5 Capture `postDeactivate` after server confirms `INACTIVE` and cleanup completes.

## 2. Inactive Cleanup Barrier

- [ ] 2.1 Ensure deactivation path reports cleanup counters for playback, web audio, CDM, and shared memory mappings.
- [ ] 2.2 Gate final deactivation success on cleanup completion semantics.
- [ ] 2.3 Add one post-inactive probe delay for allocator settling before final memory sample.

## 3. Policy Evaluation and Escalation

- [ ] 3.1 Implement release math (`activeDeltaKb`, `residualKb`, `releaseRatio`).
- [ ] 3.2 Add configurable thresholds (`inactiveResidualPssLimitKb`, `minReleaseRatio`).
- [ ] 3.3 Add retry/remediation pass before marking failure.
- [ ] 3.4 Track consecutive failures and trigger restart/escalation when threshold is exceeded.

## 4. Structured Telemetry Contract

- [ ] 4.1 Emit one structured log per `ACTIVE -> INACTIVE` verification outcome.
- [ ] 4.2 Include mandatory fields: app, serverId, pid, baseline/pre/post memory, residual, releaseRatio, cleanupOk, action.
- [ ] 4.3 Ensure telemetry is sufficient for CI PASS/FAIL without manual `/proc` inspection.

## 5. Validation

- [ ] 5.1 Add unit tests for sampler parsing and fallback behavior.
- [ ] 5.2 Add unit tests for policy arithmetic and threshold boundaries.
- [ ] 5.3 Add lifecycle tests for repeated `ACTIVE -> INACTIVE -> ACTIVE` cycles.
- [ ] 5.4 Add failure-injection tests for incomplete cleanup and escalation behavior.
- [ ] 5.5 Run OpenSpec validation and required non-regression test targets.
