# Spec: Suspended State Flow for ServerManager

## Status
Drafted from compare analysis of master...origin/SuspendPoc.

## Scope
This spec defines the suspended-state behavior for Rialto ServerManager.

In scope:
- ServerManager public API and service forwarding.
- SessionServerAppManager suspend and resurrection flow.
- SessionServerApp suspend bookkeeping.
- ServerManagerSim suspend command surface.

Out of scope:
- New session state enum values in shared public/proto state models.
- Media pipeline semantic changes unrelated to ServerManager suspend orchestration.

## Source of Truth
Primary approved architecture baseline:
- serverManager/architecture-brief.md

Change source analyzed:
- master...origin/SuspendPoc compare in rdkcentral/rialto.

Code contract sources:
- serverManager/public/include/IServerManagerService.h
- serverManager/service/include/ServerManagerService.h
- serverManager/service/source/ServerManagerService.cpp
- serverManager/common/include/ISessionServerAppManager.h
- serverManager/common/source/ISessionServerApp.h
- serverManager/common/source/SessionServerApp.h
- serverManager/common/source/SessionServerApp.cpp
- serverManager/common/source/SessionServerAppManager.h
- serverManager/common/source/SessionServerAppManager.cpp
- serverManager/serverManagerSim/commands/Suspend.h
- serverManager/serverManagerSim/commands/Suspend.cpp
- serverManager/serverManagerSim/commands/CommandFactory.cpp
- serverManager/serverManagerSim/TestService.h
- serverManager/serverManagerSim/TestService.cpp

## Problem Statement
ServerManager has existing transitions for ACTIVE, INACTIVE, and NOT_RUNNING. The change introduces a first-class suspend operation that intentionally tears down a running session server and resurrects it in INACTIVE state while preserving app identity and connection context.

## Goals
- Provide explicit suspend API for an already-launched app.
- Trigger server transition to NOT_RUNNING as suspend mechanism.
- Automatically resurrect suspended app into INACTIVE.
- Preserve app connection semantics across resurrection.
- Expose suspend operation through ServerManagerSim HTTP command.

## Non-Goals
- Introduce a new SUSPENDED enum state in SessionServerState.
- Change protobuf ServerManagerModule service methods for suspend.
- Change host observer callback interface shape.

## External API Contract

### New host-facing API
Method added to IServerManagerService:
- suspendSessionServer(appId) -> bool

Intent:
- Request suspension of a launched app session server.

Service forwarding:
- ServerManagerService forwards this method to SessionServerAppManager suspendSessionServer.

## Internal Orchestration Contract

### ISessionServerAppManager additions
- suspendSessionServer(appId) -> bool

### ISessionServerApp additions
- setSuspendOngoing()
- isSuspendOngoing() const

### SessionServerApp data model extension
- suspendOngoing flag, initialized false.

## Suspend Flow Semantics

### 1. Request handling
When suspendSessionServer(appId) is called:
1. Request is serialized on manager event thread.
2. Manager resolves app to active session server instance.
3. If no session server exists for appId, operation fails.

### 2. Suspend trigger
If app session exists:
1. Manager marks session with suspendOngoing=true.
2. Manager requests performSetState(serverId, NOT_RUNNING) via IPC controller.
3. If performSetState fails, failure handler is invoked for NOT_RUNNING path.

### 3. NOT_RUNNING event handling for suspend
On subsequent state change to NOT_RUNNING:
1. If suspendOngoing is true and expectedState is not NOT_RUNNING, manager treats this as suspend completion.
2. Manager starts resurrection flow instead of normal removal-only path.

### 4. Resurrection flow
Manager resurrects suspended server as follows:
1. Capture appName from old server instance.
2. Build initial target state INACTIVE.
3. Build app config from previous:
   - sessionManagementSocketName
   - clientDisplayName
4. Move old named socket handle using releaseNamedSocket().
5. Remove old IPC client and healthcheck bookkeeping.
6. Erase old session server object from manager set.
7. Create new session server app with preserved identity/config/socket and launch.
8. Connect new server through controller.

### 5. Normal NOT_RUNNING path unchanged for non-suspend cases
If suspend criteria are not met:
1. Remove client.
2. Remove healthcheck tracking.
3. Erase session server entry.

## State Model Implications
- No new enum state is introduced.
- Suspend is represented as an operation, not a persisted lifecycle state.
- Observable state churn is expected through existing callbacks, including NOT_RUNNING and subsequent startup states of resurrected server.

## ServerManagerSim Contract

### New command
HTTP endpoint:
- POST /Suspend/AppName

Behavior:
- Validates one path parameter.
- Calls TestService suspendSessionServer(appName).
- Returns success/failure text response.

CLI help text in simulator startup banner is extended with Suspend examples.

## Compatibility and IPC
- No protobuf service method additions are required.
- Suspend uses existing setState(NOT_RUNNING) transport path.
- Existing state observer callback contract remains unchanged.

## Error Handling Expectations
- Suspend request for unknown app must fail.
- Failure to send NOT_RUNNING command triggers existing NOT_RUNNING failure handling path.
- Failure to resurrect should be logged and leave app unavailable until explicit re-initiation.

## Test-Backed Acceptance Criteria
Derived from SuspendPoc unit-test deltas:
- SessionServerAppManager returns false when suspending non-launched app.
- SessionServerAppManager marks suspend ongoing and initiates suspend flow for launched app.
- SessionServerApp exposes suspend flag mutation/read behavior.
- ServerManagerService returns true/false according to app-manager suspend result.
- Existing NOT_RUNNING removal paths stay non-suspend by default (tests explicitly set non-suspend expectations).

## Open Questions and Follow-ups
- Return-value semantics on IPC send failure in suspend path should be clarified for strict API success/failure meaning.
- Observer event sequence during suspend-resurrect flow should be documented explicitly for integrators consuming stateChanged callbacks.
- Recovery behavior when resurrection launch fails may need explicit host notification contract.