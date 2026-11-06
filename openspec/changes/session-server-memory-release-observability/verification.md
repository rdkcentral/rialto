## Verification Plan

Functional verification:

- Repeated deactivation cycles for one app session:
  - `ACTIVE -> INACTIVE -> ACTIVE` loop and validate release fields in logs.
- Mixed workload run:
  - audio/video + encrypted media + web audio where applicable.
- Failure injection:
  - simulate incomplete cleanup and verify escalation behavior.

Telemetry verification:

- Confirm one structured memory verification log per deactivation.
- Confirm log includes mandatory fields and policy decision.
- Confirm CI parser can classify PASS/FAIL from logs only.

Regression verification:

- Existing lifecycle and restart behavior remains intact.
- No regression in state-transition notifications and healthcheck handling.
