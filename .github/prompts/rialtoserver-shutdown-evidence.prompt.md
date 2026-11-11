---
description: Verify from logs whether RialtoServer shut down properly, with explicit evidence and pass/fail criteria.
---

# RialtoServer Shutdown Evidence

Use this prompt to determine whether RialtoServer shutdown was proper, degraded, or failed based on log evidence.

Companion skill:
- `/rialto-servermanager-log-evidence` for reusable Server Manager log evidence extraction and state coverage checks.

## Scope

- Analyze log files provided by the user (systemd journal export, application logs, server manager logs, device logs).
- Focus on shutdown behavior for RialtoServer and Server Manager orchestration.
- Cross-check expected shutdown and recovery behavior against:
  - `serverManager/architecture-brief.md`
  - `serverManager/SME-notes.md`
- If user input includes a folder named `logs`, also check for rotated message logs when present:
  - `logs/sky-messages.log`
  - `logs/sky-messages.log.1`
  - `logs/sky-messages.log.2`
  - `logs/sky-messages.log.3`

## Critical Constraints

- You MUST complete phases in order.
- You MUST build evidence before writing conclusions.
- You MUST quote exact log lines for all key claims.
- You MUST include timestamps and source file/log stream for each evidence item.
- You MUST NOT modify source code.
- If mandatory evidence is missing, mark result as `Inconclusive` (not `Pass`).

## Output Requirements

Produce:
1. Working document: `./tmp/rialtoserver-shutdown-inventory.md`
2. Final report: `./tmp/rialtoserver-shutdown-evidence.md`
3. Chat summary: one-paragraph verdict with top evidence and next action

Status flow:
- Not Started -> In Progress -> Completed

---

## PHASE 0: Create Working Structure

Create `./tmp/rialtoserver-shutdown-inventory.md` with these sections:

1. Input Logs and Time Window (Not Started)
2. Shutdown Trigger Evidence (Not Started)
3. Orderly Teardown Evidence (Not Started)
4. Disqualifying Evidence (Not Started)
5. PID and Timeline Correlation (Not Started)
6. Final Verdict Inputs (Not Started)

Validation:
- File exists
- All 6 sections exist
- All sections begin as `Not Started`

## PHASE 1: Identify Inputs and Window

Objective:
- Define exactly what logs are being analyzed and the shutdown window.

Process:
1. List each log source and path.
2. Determine approximate shutdown start and end timestamps.
3. Record timezone and clock assumptions.

Update section:
- Input Logs and Time Window

Validation:
- Section marked `Completed`
- At least one concrete time window documented

## PHASE 2: Capture Shutdown Trigger Evidence

Objective:
- Find evidence that shutdown was intentionally initiated.

Look for trigger-like indicators, such as:
- user/service stop request
- server manager shutdown intent
- IPC closure initiation

Repository-relevant marker examples:
- `closing IPC channel`
- `terminating IPC thread`

Update section:
- Shutdown Trigger Evidence

Validation:
- Section marked `Completed`
- At least 1 trigger signal captured, or explicit note that none was found

## PHASE 3: Capture Orderly Teardown Evidence

Objective:
- Prove the shutdown progressed through clean teardown.

Look for ordered signals such as:
1. IPC begins closing
2. IPC thread exits
3. server process exits
4. state reaches not-running/inactive equivalent

Repository-relevant marker examples:
- `closing IPC channel`
- `terminating IPC thread`
- `exiting ipc thread`
- `Server with id: <id> exited.`

Update section:
- Orderly Teardown Evidence

Validation:
- Section marked `Completed`
- At least 2 ordered teardown signals found, or explicit gap documented

## PHASE 4: Capture Disqualifying or Degraded Signals

Objective:
- Detect evidence that shutdown was forced, timed out, or error-driven.

Disqualifying/degraded marker examples:
- `Waitpid timeout. Killing:`
- `Killing:`
- fatal/error lines tied to shutdown window
- repeated restart loops immediately after shutdown intent

Update section:
- Disqualifying Evidence

Validation:
- Section marked `Completed`
- Each disqualifying signal is classified as `Disqualifying`, `Degraded`, or `Context Only`

## PHASE 5: Correlate PID and Timeline

Objective:
- Ensure evidence lines belong to the same shutdown episode.

Process:
1. Correlate PID/serverId where possible.
2. Build chronological timeline with timestamps.
3. Flag out-of-order or conflicting evidence.

Helpful marker example for process identity:
- `<serverId> launched. PID: <pid>`

Update section:
- PID and Timeline Correlation

Validation:
- Section marked `Completed`
- Timeline includes at least 4 timestamped entries

## PHASE 6: Decide Verdict Inputs

Objective:
- Convert collected evidence into deterministic verdict inputs.

Set these fields:
- Mandatory positive signals present: Yes/No
- Any disqualifying signal present: Yes/No
- Evidence consistency: High/Medium/Low
- Missing data impact: None/Minor/Major

Update section:
- Final Verdict Inputs

Validation:
- Section marked `Completed`
- All four fields populated with rationale

## PHASE 7: Create Final Evidence Report

Create `./tmp/rialtoserver-shutdown-evidence.md` with this exact structure:

```markdown
# RialtoServer Shutdown Evidence Report

## Verdict
- Result: Pass | Degraded | Fail | Inconclusive
- Confidence: High | Medium | Low
- Time window analyzed:

## Mandatory Positive Evidence
- [E1] <timestamp> <source> <exact line>
- [E2] <timestamp> <source> <exact line>

## Teardown Sequence Timeline
1. <timestamp> <event>
2. <timestamp> <event>
3. <timestamp> <event>

## Disqualifying or Degraded Signals
- [D1] <timestamp> <source> <exact line>
  - Classification: Disqualifying | Degraded | Context Only
  - Reason:

## Evidence Gaps
- Missing log/source:
- Impact on verdict:

## Final Reasoning
- Why this is Pass/Degraded/Fail/Inconclusive:
- What additional evidence would change the verdict:
```

Verdict rules:
- Pass:
  - mandatory positive evidence present
  - ordered teardown sequence present
  - no disqualifying signals
- Degraded:
  - teardown completed but forced-kill/timeout/error shutdown signals exist
- Fail:
  - shutdown did not complete, or evidence shows crash/abort without clean exit
- Inconclusive:
  - insufficient logs to prove completion

Validation before completion:
- Report file exists
- Every key claim has quoted log evidence
- Verdict strictly follows rules above

## Completion Checklist

- All phases completed in sequence
- Inventory sections all marked `Completed`
- Final report created at `./tmp/rialtoserver-shutdown-evidence.md`
- Chat summary includes verdict, strongest evidence, and one next action
