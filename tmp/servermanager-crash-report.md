# Server Manager Crash Analysis Report

## Verdict
- Root cause: A teardown/reconnect race - during systemd shutdown, a preloaded session server was driven
  to ERROR and respawned (`connectSessionServer(preloadSessionServer())`) on the app-manager EventThread
  while `RialtoServerManager` was being destroyed, causing a use-after-free SIGSEGV.
- Confidence: High
- Context: System Teardown
- Severity: Low
- Time window analyzed: 2026-07-23T14:31:38Z -> 14:31:52.4Z (UTC)

## Fault
- Process / PID / thread / signal: appsserviced / 5316 / 5547 / signal 11 (SIGSEGV) at 14:31:45.741Z
- Mapped code location: `serverManager/common/source/SessionServerAppManager.cpp:363`
  = `connectSessionServer(preloadSessionServer())` in `handleSessionServerStateChange`
  (`ERROR && isPreloaded` branch), on the app-manager EventThread (native std::thread via std::function).
  Caveat: trace is from lib32-rialto v0.22.3-r0; workspace tag differs, so line 363 is mapped by enclosing
  function, not guaranteed identical line.

## Key Evidence
- [E1] 2026-07-23T14:31:45.741Z core_log.txt `process crashed = appsserviced`
- [E2] 2026-07-23T14:31:45.418Z system.log `audit[5316]: ANOM_ABEND ... comm="appsserviced" ... sig=11 res=1`
- [E3] 2026-07-23T14:31:38.918Z system.log `systemd[1]: Stopping Sky Apps Service Gateway...`
- [E4] 2026-07-23T14:31:45.065Z sky-messages.log `appsserviced[5316]: RialtoServerManager is closing...`
- [E5] 2026-07-23T14:31:45.103Z sky-messages.log `RialtoServer[16093]: ... main.cpp F:main` (preload respawn AFTER "closing")
- [E6] 2026-07-23T14:31:40.113Z sky-messages.log `appsserviced[5316]: Connection to serverId: 0 broken, server probably crashed. Starting recovery`

## Timeline
1. 14:31:38.918Z systemd begins stopping the Apps Service Gateway (teardown start)
2. 14:31:40.113/129Z serverId 0/1 sockets break -> "probably crashed. Starting recovery"
3. 14:31:40.117Z appsserviced receives term signal
4. 14:31:40.354 -> .378Z RialtoServer[15721] respawned and driven to ACTIVE
5. 14:31:44.701Z RialtoServer[15721] moved to NOT_RUNNING
6. 14:31:45.065Z "RialtoServerManager is closing..." (destructor begins)
7. 14:31:45.103Z RialtoServer[16093] preload respawn launches (races destruction)
8. 14:31:45.741Z appsserviced SIGSEGV

## Crash vs Teardown Classification
- appsserviced (5316): Faulted - coredump + kernel ANOM_ABEND, sig 11.
- RialtoServer session servers (0/1, 15721, 16093): Externally Terminated / Orderly Exit - no coredump,
  no ANOM_ABEND for any RialtoServer. The "probably crashed" recovery is a misclassification of
  teardown-driven child deaths (systemd cgroup stop; exact signal not individually logged - inferred SIGTERM).

## Severity Rationale
- Functional impact: None. After the crash, systemd logged `sky-appsservice.service: Failed with result
  'signal'` (14:31:51.272Z) and continued the orderly shutdown, unmounting `/opt/secure` (14:31:52.299Z).
  Reboot/teardown completed; the service was not restarted (it was stopping anyway).
- Recurrence/bound: Rare and, in this data, only on system-wide teardown. But the underlying defect is a
  memory-safety race (undefined behavior), so it is not provably confined to teardown.
- Secondary cost (telemetry/KPIs): A `signal_11` coredump is generated and uploaded
  (`corename = 1784817105_core.prog_appsserviced.signal_11.gz`), which inflates fleet crash metrics.

## Gaps / Inconclusive Items
- Exact termination signal for the original serverId 0/1 is not individually logged (RialtoServer children
  are not systemd units); SIGTERM-via-cgroup is inference, not direct evidence.
- Line-number fidelity vs v0.22.3-r0 is assumed by enclosing function, not verified against that tag.

## Recommended Next Action
- Low urgency given no functional impact. If crash-telemetry hygiene matters, close the teardown/reconnect
  race per `serverManager/SME-notes.md` RDKEMW-19123 (gate preload/restart once shutdown starts; see PR #512
  direction) and/or classify session-server exit reason so teardown deaths don't trigger recovery.
