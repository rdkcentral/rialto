---
description: Determine whether a RialtoServer restart was caused by the Server Manager healthcheck mechanism, using explicit causal log evidence.
---

# RialtoServer Healthcheck Restart Attribution

Use this prompt to understand the healthcheck mechanism and determine if a RialtoServer restart happened because of it.

## Scope

- Analyze user-provided logs and optional config inputs.
- Focus on Server Manager healthcheck, ping/ack flow, timeout/failure handling, and restart path.
- Correlate evidence by serverId, appId, and timestamp.
- Use these references for expected behavior and terminology:
  - `serverManager/architecture-brief.md`
  - `serverManager/SME-notes.md`
- If user input includes a folder named `logs`, also check for rotated message logs when present:
  - `logs/sky-messages.log`
  - `logs/sky-messages.log.1`
  - `logs/sky-messages.log.2`
  - `logs/sky-messages.log.3`

## Critical Constraints

- You MUST complete phases in order.
- You MUST quote exact log lines for every key claim.
- You MUST include timestamp and log source for each evidence item.
- You MUST separate causal evidence from contextual evidence.
- You MUST NOT modify source code.
- If causal chain is incomplete, verdict MUST be Inconclusive.

## Output Requirements

Produce:
1. Working document: ./tmp/rialtoserver-healthcheck-restart-inventory.md
2. Final report: ./tmp/rialtoserver-healthcheck-restart-report.md
3. Chat summary: one-paragraph attribution verdict and next action

Verdict options:
- Healthcheck-Caused Restart: Confirmed
- Healthcheck-Caused Restart: Not Confirmed
- Inconclusive

Status flow:
- Not Started -> In Progress -> Completed

---

## PHASE 0: Create Working Structure

Create ./tmp/rialtoserver-healthcheck-restart-inventory.md with sections:

1. Inputs and Analysis Window (Not Started)
2. Healthcheck Mechanism Evidence (Not Started)
3. Ping and Ack Failure Evidence (Not Started)
4. Recovery Threshold Evidence (Not Started)
5. Restart Path Evidence (Not Started)
6. Causal Chain and Counterfactuals (Not Started)
7. Verdict Inputs (Not Started)

Validation:
- File exists
- All sections exist
- All sections start as Not Started

## PHASE 1: Identify Inputs and Window

Objective:
- Define exactly what logs and time span are analyzed.

Process:
1. List each log source and path.
2. Define window start/end and timezone.
3. Record target entity keys (serverId, appId, PID if available).

Update section:
- Inputs and Analysis Window

Validation:
- Section marked Completed
- At least one log source and one time window recorded

## PHASE 2: Explain Healthcheck Mechanism from Evidence

Objective:
- Build a concise mechanism summary tied to observed logs.

Look for mechanism markers such as:
- Start ping procedure with id
- Queue ping procedure with id
- Queue ack handling for serverId ping id
- Ping timeout for server id
- Ack with error received
- Max num of failed pings reached for server with id: X. Starting recovery action

Also accept marker variants commonly emitted in field logs:
- failed to ping due to 'Timed out'
- Ping with id: N failed for server: X

Update section:
- Healthcheck Mechanism Evidence

Validation:
- Section marked Completed
- At least 3 mechanism markers found or explicitly missing

## PHASE 3: Capture Ping and Ack Failure Evidence

Objective:
- Prove failures accumulated for the same server entity.

Process:
1. Capture failed ping sends and timeout events.
2. Capture ack failures, unexpected acks, and late acks.
3. Correlate by serverId and pingId sequence.

Update section:
- Ping and Ack Failure Evidence

Validation:
- Section marked Completed
- At least 2 failure-related events tied to same serverId, or explicit gap documented

## PHASE 4: Capture Recovery Threshold Evidence

Objective:
- Prove the recovery threshold condition was hit.

Primary marker to seek:
- Max num of failed pings reached for server with id: X. Starting recovery action

Supporting markers:
- repeated ping timeout or ack error for same serverId
- session state moved to Error before restart

Update section:
- Recovery Threshold Evidence

Validation:
- Section marked Completed
- Threshold marker found or explicitly not found

## PHASE 5: Capture Restart Path Evidence

Objective:
- Prove restart action was executed for the same server entity.

Restart path markers:
- Queue restart server handling for serverId: X
- Restarting server with id: X
- Server with id: X exited
- Relaunch or reconnect success/failure for same serverId

Disqualifying competing causes to check:
- startup-timeout kill path (Killing: X)
- manual/admin stop request
- explicit non-healthcheck restart trigger

Update section:
- Restart Path Evidence

Validation:
- Section marked Completed
- At least 2 restart-path markers for same serverId, or explicit gap documented

## PHASE 6: Build Causal Chain and Counterfactuals

Objective:
- Distinguish causation from simple temporal correlation.

Build causal chain template:
1. Failure signal(s)
2. Threshold/recovery trigger
3. Restart request
4. Restart execution

Counterfactual checks:
- Could restart be explained by startup timeout?
- Could restart be explained by manual command?
- Could restart be explained by unrelated crash path?

Update section:
- Causal Chain and Counterfactuals

Validation:
- Section marked Completed
- Each link in chain marked Present, Missing, or Ambiguous

## PHASE 7: Decide Verdict Inputs

Objective:
- Compute deterministic inputs for final attribution verdict.

Set fields:
- Same serverId continuity across chain: Yes or No
- Threshold marker present: Yes or No
- Restart marker present after threshold: Yes or No
- Competing cause evidence present: Yes or No
- Overall evidence confidence: High, Medium, Low

Update section:
- Verdict Inputs

Validation:
- Section marked Completed
- All fields populated with rationale

## PHASE 8: Create Final Attribution Report

Create ./tmp/rialtoserver-healthcheck-restart-report.md with this exact structure:

```markdown
# RialtoServer Healthcheck Restart Attribution Report

## Verdict
- Result: Healthcheck-Caused Restart: Confirmed | Healthcheck-Caused Restart: Not Confirmed | Inconclusive
- Confidence: High | Medium | Low
- Time window analyzed:
- Target serverId/appId:

## Healthcheck Mechanism Evidence
- [M1] <timestamp> <source> <exact line>
- [M2] <timestamp> <source> <exact line>

## Failure and Threshold Evidence
- [F1] <timestamp> <source> <exact line>
- [T1] <timestamp> <source> <exact line>

## Restart Path Evidence
- [R1] <timestamp> <source> <exact line>
- [R2] <timestamp> <source> <exact line>

## Causal Chain
1. <failure event>
2. <threshold trigger>
3. <restart request>
4. <restart execution>

## Competing Causes Check
- Startup-timeout path evidence:
- Manual/admin restart evidence:
- Other crash-path evidence:
- Conclusion:

## Evidence Gaps
- Missing logs or fields:
- Impact on confidence:

## Final Reasoning
- Why verdict is Confirmed/Not Confirmed/Inconclusive:
- What additional evidence would change the verdict:
```

Verdict rules:
- Confirmed:
  - threshold marker present for target serverId
  - restart request and restart execution markers present for same serverId
  - no stronger competing-cause evidence
- Not Confirmed:
  - restart happened, but threshold marker missing and evidence points to other cause
- Inconclusive:
  - missing or ambiguous links in causal chain

Validation before completion:
- Final report exists
- Every key claim is tied to quoted evidence
- Verdict strictly follows rules

## Completion Checklist

- All phases completed in sequence
- Inventory sections all marked Completed
- Final report created at ./tmp/rialtoserver-healthcheck-restart-report.md
- Chat summary includes verdict, strongest evidence, and one next action
