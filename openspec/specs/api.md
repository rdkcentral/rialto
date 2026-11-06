# API Spec: Rialto ServerManager

## Scope
This API spec is limited to ServerManager contracts.

- Public C++ host-facing API in `serverManager/public/include`.
- Internal manager-to-session-server protobuf API in `proto/servermanagermodule.proto`.

## Normative Sources
- `serverManager/public/include/IServerManagerService.h`
- `serverManager/public/include/IStateObserver.h`
- `serverManager/public/include/ServerManagerServiceFactory.h`
- `serverManager/public/include/LoggingLevels.h`
- `serverManager/public/include/ILogHandler.h`
- `proto/servermanagermodule.proto`

## Public C++ API

### Service Creation

#### `create(stateObserver)`
Creates a ServerManager service with default configuration.

Input:
- `stateObserver`: shared pointer to `IStateObserver`.

Output:
- `std::unique_ptr<IServerManagerService>`.

#### `create(stateObserver, config)`
Creates a ServerManager service with explicit configuration.

Input:
- `stateObserver`: shared pointer to `IStateObserver`.
- `config`: `firebolt::rialto::common::ServerManagerConfig`.

Output:
- `std::unique_ptr<IServerManagerService>`.

### Lifecycle Operations

#### `initiateApplication(appId, state, appConfig) -> bool`
Requests startup from `NOT_RUNNING` toward `INACTIVE` or `ACTIVE`.

Rules:
- `state` must not be `NOT_RUNNING`.
- Expected startup intent is to create or reuse session-server process and configure app session.

#### `changeSessionServerState(appId, state) -> bool`
Requests state transition for an existing managed application session.

Rules:
- If the app is `NOT_RUNNING`, caller should use `initiateApplication`.

#### `getAppConnectionInfo(appId) -> string`
Returns app-to-session-server connection info (socket name).

#### `setLogLevels(logLevels) -> bool`
Requests log-level update across Rialto components for managed flows.

#### `registerLogHandler(handler) -> bool`
Registers custom manager log sink implementing `ILogHandler`.

## Observer Callback API

### `IStateObserver::stateChanged(appId, state)`
Asynchronous callback used by manager to report effective application session state changes.

## Internal Protobuf API (ServerManagerModule)

Service methods:
- `setConfiguration(SetConfigurationRequest) -> SetConfigurationResponse`
- `setState(SetStateRequest) -> SetStateResponse`
- `setLogLevels(SetLogLevelsRequest) -> SetLogLevelsResponse`
- `ping(PingRequest) -> PingResponse`

Related asynchronous events consumed by manager-side logic:
- `StateChangedEvent`
- `AckEvent`

## State and Health API Semantics
- Session state enum values: `UNINITIALIZED`, `INACTIVE`, `ACTIVE`, `NOT_RUNNING`, `ERROR`.
- Healthcheck correlation is by ping id.
- Ack handling must correlate to ping id; outdated ack behavior is not equivalent to current-cycle success.

## Logging API Semantics

### Logging levels (`LoggingLevel`)
- `FATAL`
- `ERROR`
- `WARNING`
- `MILESTONE`
- `INFO`
- `DEBUG`
- `DEFAULT`
- `UNCHANGED`

### Component-level log-level bundle (`LoggingLevels`)
- `defaultLoggingLevel`
- `clientLoggingLevel`
- `sessionServerLoggingLevel`
- `ipcLoggingLevel`
- `serverManagerLoggingLevel`
- `commonLoggingLevel`

## Compatibility Notes
- This spec intentionally describes currently verified ServerManager behavior and interface shape.
- Any contract change in the listed normative sources must update this file.