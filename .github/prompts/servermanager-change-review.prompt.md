---
description: Perform a structured, evidence-based review of Server Manager changes without modifying code.
---

# Server Manager Change Review

Use this prompt to perform a deep review of Server Manager-related changes.

## Scope

- Primary scope: `serverManager/**`
- Supporting scope when relevant: `tests/unittests/serverManager/**`, `tests/componenttests/**`, `ipc/**`, `common/**`
- Reference scope for expected behavior and known risks:
  - `serverManager/architecture-brief.md`
  - `serverManager/SME-notes.md`
  - `docs/architecture-brief.md`

## Critical Constraints

- You MUST complete phases in order.
- You MUST update the working document after each phase.
- You MUST provide evidence for every finding (file path and rationale).
- You MUST NOT modify code in this task.
- This is analysis-only unless the user explicitly asks for fixes.
- If evidence is insufficient, mark the item as `Needs Follow-up`.

## Output Requirements

Produce:
1. A working document at `./tmp/servermanager-review-inventory.md`
2. A final report at `./tmp/servermanager-review-report.md`
3. A concise chat summary of critical/high risks and recommended next actions

Severity scale:
- Critical
- High
- Medium
- Low
- Info

Status flow:
- Not Started -> In Progress -> Completed

---

## PHASE 0: Create Working Structure

Create `./tmp/servermanager-review-inventory.md` with these sections:

1. Scope Confirmation (Not Started)
2. Behavioral Changes (Not Started)
3. Lifecycle and Process Risks (Not Started)
4. IPC and Contract Risks (Not Started)
5. Configuration and Failure Paths (Not Started)
6. Test Coverage and Gaps (Not Started)
7. Consolidated Findings (Not Started)

Validation before proceeding:
- File exists
- All 7 sections exist
- Each section starts as `Not Started`

## PHASE 1: Confirm Scope and Inputs

Objective:
- Identify what changed and what should be reviewed.

Process:
1. Determine changed files relevant to Server Manager.
2. Identify potentially impacted neighboring modules.
3. Record assumptions and unknowns.
4. Record any conflict between diff behavior and architecture/SME references.

Update section:
- Scope Confirmation

Validation before proceeding:
- Scope Confirmation marked `Completed`
- At least one changed file or a clear "no changes detected" note

## PHASE 2: Behavioral Change Analysis

Objective:
- Determine functional behavior changes introduced by the diff.

Process:
1. Review changed logic in `serverManager/**`.
2. Identify behavior deltas (startup, shutdown, restart, supervision, state transitions).
3. Note regressions, ambiguous behavior, or undocumented side effects.

Update section:
- Behavioral Changes

Validation before proceeding:
- Behavioral Changes marked `Completed`
- At least 3 concrete observations or a justified "no material deltas" conclusion

## PHASE 3: Lifecycle and Process Risk Review

Objective:
- Evaluate process/lifecycle correctness and resilience.

What to check:
- State transition correctness
- Retry/backoff behavior
- Timeout handling
- Resource cleanup on failure
- Ordering/race risk during start/stop

Update section:
- Lifecycle and Process Risks

Validation before proceeding:
- Section marked `Completed`
- At least 2 risk checks documented with evidence

## PHASE 4: IPC and Contract Review

Objective:
- Validate IPC interactions and interface contract consistency.

What to check:
- Request/response assumptions
- Error propagation and mapping
- Backward compatibility expectations
- Serialization/protocol usage impact

Update section:
- IPC and Contract Risks

Validation before proceeding:
- Section marked `Completed`
- Contract-impact statement included (even if "none")

## PHASE 5: Configuration and Failure Path Review

Objective:
- Ensure configuration-driven behavior and failure paths are safe.

What to check:
- Defaults and overrides
- Invalid/missing configuration handling
- Failure mode fallback behavior
- Log quality for diagnosability

Update section:
- Configuration and Failure Paths

Validation before proceeding:
- Section marked `Completed`
- At least 2 failure-path scenarios recorded

## PHASE 6: Test Coverage and Gaps

Objective:
- Assess whether tests cover changed behavior and risk areas.

What to check:
- Existing unit/component test coverage relevance
- Missing tests for new branches/error paths
- Flaky-risk patterns

Update section:
- Test Coverage and Gaps

Validation before proceeding:
- Section marked `Completed`
- Explicit list of missing tests (or "no gaps found" with rationale)

## PHASE 7: Consolidate Findings

Objective:
- Normalize and prioritize findings.

Process:
1. Deduplicate overlapping findings.
2. Assign severity and confidence.
3. Map each finding to impacted files.

Update section:
- Consolidated Findings

Validation before proceeding:
- Section marked `Completed`
- Every finding includes: `Severity`, `Confidence`, `Evidence`, `Impact`, `Recommendation`

## PHASE 8: Produce Final Report

Create `./tmp/servermanager-review-report.md` with this exact structure:

```markdown
# Server Manager Review Report

## Executive Summary
- Overall risk level:
- Top 3 concerns:

## Findings by Severity

### Critical
- [ID] Title
  - Evidence:
  - Impact:
  - Recommendation:

### High
- ...

### Medium
- ...

### Low
- ...

### Info
- ...

## Test Gap Recommendations
- [Gap] Suggested test and location

## Open Questions
- ...

## Reviewer Notes
- Assumptions:
- Out-of-scope items:
```

Validation before completion:
- Report file exists
- All findings trace back to evidence
- Empty severity groups explicitly marked `None`

## Completion Checklist

- All phases completed in sequence
- Working document sections all marked `Completed`
- Final report created at `./tmp/servermanager-review-report.md`
- Chat summary includes only highest-priority items and next actions
