---
description: Capture and verify all RialtoServer state evidence and transitions from logs.
---

# RialtoServer State Capture

Use this prompt to collect complete evidence of RialtoServer states and state transitions from logs.

## Canonical States

Use this exact canonical state set for coverage checks:

1. Uninitialized
2. Inactive
3. Active
4. NotRunning
5. Error

If logs contain unknown variants, record them separately under Noncanonical States.

## Scope

- Analyze user-provided logs only.
- Include server manager logs, session server logs, and related system/service logs when available.
- Track state evidence per app and per serverId whenever identifiers exist.
- Use canonical state and transition semantics from:
  - `serverManager/architecture-brief.md`
  - `serverManager/SME-notes.md`
- If user input includes a folder named `logs`, also check for rotated message logs when present:
  - `logs/sky-messages.log`
  - `logs/sky-messages.log.1`
  - `logs/sky-messages.log.2`
  - `logs/sky-messages.log.3`

## Critical Constraints

- You MUST complete phases in order.
- You MUST quote exact log lines for each state claim.
- You MUST include timestamp and source for every evidence line.
- You MUST NOT infer a state without direct evidence.
- You MUST mark missing states as Missing Evidence, not as absent behavior.
- You MUST NOT modify code.

## Output Requirements

Produce:
1. Working document: ./tmp/rialtoserver-state-inventory.md
2. Final report: ./tmp/rialtoserver-state-report.md
3. Chat summary: state coverage verdict and top gaps

Status flow:
- Not Started -> In Progress -> Completed

---

## PHASE 0: Create Working Structure

Create ./tmp/rialtoserver-state-inventory.md with these sections:

1. Input Logs and Time Window (Not Started)
2. State Evidence by Canonical State (Not Started)
3. Transition Evidence (Not Started)
4. Coverage and Gaps (Not Started)
5. Noncanonical and Ambiguous Entries (Not Started)
6. Verdict Inputs (Not Started)

Validation:
- File exists
- All sections exist
- All start as Not Started

## PHASE 1: Identify Inputs and Window

Objective:
- Define sources and analysis window before state extraction.

Process:
1. List each log source path or stream name.
2. Record analysis time window start and end.
3. Record timezone assumptions.

Update section:
- Input Logs and Time Window

Validation:
- Section marked Completed
- At least one concrete log source listed

## PHASE 2: Extract Evidence for Every Canonical State

Objective:
- Collect direct log evidence for each canonical state.

Process:
1. Search for state events and state strings.
2. For each canonical state, add all matching evidence lines.
3. Tag each evidence line with appId and serverId when present.

Update section:
- State Evidence by Canonical State

Required template per state:
- State: <CanonicalState>
- Evidence count:
- Evidence lines:
  - [timestamp] [source] [serverId/appId] <exact line>

Validation:
- Section marked Completed
- All five canonical states included, even if evidence count is zero

## PHASE 3: Build Transition Evidence

Objective:
- Reconstruct observed state transitions in chronological order.

Process:
1. Sort evidence by timestamp.
2. Build transition chains per appId or serverId.
3. Flag invalid or suspicious transitions.

Update section:
- Transition Evidence

Transition format:
- Entity: <appId or serverId>
- Sequence:
  1. <time> <from> -> <to> | Evidence: <quoted line>

Validation:
- Section marked Completed
- At least one sequence produced, or explicit note explaining why impossible

## PHASE 4: Compute Coverage and Gaps

Objective:
- Quantify how complete the state evidence is.

Process:
1. Mark each canonical state as Observed or Missing Evidence.
2. Count transitions observed per entity.
3. Note windows where logging is sparse or missing.

Update section:
- Coverage and Gaps

Coverage table format:
- Uninitialized: Observed | Missing Evidence
- Inactive: Observed | Missing Evidence
- Active: Observed | Missing Evidence
- NotRunning: Observed | Missing Evidence
- Error: Observed | Missing Evidence

Validation:
- Section marked Completed
- Coverage status present for all five canonical states

## PHASE 5: Capture Noncanonical and Ambiguous Entries

Objective:
- Avoid data loss while protecting canonical interpretation.

Process:
1. Record lines that imply a state but do not match canonical values.
2. Record ambiguous lines with possible interpretations.
3. Do not map ambiguous lines to canonical states without explicit evidence.

Update section:
- Noncanonical and Ambiguous Entries

Validation:
- Section marked Completed
- Each ambiguous entry has a short reason

## PHASE 6: Decide Verdict Inputs

Objective:
- Build deterministic inputs for the final verdict.

Set fields:
- Canonical state coverage: 0 to 5 observed
- Transition continuity quality: High | Medium | Low
- Evidence confidence: High | Medium | Low
- Missing data impact: None | Minor | Major

Update section:
- Verdict Inputs

Validation:
- Section marked Completed
- All four fields present with rationale

## PHASE 7: Create Final Report

Create ./tmp/rialtoserver-state-report.md with this exact structure:

```markdown
# RialtoServer State Capture Report

## Coverage Verdict
- Canonical states observed: X/5
- Coverage status: Complete | Partial | Inconclusive
- Confidence: High | Medium | Low
- Time window analyzed:

## Canonical State Evidence

### Uninitialized
- Evidence count:
- Key lines:

### Inactive
- Evidence count:
- Key lines:

### Active
- Evidence count:
- Key lines:

### NotRunning
- Evidence count:
- Key lines:

### Error
- Evidence count:
- Key lines:

## Transition Sequences
- Entity:
  1. <time> <from> -> <to>

## Missing Evidence and Gaps
- Missing states:
- Sparse windows:
- Impact:

## Noncanonical or Ambiguous Entries
- <line>
  - Why ambiguous:

## Final Reasoning
- Why this verdict was chosen:
- What additional logs would complete coverage:
```

Coverage verdict rules:
- Complete: all 5 canonical states observed with direct evidence.
- Partial: at least 1 canonical state missing evidence.
- Inconclusive: logs are insufficient to reliably assess transitions or coverage.

Validation before completion:
- Final report exists
- Every canonical state section is present
- Every key claim references quoted evidence

## Completion Checklist

- All phases completed in sequence
- Inventory sections all marked Completed
- Final report created at ./tmp/rialtoserver-state-report.md
- Chat summary includes coverage count, missing states, and one next action
