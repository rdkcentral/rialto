---
name: rialto-servermanager-log-evidence
description: 'Extract and validate Rialto Server Manager lifecycle evidence from logs. Use when analyzing RialtoServer shutdown, state transitions, process exits, restart loops, or missing state coverage for Uninitialized, Inactive, Active, NotRunning, and Error.'
argument-hint: 'Provide log paths, time window, and optional appId or serverId filter.'
---

# Rialto Server Manager Log Evidence

## When to Use

Use this skill when you need reliable, evidence-first conclusions from logs for:
- RialtoServer shutdown quality
- session server state transition tracking
- forced-kill or timeout diagnosis
- state coverage checks across canonical states

Reference behavior model:
- `serverManager/architecture-brief.md` for lifecycle and healthcheck semantics.
- `serverManager/SME-notes.md` for operational failure signatures and incident context.

## Required Inputs

- Log sources to analyze
- Time window start and end
- Optional scope filters: appId, serverId, process id

Default log discovery convention:
- If the user provides a folder named `logs`, automatically check for these files if present:
  - `logs/sky-messages.log`
  - `logs/sky-messages.log.1`
  - `logs/sky-messages.log.2`
  - `logs/sky-messages.log.3`
- Treat discovered files as the same rotated stream and preserve filename in evidence citations.
- If none are present, explicitly record the gap and continue with other provided sources.

If inputs are incomplete, request the missing values before final verdict.

## Canonical State Set

Always use this exact state set for coverage and transition checks:
1. Uninitialized
2. Inactive
3. Active
4. NotRunning
5. Error

## Procedure

1. Build an input table
- List each source path or stream name
- Record timezone assumptions
- Define analysis window

2. Extract direct evidence lines
- Capture exact lines with timestamp and source
- Collect state and lifecycle lines only
- Tag by appId and serverId where present
- Accept equivalent marker variants when exact canonical strings differ
  (for example, `failed to ping due to 'Timed out'` and
  `Ping with id: N failed for server: X`).

3. Reconstruct ordered timeline
- Sort by timestamp
- Build per-entity transition chains
- Flag out-of-order or conflicting transitions

4. Classify shutdown quality signals
- Positive examples: orderly channel close, thread exit, process exit
- Negative examples: timeout kill, forced kill, repeated restart loops

5. Compute deterministic verdict inputs
- Canonical state coverage count
- Transition continuity quality
- Presence of disqualifying signals
- Missing-data impact

6. Produce outputs
- Inventory of evidence and gaps
- Final report with verdict and confidence
- One concise summary with next action

## Evidence Rules

- Never claim a state without a quoted line.
- Never mark pass when mandatory evidence is missing.
- Mark ambiguous lines separately from canonical evidence.
- Keep conclusions traceable to specific log lines.

## Suggested Output Shape

- Verdict: Pass, Degraded, Fail, or Inconclusive
- Confidence: High, Medium, or Low
- Coverage: observed canonical states out of 5
- Top evidence: strongest 2 to 5 lines
- Gaps: missing logs and impact on confidence

## Related Repo Prompts

- Use [/rialtoserver-shutdown-evidence](../../prompts/rialtoserver-shutdown-evidence.prompt.md) for shutdown verdict workflow.
- Use [/rialtoserver-state-capture](../../prompts/rialtoserver-state-capture.prompt.md) for full state coverage workflow.
