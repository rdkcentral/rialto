# Server Manager Crash Analysis - Working Inventory

Input case: RDKEMW-21825
Analyst run: dry-run of `/servermanager-crash-analysis`

## 1. Inputs, Fault Marker, and Time Window (Completed)

Log sources:
- `.../07-23-26-02-38PM-logbackup/sky-messages.log` (primary application/device stream, 24494 lines,
  window 2026-07-23T14:00:28Z -> 14:31:52.380Z, UTC)
- `.../07-23-26-02-38PM-logbackup/system.log` (systemd/init + kernel audit)
- `.../07-23-26-02-38PM-logbackup/core_log.txt` (coredump record)

Stack trace (user-provided, from lib32-rialto v0.22.3-r0):
- `serverManager/common/source/SessionServerAppManager.cpp:363`
- `bits/std_function.h:590 std::execute_native_thread_routine`
- `thread.cc:82 start_thread`
- `pthread_create.c:442 clone`

Fault marker (exact lines):
- `2026-07-23T14:31:45.741Z process crashed = appsserviced` (core_log.txt)
- `2026-07-23T14:31:45.761Z signal causing dump = 11` (core_log.txt)
- `2026-07-23T14:31:45.785Z Process ID = 5316` / `Thread ID within process = 5547` (core_log.txt)
- `2026-07-23T14:31:45.418Z audit[5316]: ANOM_ABEND ... comm="appsserviced" ... sig=11 res=1` (system.log)

Faulting process: appsserviced, PID 5316, thread 5547, signal 11 (SIGSEGV), at 14:31:45.741Z.
Analysis window: 14:31:38Z -> 14:31:52.4Z.
Version note: reported lib is v0.22.3-r0; workspace is a different tag -> possible line drift, reason about
the enclosing function.

## 2. Stack Trace and Code Mapping (Completed)

Top Server Manager frame: `SessionServerAppManager.cpp:363`, running on a native `std::thread` via
`std::function` (`execute_native_thread_routine`) = the app-manager EventThread.

Workspace mapping (current tree): line 363 = `connectSessionServer(preloadSessionServer());` inside
`handleSessionServerStateChange`, the `newState == ERROR && sessionServer->isPreloaded()` branch, guarded
by `if (!m_isShuttingDown)`. This is the preloaded-server respawn path.

## 3. Lifecycle Timeline (Completed)

1. 14:31:38.918Z `systemd[1]: Stopping Sky Apps Service Gateway...` (system.log)
2. 14:31:40.113Z `appsserviced[5316]: Connection to serverId: 0 broken, server probably crashed. Starting recovery`
3. 14:31:40.117Z `appsserviced[5316]: detected term signal, shutting down`
4. 14:31:40.129Z `appsserviced[5316]: Connection to serverId: 1 broken, server probably crashed. Starting recovery`
5. 14:31:40.354Z `RialtoServer[15721]: ... main.cpp F:main` (respawn launched during teardown)
6. 14:31:40.378Z `RialtoServer[15721]: ... switchToActive ... state is ACTIVE now`
7. 14:31:44.701Z `RialtoServer[15721]: ... switchToNotRunning ... requested`
8. 14:31:45.062Z `appsserviced[5316]: Ping with id: 185 failed for server: 1`
9. 14:31:45.065Z `appsserviced[5316]: RialtoServerManager is closing...`
10. 14:31:45.103Z `RialtoServer[16093]: ... main.cpp F:main` (preload respawn AFTER "closing")
11. 14:31:45.741Z `process crashed = appsserviced` / signal 11 (core_log.txt)

## 4. Context Classification (Teardown vs Operation) (Completed)

Classification: System Teardown (orderly, systemd-driven).
Evidence:
- `systemd[1]: Stopping Sky Apps Service Gateway...` (14:31:38.918Z) and `Stopped target Multi-User System`.
- Host `detected term signal, shutting down` (14:31:40.117Z).
- Post-crash: `sky-appsservice.service: Failed with result 'signal'` (14:31:51.272Z) then systemd continued
  stopping every unit and `Unmounted /opt/secure` (14:31:52.299Z) -> teardown completed cleanly.
SME match: `serverManager/SME-notes.md` RDKEMW-19123 teardown/reconnect race (preload respawn during
shutdown). Signature matches.

## 5. Crash vs Teardown-Driven Exit (per process) (Completed)

- appsserviced (5316): Faulted. Coredump + kernel ANOM_ABEND, sig 11.
- RialtoServer session servers (serverId 0/1, and 15721/16093): No coredump and no ANOM_ABEND for any
  RialtoServer in core_log.txt / system.log -> Externally Terminated / Orderly Exit, NOT a fault crash.
  The "probably crashed. Starting recovery" lines are a misclassification of teardown-driven deaths (the
  session-server children are torn down by the systemd cgroup stop before the manager marks itself
  shutting down). Exact kill signal not individually logged (inference: SIGTERM via cgroup).

## 6. Root Cause and Severity Inputs (Completed)

- Root cause hypothesis: During systemd teardown, late disconnect/ping-failure callbacks drove a preloaded
  session server to ERROR on the app-manager EventThread; the ERROR+isPreloaded branch respawned a preload
  (`connectSessionServer(preloadSessionServer())`, line 363) concurrently with `RialtoServerManager`
  destruction -> use-after-free/race -> SIGSEGV. [E: timeline 9->10->11; SME RDKEMW-19123]
- Confidence: High (frame + timeline + SME signature align; version line-drift is the only caveat).
- Context: System Teardown.
- Functional impact: None (reboot/teardown completed cleanly).
- Recurrence/bound: rare, teardown-bound in observed data; underlying race is UB so not provably bounded.
- Secondary cost: Yes - coredump generated and uploaded (`corename = ...signal_11.gz`), inflates crash KPIs.
