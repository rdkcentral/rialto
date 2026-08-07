---
name: servermanager-crash-analysis
description: 'Diagnose a Rialto Server Manager crash from logs (and an optional stack trace). Use when a process such as appsserviced dumps with a signal inside the Server Manager library, to map the fault to code, reconstruct the lifecycle timeline, distinguish a genuine crash from a teardown-driven session-server exit, and judge severity.'
argument-hint: 'Provide the log file or logs/ folder, an optional stack-trace/coredump text file, and (optionally) the RialtoServer/library version or commit id.'
---

# Server Manager Crash Analysis

## When to Use

Use this skill when a crash (SIGSEGV/SIGABRT/SIGBUS/etc.) is reported in, or attributed to,
the Rialto Server Manager library, and you need an evidence-first root-cause and severity call.
Typical triggers:
- A host process (for example `appsserviced`) dumps with a signal and the callstack points into
  `serverManager/...`.
- Restart/preload churn or "server probably crashed" recovery is seen around the fault.
- A crash occurs during a system-wide teardown and you must judge whether it matters.

Reference behavior model (source of truth, in this precedence):
1. Verified implementation in code (`serverManager/`, `media/server/`, `ipc/`).
2. `serverManager/architecture-brief.md` for lifecycle, healthcheck, and recovery semantics.
3. `serverManager/SME-notes.md` for known incident signatures (for example RDKEMW-19123 shutdown race).

## Required Inputs

- Log sources to analyze (primary input).
- Optional: a stack-trace or coredump text file (top frames with file:line).
- Optional: RialtoServer/library version or commit id, to map frame line numbers accurately.

Default log discovery convention:
- If the user provides a folder named `logs` (or a `*-logbackup` folder), check for these if present
  and treat them as one rotated stream, preserving the filename in every citation:
  - `sky-messages.log`, `sky-messages.log.1`, `sky-messages.log.2`, `sky-messages.log.3`
  - System/init evidence: `system.log`, `syslog_fallback.log`, `messages.txt`
  - Crash evidence: `core_log.txt`, `rebootreason.log`, `rebootInfo.log`
- If a needed source is absent, record the gap explicitly and continue.

If inputs are incomplete for a firm conclusion, request the missing values before the final verdict.

## Crash Signal Vocabulary

Treat these as authoritative crash/fault markers (quote them verbatim with timestamp and source):
- `process crashed = <name>` / `signal causing dump = <n>` / `corename = ...signal_<n>...` (core_log.txt)
- `ANOM_ABEND ... comm="<name>" ... sig=<n>` (kernel audit in system.log/syslog)
- `<unit>.service: Failed with result 'signal'` / `Main process exited, code=killed, status=<n>` (systemd)

Server Manager lifecycle / recovery markers (from code and prior incidents):
- `Connection to serverId: <id> broken, server probably crashed. Starting recovery`
- `Connection to serverId: <id> broken, but server manager is shutting down`
- `RialtoServerManager is closing...`
- `Max num of failed pings reached for server with id: <id>. Starting recovery action`
- `Waitpid timeout. Killing: <id>` / `<id> launched. PID: <pid>` / `Server with id: <id> exited.`
- RialtoServer side: `switchToActive`, `switchToNotRunning`, `Rialto Server Manager disconnected`

System teardown markers (to classify context):
- `systemd[1]: Stopping <unit>...` / `Stopped target Multi-User System`
- host process: `detected term signal, shutting down`

## Procedure

1. Locate the fault
- Find the crash marker, the faulting process/PID/thread, the signal, and the exact timestamp.
- If a stack trace is provided, capture the top frames verbatim; identify the highest frame that lands
  in `serverManager/` (or `media/server/`).

2. Map the fault to code
- Resolve the top Server Manager frame to `file:function:line` in the workspace.
- If a version/commit was given and differs from the workspace, flag possible line drift and reason
  about the surrounding function rather than the literal line.

3. Reconstruct the lifecycle timeline
- Build an ordered, timestamped chain of state transitions, disconnects, ping failures, restart/preload
  activity, and shutdown markers around the fault window. Preserve source filename per line.

4. Classify the context
- Determine whether this is a system-wide, orderly teardown (systemd stopping targets; host SIGTERM) or
  a fault during normal operation. Cross-check `SME-notes.md` for a matching incident signature.

5. Distinguish crash vs teardown-driven exit (per session server)
- A genuine crash of a session server produces a coredump/`ANOM_ABEND`; an orderly stop does not.
- Recovery ("probably crashed") fires on any unexpected disconnect the manager cannot attribute to a
  `NOT_RUNNING` it drove — note when that is a misclassification of a teardown death.

6. Root cause + severity
- State the most-supported root cause with its evidence.
- Judge severity: functional impact (did teardown/reboot still complete?), recurrence rarity, whether it
  is bounded to teardown, and crash-telemetry cost (each dump uploads a core and inflates crash KPIs).

7. Produce outputs
- Inventory of evidence and gaps, a final report with root cause + severity + confidence, and one concise
  chat verdict with the single strongest piece of evidence and a recommended next action.

## Constraints

- Evidence-first: every key claim is backed by an exact quoted line with timestamp and source.
- Do NOT modify source code.
- If mandatory evidence is missing, mark the result `Inconclusive` (never `Confirmed`).
- Prefer verified code over documentation when they conflict, and note the discrepancy.
