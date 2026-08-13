# Data Model Spec: Rialto ServerManager

## Scope
This data model spec defines ServerManager-relevant data contracts across:
- Public C++ model types used by ServerManager API.
- Internal protobuf request/response/event messages for manager-session-server control.

## Normative Sources
- `common/public/include/SessionServerCommon.in`
- `serverManager/public/include/LoggingLevels.h`
- `proto/servermanagermodule.proto`

## Core Domain Types (C++)

### `SessionServerState` (enum)
Allowed values:
- `UNINITIALIZED`
- `INACTIVE`
- `ACTIVE`
- `NOT_RUNNING`
- `ERROR`

Usage:
- Input to lifecycle commands.
- Output in observer callbacks and internal state events.

### `AppConfig` (struct)
Fields:
- `clientIpcSocketName: string`
- `clientDisplayName: string`

Rules:
- `clientIpcSocketName` may be empty, full path, or bare name.
- Effective socket details are retrievable via `getAppConnectionInfo(appId)`.

### `SocketPermissions` (struct)
Fields:
- `ownerPermissions: unsigned`
- `groupPermissions: unsigned`
- `otherPermissions: unsigned`
- `owner: string`
- `group: string`

Permission constants:
- `kRead = 4`
- `kWrite = 2`
- `kExecute = 1`

### `ServerManagerConfig` (struct)
Fields:
- `sessionServerEnvVars: list<string>`
- `numOfPreloadedServers: unsigned`
- `sessionServerPath: string`
- `sessionServerStartupTimeout: chrono::milliseconds`
- `healthcheckInterval: chrono::seconds`
- `sessionManagementSocketPermissions: SocketPermissions`
- `numOfFailedPingsBeforeRecovery: unsigned`

Behavioral notes:
- Startup timeout value `0` disables startup timeout behavior.
- `numOfFailedPingsBeforeRecovery` controls restart threshold.

### `LoggingLevel` and `LoggingLevels`
`LoggingLevel` enum values:
- `FATAL`
- `ERROR`
- `WARNING`
- `MILESTONE`
- `INFO`
- `DEBUG`
- `DEFAULT`
- `UNCHANGED`

`LoggingLevels` fields:
- `defaultLoggingLevel`
- `clientLoggingLevel`
- `sessionServerLoggingLevel`
- `ipcLoggingLevel`
- `serverManagerLoggingLevel`
- `commonLoggingLevel`

## Protobuf Contract Types

### Enum: `SessionServerState` (proto)
Values and wire IDs:
- `UNINITIALIZED = 0`
- `INACTIVE = 1`
- `ACTIVE = 2`
- `NOT_RUNNING = 3`
- `ERROR = 4`

### Message: `LogLevels`
Optional `uint32` fields:
- `defaultLogLevels` (1)
- `clientLogLevels` (2)
- `sessionServerLogLevels` (3)
- `ipcLogLevels` (4)
- `serverManagerLogLevels` (5)
- `commonLogLevels` (6)

### Message: `Resources`
Fields:
- `maxPlaybacks: int32` (1, default `-1`)
- `maxWebAudioPlayers: int32` (2, default `-1`)

### Message: `SetConfigurationRequest`
Fields:
- `sessionManagementSocketName: string` (1)
- `initialSessionServerState: SessionServerState` (2)
- `resources: Resources` (3)
- `logLevels: LogLevels` (4)
- `socketPermissions: uint32` (5)
- `clientDisplayName: string` (6)
- `socketOwner: string` (7)
- `socketGroup: string` (8)
- `appName: string` (9)
- `sessionManagementSocketFd: int32` (10)
- `subtitleClockResyncInterval: uint32` (11)

Model constraints:
- Contract supports name-based socket and fd-based socket forms.

### Message: `SetConfigurationResponse`
- Empty payload.

### Message: `StateChangedEvent`
- `sessionServerState: SessionServerState` (1)

### Message: `SetStateRequest`
- `sessionServerState: SessionServerState` (1)

### Message: `SetStateResponse`
- Empty payload.

### Message: `PingRequest`
- `id: int32` (1, default `-1`)

### Message: `PingResponse`
- Empty payload.

### Message: `AckEvent`
- `id: int32` (1, default `-1`)
- `success: bool` (2)

### Message: `SetLogLevelsRequest`
- `logLevels: LogLevels` (1)

### Message: `SetLogLevelsResponse`
- Empty payload.

## Relationship Model
- One ServerManager instance manages multiple session-server instances.
- One application maps to at most one active managed session-server instance at a time.
- Healthcheck bookkeeping is maintained per managed server with ping-id correlation.

## Invariants
- State values must come from `SessionServerState` only.
- Ping acknowledgement semantics are tied to unique ping IDs.
- Data model changes in normative sources require synchronized update of this spec.