# Rialto Server Manager Skill Prefilled Template

Copy this file to:
.github/skills/<your-skill-name>/SKILL.md

Then update the placeholders marked with angle brackets.

```markdown
---
name: <your-skill-name-kebab-case>
description: 'Use when the user wants to analyze Rialto Server Manager behavior from logs, including shutdown quality, lifecycle transitions, and state coverage.'
argument-hint: 'Provide log paths, analysis time window, and optional appId or serverId filter.'
user-invocable: true
---

# <Your Skill Title>

This skill guides users through a step-by-step Rialto Server Manager log analysis workflow.

## When to Use

Use this skill when the user needs:
- Evidence of whether RialtoServer shut down properly
- Complete capture of canonical server states
- Timeline of state and lifecycle transitions
- Identification of forced-kill, timeout, or restart-loop patterns

Do not use this skill when:
- The user asks for direct code implementation instead of log analysis
- No logs or time window can be provided

## Input

Required:
- Log sources to analyze
- Time window start and end

Optional:
- appId filter
- serverId filter
- expected behavior baseline

If required input is missing, ask for it before proceeding.

## Optional Repository Reference Pack

If available in the repository, align analysis with:
- `serverManager/architecture-brief.md`
- `serverManager/SME-notes.md`
- `docs/architecture-brief.md`

## Canonical States

Track coverage for this exact set:
1. Uninitialized
2. Inactive
3. Active
4. NotRunning
5. Error

## Steps

1. **Confirm scope and assumptions**
- Confirm target service/process and time window.
- Record timezone and identifier assumptions.
- STOP and wait if scope is ambiguous.

2. **Collect direct evidence lines**
- Extract exact lines with timestamp and source.
- Tag each line with appId or serverId when present.
- Separate canonical state evidence from ambiguous lines.

3. **Build transition timeline**
- Sort by timestamp.
- Build per-entity transition chain.
- Flag out-of-order or conflicting transitions.

4. **Evaluate shutdown and lifecycle quality**
- Identify orderly teardown signals.
- Identify degraded or disqualifying signals such as timeouts, forced kills, or restart loops.
- STOP and ask user if extra logs are needed to resolve contradictions.

5. **Compute verdict inputs**
- Canonical state coverage count out of 5
- Transition continuity quality: High, Medium, Low
- Presence of disqualifying signals: Yes or No
- Missing-data impact: None, Minor, Major

6. **Produce final output**
- Inventory of evidence and gaps
- Final verdict with confidence
- One next action recommendation

## Output

Return:
- Coverage summary for Uninitialized, Inactive, Active, NotRunning, Error
- Ordered transition timeline
- Verdict: Pass, Degraded, Fail, or Inconclusive
- Confidence: High, Medium, or Low
- Explicit evidence gaps and their impact

## Guardrails

- Do NOT claim state presence without quoted evidence.
- Do NOT mark Pass if mandatory evidence is missing.
- Do NOT modify source code.
- Do NOT proceed past decision points without user confirmation when inputs are incomplete.
- If findings conflict, report the conflict and ask whether to expand log scope.

## Suggested Companion Prompts

- /rialtoserver-shutdown-evidence
- /rialtoserver-state-capture
```
