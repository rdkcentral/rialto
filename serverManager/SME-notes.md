# SME Notes: RialtoServerManager

## Operations

### Deployment
How to deploy this system.
- Pre-deployment checklist
  - Confirm required build dependencies are installed (protobuf, pthreads, mongoose for simulator, jsoncpp if config-file support is enabled).
  - Confirm `RialtoServer` runtime binary path is valid for target environment.
  - Decide whether JSON config support is required (`RIALTO_ENABLE_CONFIG_FILE`) and verify config file locations.
  - Verify socket permission/ownership policy for session management sockets in target environment.
  - If simulator is used in the environment, verify TCP port `9008` availability.
- Deployment commands
  - Configure build:
    - `cmake -S . -B build`
  - Build server manager libraries and simulator:
    - `cmake --build build --target RialtoServerManager`
    - `cmake --build build --target RialtoServerManagerSim`
  - Install artifacts (example):
    - `cmake --install build`
  - Optional simulator run:
    - `./build/serverManager/RialtoServerManagerSim`
- Post-deployment verification
  - Verify `RialtoServerManager` library is installed and loadable by host process.
  - Verify basic app lifecycle path via integration host or simulator:
    - initiate app to `Inactive`
    - transition to `Active`
    - transition to `NotRunning`
  - Verify healthcheck behavior by checking for ping/ack activity and expected recovery on forced session-server disconnect.
  - If simulator is deployed, verify endpoints:
    - `GET /GetState/<AppName>`
    - `POST /SetState/<AppName>/<NewState>`
    - `GET /GetAppInfo/<AppName>`

### Monitoring
What to monitor and where.
- Key metrics
  - Host process liveness for component embedding `RialtoServerManager`.
  - Number of managed session servers (preloaded + assigned).
  - State distribution by app (`UNINITIALIZED`, `INACTIVE`, `ACTIVE`, `NOT_RUNNING`, `ERROR`).
  - Healthcheck failures and restart count (`numOfFailedPingsBeforeRecovery` threshold events).
  - IPC disconnect events between manager and session servers.
  - Startup timeout events for new/preloaded session servers.
- Dashboard locations
  - No built-in dashboard in this folder.
  - Use platform observability dashboards/log search for:
    - server manager logs
    - host process/service status
    - child `RialtoServer` process lifecycle
- Alert thresholds
  - Recommended starting thresholds:
    - Session server restart bursts (for example >= 3 restarts for same app within 10 minutes): warning.
    - Persistent `ERROR` state for same app (for example > 60 seconds): warning.
    - Startup timeout occurrences above baseline: warning.
    - Complete loss of manager-to-session-server IPC connectivity: critical.

### Common Issues
Problems that happen regularly.
- Symptom -> Cause -> Fix
  - App stuck in `NOT_RUNNING` after initiate call -> launch/configuration path failed (socket bind, IPC setup, or startup timeout) -> check server manager logs and validate session-server path/permissions.
  - Frequent app flapping between `ERROR` and recovery states -> aggressive healthcheck policy or unstable session-server process -> tune healthcheck interval/threshold and inspect session-server crash causes.
  - `changeSessionServerState` returns failure -> app not found in manager map or IPC RPC failure -> verify app was initiated and client exists for server id.
  - Preloaded pool drains unexpectedly -> repeated preload startup/connect failures -> validate environment vars, binary path, and socket ownership/permissions.
  - Log-level updates appear partial -> one or more managed clients failed `setLogLevels` RPC -> inspect IPC health and retry after connectivity recovers.

## Tribal Knowledge

### Gotchas
Things that surprise people.
- Healthcheck uses one periodic timer with outstanding-ack tracking, not one timer per server.
- Late ack handling is strict by ping id; stale ack does not clear current outstanding ping.
- Recovery may use hard kill (`SIGKILL`) for session-server restart paths; this is robust but not graceful.
- `getAppConnectionInfo` returns empty string if app is not known; this is a normal failure form, not an exception.
- Simulator startup banner says localhost, but it binds `0.0.0.0:9008` in code.

### Historical Context
Why things are the way they are.
- The service is split into `public`, `service`, `common`, and `ipc` layers to keep API, orchestration policy, and transport concerns separated.
- Event-thread serialization in app manager exists to avoid lifecycle race conditions across async callbacks.
- Preloading exists to reduce first-launch latency for session servers.
- Healthcheck policy is threshold-based to tolerate transient blips before restart.

### Workarounds
Known issues and their workarounds.
- If repeated startup timeouts occur in constrained environments, temporarily increase startup timeout and/or reduce preload count.
- If socket permission issues appear across deployments, standardize ownership/group defaults in config and verify filesystem policy early in boot.
- If diagnosing lifecycle issues is noisy, run with preload disabled to simplify state-flow debugging.
- If simulator route tests fail intermittently, verify command method/route pair exactly matches expected factory mapping.

## Lessons Learned
What we've learned from incidents.
- Incident: Healthcheck false positives under scheduler pressure.
  - What happened: manager marked apps `ERROR` and triggered restart more often than expected.
  - What we learned: tune `healthcheckInterval` and `numOfFailedPingsBeforeRecovery` together; avoid aggressive intervals without platform scheduling headroom.
- Incident: Environment-specific socket ownership mismatch.
  - What happened: session socket setup failed in some deployments.
  - What we learned: validate socket owner/group/permission policy as part of deployment checklist, not post-failure.
- Incident: Reconnect loops after session-server crash.
  - What happened: repeated disconnect/restart churn for same app.
  - What we learned: treat restart bursts as a first-class alert and capture core dump/crash reason before increasing recovery thresholds.
- Incident: RDKEMW-19750 deactivate/inactive failure due to stale CDM session data.
  - Problem signature:
    - `MediaPipelineService::destroySession` logs `Session with id: <id> does not exists`.
    - `MediaPipelineModuleService::destroySession` logs `Destroy session failed`.
    - Host/appservice side reports failure to change app state to inactive/deactivate.
  - What happened:
    - On app transition to inactive/suspended, Rialto attempted to clear resources that should already have been released by the app lifecycle.
    - CDM bookkeeping was not fully cleared in inactive transition, leaving stale state and causing server-side failures/crash behavior during cleanup.
  - Root cause area:
    - CDM cleanup path in server media service during inactive transition.
    - Related fix was tracked in PR #541 (`Clear all CDM resources when switching to inactive state`, Jira: `RDKEMW-19750`).
  - Practical SME guidance:
    - For deactivate failures, correlate appservice state-change errors with earlier `destroySession` errors in RialtoServer logs.
    - Verify inactive transition clears all per-session and per-client CDM state, not only top-level CDM objects.
    - Confirm no stale session ids remain before subsequent teardown RPCs.
  - Validation checklist:
    - Execute Active -> Inactive -> Active cycles repeatedly for affected app(s).
    - Confirm no `Session with id ... does not exists` / `Destroy session failed` during deactivate.
    - Confirm appservice no longer reports `RialtoServer failed to deactivate` for the same flow.
    - Keep UT/CT coverage for inactive-transition cleanup regression path enabled.
- Incident: RDKEMW-19123 teardown/reconnect race during shutdown.
  - Problem statement:
    - A race condition was identified between `ServiceContext` teardown and preloaded SessionServer reconnect/restart callbacks.
    - Risk is restart/preload activity while shutdown is in progress, which can produce unstable or non-deterministic shutdown behavior.
  - Related change:
    - PR #512 (`Fix race condition with concurrent teardown of ServiceCtx and re-connection of the preloaded Session Server`).
    - PR summary indicates adding explicit shutdown gating in app-manager restart/preload paths and UT coverage for shutdown behavior.
  - Additional test notes from investigation:
    - Additional ServerManager shutdown tests exposed an issue in the initial fix approach.
    - Change was modified and shutdown scenario was then reported as OK.
    - Original issue could not be consistently reproduced with or without the change, so fix confidence remains partial pending broader validation.
    - Ticket/fix was handed off to DevQA for confirmation.
  - Practical SME guidance:
    - Treat shutdown as a special state where reconnect/restart of preloaded servers must be blocked.
    - Validate no new preloaded SessionServer is spawned once shutdown starts.
    - Correlate teardown logs with any late IPC disconnect callbacks to ensure callbacks do not restart servers during destruction.
  - Validation checklist:
    - Run repeated shutdown stress loops while preloading is enabled.
    - Inject/trigger late IPC disconnect events during shutdown and verify no restart is attempted.
    - Keep UT/CT shutdown race tests in regression suite and require DevQA sign-off for environment-level confirmation.
