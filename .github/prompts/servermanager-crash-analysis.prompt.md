---
description: Diagnose a Rialto Server Manager crash from logs (stack trace optional) - map the fault to code, reconstruct the lifecycle timeline, classify crash vs teardown, and judge severity, with exact log evidence.
---

# Server Manager Crash Analysis

Use this prompt to root-cause a crash reported in, or attributed to, the Rialto Server Manager
library (for example an `appsserviced` SIGSEGV whose callstack points into `serverManager/...`).
Produce a direct, evidence-first analysis with a severity verdict.

Companion skill:
- `/servermanager-crash-analysis` for reusable crash-evidence extraction, code mapping, and
  crash-vs-teardown classification.

## Scope

- Analyze log files provided by the user (device logs, systemd/journal exports, server manager logs,
  crash/coredump records). Logs are the primary input.
- A stack-trace or coredump text file is optional; use it to pinpoint the faulting frame when present.
- Cross-check expected lifecycle, recovery, and shutdown behavior against:
  - `serverManager/architecture-brief.md`
  - `serverManager/SME-notes.md`
- Map the faulting frame to code in the workspace (`serverManager/`, `media/server/`, `ipc/`).
- If the user provides a folder named `logs` (or a `*-logbackup` folder), check for rotated and
  system logs when present and treat rotations as one stream:
  - `sky-messages.log`, `sky-messages.log.1`, `sky-messages.log.2`, `sky-messages.log.3`
  - `system.log`, `syslog_fallback.log`, `messages.txt`, `core_log.txt`, `rebootreason.log`

## Critical Constraints

- You MUST complete phases in order.
- You MUST build evidence before writing conclusions.
- You MUST quote exact log lines for all key claims, each with timestamp and source file/stream.
- You MUST map the top Server Manager stack frame to a concrete `file:function:line` when a trace exists;
  if no trace is provided, derive the suspect code path from log markers instead.
- You MUST prefer verified code over documentation when they conflict, and note the discrepancy.
- You MUST NOT modify source code.
- If mandatory evidence is missing, mark the result `Inconclusive` (not `Confirmed`).
- If the reported library version/commit differs from the workspace, flag possible line-number drift and
  reason about the enclosing function rather than the literal line.

## Output Requirements

Produce:
1. Working document: `./tmp/servermanager-crash-inventory.md`
2. Final report: `./tmp/servermanager-crash-report.md`
3. Chat summary: one-paragraph root cause + severity with the single strongest evidence and next action

Status flow:
- Not Started -> In Progress -> Completed

---

## PHASE 0: Create Working Structure

Create `./tmp/servermanager-crash-inventory.md` with these sections:

1. Inputs, Fault Marker, and Time Window (Not Started)
2. Stack Trace and Code Mapping (Not Started)
3. Lifecycle Timeline (Not Started)
4. Context Classification (Teardown vs Operation) (Not Started)
5. Crash vs Teardown-Driven Exit (per process) (Not Started)
6. Root Cause and Severity Inputs (Not Started)

Validation:
- File exists
- All 6 sections exist
- All sections begin as `Not Started`

## PHASE 1: Identify Inputs and the Fault

Objective:
- Pin exactly what is being analyzed and where/when the crash happened.

Process:
1. List each log source and path; record timezone and clock assumptions.
2. Find the fault marker and capture the faulting process, PID, thread id, signal, and timestamp.
   Marker examples:
   - `process crashed = <name>` / `signal causing dump = <n>` / `corename = ...signal_<n>...`
   - `ANOM_ABEND ... comm="<name>" ... sig=<n>`
   - `<unit>.service: Failed with result 'signal'`
3. Define a tight analysis window bracketing the fault (typically a few seconds before to after).

Update section:
- Inputs, Fault Marker, and Time Window

Validation:
- Section marked `Completed`
- Faulting process, signal, and timestamp captured, or explicit gap recorded

## PHASE 2: Map the Stack Trace to Code

Objective:
- Turn the crash into a concrete code location.

Process:
1. If a stack-trace/coredump file was provided, quote the top frames verbatim.
2. Identify the highest frame that lands in `serverManager/` (or `media/server/`, `ipc/`).
3. Resolve it to `file:function:line` in the workspace; summarize what that code does and which
   thread/callback it runs on (for example the app-manager EventThread, an IPC-loop thread).
4. If no trace exists, derive the suspect code path from log markers and state so explicitly.

Update section:
- Stack Trace and Code Mapping

Validation:
- Section marked `Completed`
- Either a mapped `file:function:line` with rationale, or an explicit "no trace; path inferred from logs"

## PHASE 3: Reconstruct the Lifecycle Timeline

Objective:
- Build an ordered, timestamped chain of events around the fault.

Capture and order signals such as:
- State transitions and observer events (`switchToActive`, `switchToNotRunning`, state changed).
- Disconnect/recovery: `Connection to serverId: <id> broken, server probably crashed. Starting recovery`.
- Healthcheck: `Ping with id: <n> failed for server: <id>`, `Max num of failed pings reached ...`.
- Preload/restart/spawn: `<id> launched. PID: <pid>`, new `RialtoServer[<pid>] ... main.cpp`.
- Shutdown intent: `RialtoServerManager is closing...`, host `detected term signal, shutting down`.

Update section:
- Lifecycle Timeline

Validation:
- Section marked `Completed`
- At least 4 timestamped, sourced entries, ordered chronologically

## PHASE 4: Classify the Context

Objective:
- Decide whether the fault occurred during an orderly system-wide teardown or during normal operation.

Process:
1. Look for init/system teardown markers: `systemd[1]: Stopping <unit>...`,
   `Stopped target Multi-User System`, host `detected term signal`.
2. Cross-check `serverManager/SME-notes.md` for a matching incident signature (for example the
   RDKEMW-19123 shutdown/reconnect race) and note the match or absence.

Update section:
- Context Classification (Teardown vs Operation)

Validation:
- Section marked `Completed`
- Context labeled `System Teardown`, `Normal Operation`, or `Unclear`, with cited evidence

## PHASE 5: Distinguish Crash vs Teardown-Driven Exit

Objective:
- For each relevant process, separate a genuine fault from an orderly/externally-driven exit.

Process:
1. Confirm which processes actually dumped (coredump / `ANOM_ABEND`) versus which merely disconnected.
2. Note where "server probably crashed" recovery is a misclassification of a teardown death (no
   coredump, killed as a child of the host being stopped).
3. Correlate PID/serverId to keep evidence within one episode.

Update section:
- Crash vs Teardown-Driven Exit (per process)

Validation:
- Section marked `Completed`
- Each process classified `Faulted`, `Orderly Exit`, or `Externally Terminated`, with evidence

## PHASE 6: Decide Root Cause and Severity

Objective:
- Convert evidence into a root cause and a severity call.

Set these fields:
- Root cause hypothesis: <statement> (with the key evidence ids)
- Confidence: High | Medium | Low
- Context: System Teardown | Normal Operation | Unclear
- Functional impact: None | Minor | Major (did teardown/reboot still complete? any user-visible effect?)
- Recurrence/bound: rare-teardown-only | possibly-broader | unknown
- Secondary cost: crash-telemetry/coredump upload, KPI inflation (Yes/No)

Update section:
- Root Cause and Severity Inputs

Validation:
- Section marked `Completed`
- All fields populated with rationale

## PHASE 7: Create Final Report

Create `./tmp/servermanager-crash-report.md` with this exact structure:

```markdown
# Server Manager Crash Analysis Report

## Verdict
- Root cause: <one sentence>
- Confidence: High | Medium | Low
- Context: System Teardown | Normal Operation | Unclear
- Severity: Low | Medium | High
- Time window analyzed:

## Fault
- Process / PID / thread / signal:
- Mapped code location: <file:function:line> (or "inferred from logs")

## Key Evidence
- [E1] <timestamp> <source> <exact line>
- [E2] <timestamp> <source> <exact line>

## Timeline
1. <timestamp> <event>
2. <timestamp> <event>
3. <timestamp> <event>

## Crash vs Teardown Classification
- <process>: Faulted | Orderly Exit | Externally Terminated - <evidence>

## Severity Rationale
- Functional impact:
- Recurrence/bound:
- Secondary cost (telemetry/KPIs):

## Gaps / Inconclusive Items
- <missing evidence and its impact>

## Recommended Next Action
- <single most useful next step>
```

Validation:
- Report file created with all sections
- Verdict, Fault, and at least 2 Key Evidence items populated

## PHASE 8: Chat Summary

Provide a one-paragraph verdict: root cause, context, severity, the single strongest evidence line
(with timestamp), and the recommended next action. Keep it direct.
