# RialtoServer Healthcheck Restart Attribution Report

## Verdict
- Result: Healthcheck-Caused Restart: Confirmed
- Confidence: Medium
- Time window analyzed: 2026-05-17T06:06:30.250Z to 2026-05-17T06:06:40.378Z (UTC)
- Target serverId/appId: serverId 0 / com.comcast.viper_ipa

## Healthcheck Mechanism Evidence
- [M1] 2026-05-17T06:06:30.250Z 05-17-26-06-21AM-sky-messages.log.1 `appsserviced[9675]:  failed to ping due to 'Timed out'`
- [M2] 2026-05-17T06:06:30.250Z 05-17-26-06-21AM-sky-messages.log.1 `appsserviced[9675]:  Ping with id: 14964 failed for server: 0`
- [M3] 2026-05-17T06:06:40.251Z 05-17-26-06-21AM-sky-messages.log.1 `appsserviced[9675]:  Max num of failed pings reached for server with id: 0. Starting recovery action`

## Failure and Threshold Evidence
- [F1] 2026-05-17T06:06:35.250Z 05-17-26-06-21AM-sky-messages.log.1 `appsserviced[9675]:  Ping with id: 14965 failed for server: 0`
- [F2] 2026-05-17T06:06:40.251Z 05-17-26-06-21AM-sky-messages.log.1 `appsserviced[9675]:  Ping with id: 14966 failed for server: 0`
- [T1] 2026-05-17T06:06:40.251Z 05-17-26-06-21AM-sky-messages.log.1 `appsserviced[9675]:  Max num of failed pings reached for server with id: 0. Starting recovery action`

## Restart Path Evidence
- [R1] 2026-05-17T06:06:40.251Z 05-17-26-06-21AM-sky-messages.log.1 `appsserviced[9675]:  Max num of failed pings reached for server with id: 0. Starting recovery action`
- [R2] 2026-05-17T06:06:40.362Z 05-17-26-06-21AM-sky-messages.log.1 `RialtoServer[17698]:  MIL: SRV: < T:17698 M:main.cpp F:main L:42 > Release Tag(s): No Release Tags! (Commit ID: ae413b045d7c84841fd93a5ccb235c8a15c07889)`
- [R3] 2026-05-17T06:06:40.373Z 05-17-26-06-21AM-sky-messages.log.1 `RialtoServer[17698]:  MIL: SRV: < T:17709 M:SessionManagementServer.cpp F:initialize L:127 > Session Management Server initialized`
- [R4] 2026-05-17T06:06:40.378Z 05-17-26-06-21AM-sky-messages.log.1 `RialtoServer[17698]:  MIL: SRV: < T:17709 M:SessionServerManager.cpp F:switchToActive L:215 > RialtoServer state is ACTIVE now`

## Causal Chain
1. Failure event: Repeated ping failures for server 0 (`Ping with id: 14964/14965/14966 failed for server: 0`)
2. Threshold trigger: `Max num of failed pings reached for server with id: 0. Starting recovery action`
3. Restart request: Recovery action trigger is explicit in the same threshold line (`Starting recovery action`)
4. Restart execution: New RialtoServer process startup and transition to ACTIVE

## Competing Causes Check
- Startup-timeout path evidence: No explicit `Waitpid timeout. Killing:` line found in analyzed files/window.
- Manual/admin restart evidence: No explicit manual/admin restart trigger found in analyzed files/window.
- Other crash-path evidence: No stronger alternative cause marker found in this chain.
- Conclusion: Available evidence supports healthcheck-failure threshold as the restart trigger.

## Evidence Gaps
- Missing logs or fields: Explicit `Queue restart server handling for serverId` / `Restarting server with id` string is not present verbatim.
- Impact on confidence: Medium rather than High; causation is still supported by threshold-to-relaunch sequence.

## Final Reasoning
- Why verdict is Confirmed/Not Confirmed/Inconclusive: The required threshold marker is present for serverId 0, and it is immediately followed by RialtoServer relaunch and ACTIVE transition with no stronger competing-cause signal.
- What additional evidence would change the verdict: An explicit alternate trigger (manual/admin/startup-timeout/crash-path) preceding this relaunch could weaken or overturn this attribution.
