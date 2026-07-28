# Architecture Spec: Rialto ServerManager

## Scope
This spec defines the approved architecture baseline for Rialto ServerManager only.

- In scope: `serverManager/public`, `serverManager/service`, `serverManager/common`, `serverManager/ipc`, `serverManager/serverManagerSim`, and `proto/servermanagermodule.proto`.
- Out of scope: full-platform architecture details outside ServerManager.

## Source of Truth
- Approved architectural source: `serverManager/architecture-brief.md`.
- Operational incident context: `serverManager/SME-notes.md`.
- Repository-wide architecture docs (for context only, not normative for this spec): `docs/architecture-brief.md`, `logging/architecture.md`.

## Purpose
ServerManager orchestrates `RialtoSessionServer` lifecycle per application and provides deterministic session-state transitions, local IPC control, healthcheck monitoring, and recovery behavior.

## Primary Responsibilities
- Spawn and configure session-server processes.
- Drive app/session lifecycle transitions.
- Manage ping/ack healthcheck and recovery thresholds.
- Propagate state changes to host integrators through observer callbacks.
- Provide simulator-backed control paths for integration testing.

## Architectural Building Blocks

### Public API Layer
- Entry interface: `IServerManagerService`.
- Observer interface: `IStateObserver` with `stateChanged(appId, state)`.
- Factory creation path used by host process.

### Service Layer
- `ServerManagerService` and `ServiceContext` coordinate initialization and orchestration wiring.
- Config resolution is centralized via `ConfigHelper` (including optional config-file precedence when enabled).

### Common Orchestration Layer
- `SessionServerAppManager` serializes lifecycle events on a single manager thread model.
- `SessionServerApp` owns per-app process/socket/session mappings.
- `HealthcheckService` tracks ping lifecycle and failed ping history by ping IDs.

### IPC Layer
- `Controller` manages per-server client registry.
- IPC client and loop exchange protobuf RPC/events with each session server.
- Contract service: `ServerManagerModule` (`setConfiguration`, `setState`, `setLogLevels`, `ping`).

### Simulator Layer
- `RialtoServerManagerSim` exposes HTTP test endpoints on port `9008`:
  - `POST /SetState/<AppName>/<NewState>`
  - `GET /GetState/<AppName>`
  - `GET /GetAppInfo/<AppName>`
  - `POST /SetLog/<component>/<level>`
  - `POST /Quit`

## State Model
Canonical session-server states:
- `UNINITIALIZED`
- `INACTIVE`
- `ACTIVE`
- `NOT_RUNNING`
- `ERROR`

Expected lifecycle includes cold start and preload paths, plus runtime transitions:
- `NOT_RUNNING -> UNINITIALIZED`
- `NOT_RUNNING -> INACTIVE`
- `UNINITIALIZED -> INACTIVE`
- `INACTIVE -> ACTIVE`
- `ACTIVE -> INACTIVE`
- `INACTIVE -> NOT_RUNNING`
- `ACTIVE -> NOT_RUNNING`

## Healthcheck and Recovery Contract
- Healthcheck runs periodically using a single manager timer.
- Ack matching is strict by ping ID.
- Timeouts/failures move assigned apps toward `ERROR` and update failure history.
- Recovery is triggered at `numOfFailedPingsBeforeRecovery` threshold.
- Outdated successful acks are treated as stale timing signals and can remove corresponding failed-ping records without being treated as current-cycle success.

## Integration Contracts
- Host controls lifecycle through public API methods (`initiateApplication`, `changeSessionServerState`, `getAppConnectionInfo`, `setLogLevels`).
- Manager-to-session-server control is local protobuf over Unix socketpair/fd channels.
- App connection details are delivered via `getAppConnectionInfo(appId)`.

## Non-Functional Expectations
- Deterministic event ordering via serialized manager orchestration.
- Local-only communication boundaries for control plane.
- Failure containment with restart-based recovery.
- Configurable socket permission/ownership policy for deployment environments.

## Constraints and Notes
- This document intentionally avoids asserting non-ServerManager architecture as normative.
- If broader architecture docs conflict with implementation, ServerManager implementation and `serverManager/architecture-brief.md` take precedence for this spec.