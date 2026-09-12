# Rialto Client Architecture Brief

Status: Validation Complete ✅
Last Updated: 2026-08-07

## Table of Contents
- [Overview](#overview)
- [Problem Definitions and Business Context](#problem-definitions-and-business-context)
  - [Problem Statement](#problem-statement)
  - [Primary Users and Use Cases](#primary-users-and-use-cases)
  - [Non Functional Requirements](#non-functional-requirements)
  - [Integration Points](#integration-points)
- [C4 System Context Diagram](#c4-system-context-diagram)
- [System Overview](#system-overview)
  - [C4 Container Diagram](#c4-container-diagram)
  - [Container Explanation](#container-explanation)
  - [Critical User Journey Sequence](#critical-user-journey-sequence)
- [Technology Stack](#technology-stack)
- [System Data Models](#system-data-models)
- [API Endpoints](#api-endpoints)
  - [Public API](#public-api)
  - [Internal IPC API](#internal-ipc-api)
  - [Authentication and Authorization API](#authentication-and-authorization-api)
  - [AI and ML API](#ai-and-ml-api)
  - [Data Processing API](#data-processing-api)
- [Deployment Architecture](#deployment-architecture)
- [Round 1 Validation Findings](#round-1-validation-findings)
- [Round 2 Validation Findings](#round-2-validation-findings)
- [Round 3 Validation Findings](#round-3-validation-findings)
- [Validation Summary](#validation-summary)

## Overview
`media/client` implements the application-facing Rialto client runtime (`RialtoClient` shared library). It provides C++ playback, DRM, and web-audio APIs, and translates those calls into protobuf RPC over local IPC to `RialtoSessionServer`.

The module is split into three sub-components:
- `media/client/main`: API object model (`MediaPipeline`, `MediaKeys`, `WebAudioPlayer`, controller/proxy classes).
- `media/client/ipc`: IPC adapters and protobuf stubs (`*Ipc` classes and `IpcClient`).
- `media/client/common`: shared logging/support utilities.

## Problem Definitions and Business Context
### Problem Statement
Rialto client addresses these concerns:
1. Expose stable media/DRM APIs to applications without embedding server complexity.
2. Hide protobuf/IPC transport details behind synchronous C++ APIs and callback interfaces.
3. Manage client lifecycle against server application-state changes and disconnections.
4. Provide shared-memory and event coordination for low-overhead media data transfer.

### Primary Users and Use Cases
Primary users:
- Application developers integrating playback, DRM, and WebAudio.
- Platform middleware teams consuming client API surface.

Primary use cases:
1. Create media pipeline, attach sources, load/play/pause/seek, receive async playback events.
2. Create media keys, manage key sessions, handle license request/renewal and key-status events.
3. Create web audio player, write shared-buffer audio frames, receive state updates.
4. React to application-state updates and health pings from session server.

### Non Functional Requirements
Availability:
- Dedicated IPC processing thread handles channel `process/wait` loop and disconnection detection.
- Module-level reconnect path (`reattachChannelIfRequired`) attempts transport recovery.
- Connection observer propagates broken-channel state to control client path.

Performance:
- Shared memory is acquired via control IPC and used for media/web-audio buffer paths.
- Event callbacks are processed on dedicated event threads per module to avoid blocking IPC thread.
- Transport reuses shared generic IPC runtime (`RialtoIpcClient`, `RialtoIpcCommon`).

Security:
- Local-only Unix socket communication (`RIALTO_SOCKET_PATH` or `RIALTO_SOCKET_FD` source).
- No remote network listener in client module.
- Protocol compatibility checks validate client/server schema versions during registration.

Scalability:
- Multiple client-side objects can register with `ClientController`.
- Per-object IPC adapters maintain session/media-key/player handles independently.
- Event subscription tags are tracked per module for deterministic teardown.

### Integration Points
Programmatic integrations in code:
1. Root IPC transport libraries (`RialtoIpcClient`, `RialtoIpcCommon`).
2. Protobuf contracts in `proto/*.proto` (media pipeline, control, keys, capabilities, web audio).
3. Session server IPC endpoints hosted by `media/server/ipc`.
4. Common shared-memory and frame writer utilities from `RialtoPlayerCommon`/`RialtoCommon`.
5. Logging integration via `RialtoEthanLog` and client logging macros.

## C4 System Context Diagram
```mermaid
graph TD
    AppDev["👩‍💻 App Developer"]
    AppProc["📱 Application Process"]

    subgraph RialtoClientSystem ["Rialto Client Library"]
        ClientApi["📦 Public C++ APIs\nMediaPipeline MediaKeys WebAudioPlayer"]
        ClientIpc["🔌 Client IPC Adapters\nprotobuf stub wrappers"]
    end

    subgraph ExternalSystems ["Platform Runtime"]
        SessionServer["🖥️ RialtoSessionServer"]
        ServerManager["🧭 RialtoServerManager"]
        Linux["🐧 Linux Unix Socket and Shared Memory"]
    end

    AppDev -->|integrates API| AppProc
    AppProc -->|calls API methods| ClientApi
    ClientApi -->|RPC and event subscriptions| ClientIpc
    ClientIpc <-->|protobuf over unix socket| SessionServer
    SessionServer -->|state and ping events| ClientIpc
    ServerManager -->|drives session app state| SessionServer
    ClientIpc -->|RIALTO_SOCKET_PATH or RIALTO_SOCKET_FD| Linux
    ClientApi -->|mmap/read write shared memory| Linux

    classDef actor fill:#fff3e0,stroke:#ef6c00,stroke-width:2px
    classDef core fill:#e1f5fe,stroke:#0277bd,stroke-width:2px
    classDef ext fill:#e8f5e8,stroke:#2e7d32,stroke-width:2px

    class AppDev,AppProc actor
    class ClientApi,ClientIpc core
    class SessionServer,ServerManager,Linux ext
```

## System Overview
### C4 Container Diagram
```mermaid
graph TD
    subgraph AppContainer ["Application Process"]
        AppCode["App playback/DRM code"]

        subgraph RialtoClientMain ["RialtoClient SHARED lib"]
            ClientController["ClientController\nregister/unregister and app state"]
            MediaPipelineMain["MediaPipeline and MediaPipelineProxy"]
            MediaKeysMain["MediaKeys"]
            WebAudioMain["WebAudioPlayer and WebAudioPlayerProxy"]
            CapsMain["MediaPipelineCapabilities and MediaKeysCapabilities"]
        end

        subgraph RialtoClientIpc ["RialtoClientIpcImpl STATIC lib"]
            IpcClient["IpcClient\nchannel connect/reconnect thread"]
            IpcModule["IpcModule base\nreattach subscribe unsubscribe"]
            ControlIpc["ControlIpc\nregisterClient getSharedMemory ack"]
            PipelineIpc["MediaPipelineIpc"]
            KeysIpc["MediaKeysIpc"]
            CapsIpc["Capabilities IPC modules"]
            WebAudioIpc["WebAudioPlayerIpc"]
        end
    end

    subgraph Transport ["Root IPC Runtime"]
        RootClient["RialtoIpcClient"]
        RootCommon["RialtoIpcCommon"]
    end

    subgraph SessionServerContainer ["RialtoSessionServer Process"]
        ServerIpc["media/server/ipc service modules"]
    end

    AppCode --> ClientController
    AppCode --> MediaPipelineMain
    AppCode --> MediaKeysMain
    AppCode --> WebAudioMain
    AppCode --> CapsMain

    ClientController --> ControlIpc
    MediaPipelineMain --> PipelineIpc
    MediaKeysMain --> KeysIpc
    WebAudioMain --> WebAudioIpc
    CapsMain --> CapsIpc

    ControlIpc --> IpcModule
    PipelineIpc --> IpcModule
    KeysIpc --> IpcModule
    WebAudioIpc --> IpcModule
    CapsIpc --> IpcModule
    IpcModule --> IpcClient

    IpcClient --> RootClient
    RootClient --> RootCommon
    RootClient <-->|protobuf rpc/events| ServerIpc

    classDef app fill:#fff3e0,stroke:#ef6c00,stroke-width:2px
    classDef internal fill:#e1f5fe,stroke:#0277bd,stroke-width:2px
    classDef transport fill:#ede7f6,stroke:#512da8,stroke-width:2px
    classDef external fill:#e8f5e8,stroke:#2e7d32,stroke-width:2px

    class AppCode app
    class ClientController,MediaPipelineMain,MediaKeysMain,WebAudioMain,CapsMain,IpcClient,IpcModule,ControlIpc,PipelineIpc,KeysIpc,CapsIpc,WebAudioIpc internal
    class RootClient,RootCommon transport
    class ServerIpc external
```

### Container Explanation
- `RialtoClient` (`media/client/main`) is the API object layer applications instantiate.
- `ClientController` owns control registration and shared-memory lifecycle tied to app state.
- Module IPC classes (`ControlIpc`, `MediaPipelineIpc`, `MediaKeysIpc`, `WebAudioPlayerIpc`, capability IPCs) create protobuf stubs and map synchronous calls + async events.
- `IpcClient` owns channel creation (`RIALTO_SOCKET_PATH` / `RIALTO_SOCKET_FD`), IPC thread, reconnect support, and controller/closure factories.
- `IpcModule` base class centralizes attach/reattach/detach behavior and event-subscription tracking.

### Critical User Journey Sequence
```mermaid
sequenceDiagram
    participant App as Application
    participant Ctrl as ClientController and ControlIpc
    participant Pipe as MediaPipeline and MediaPipelineIpc
    participant IPC as IpcClient
    participant Server as SessionServer IPC Modules

    App->>Ctrl: registerClient
    Ctrl->>IPC: ensure channel connected
    Ctrl->>Server: ControlModule.registerClient(schemaVersion)
    Server-->>Ctrl: control_handle plus server_schema_version

    Server-->>Ctrl: ApplicationStateChangeEvent RUNNING
    Ctrl->>Server: ControlModule.getSharedMemory
    Server-->>Ctrl: shared fd and size

    App->>Pipe: create media pipeline and load/play
    Pipe->>Server: MediaPipelineModule.createSession, load, play
    Server-->>Pipe: responses

    Server-->>Pipe: PlaybackStateChangeEvent and NeedMediaDataEvent
    Pipe-->>App: notifyPlaybackState and needMediaData callbacks

    Server-->>Ctrl: PingEvent
    Ctrl->>Server: ControlModule.ack
```

## Technology Stack
Runtime and language:
- C++17 (`RialtoClient` and `RialtoClientIpcImpl` builds).
- Shared and static library composition via CMake.

IPC and serialization:
- Protocol Buffers generated stubs (`RialtoProtobuf`).
- Local Unix socket channel from root IPC library.
- Blocking closure and RPC controller abstractions for call synchronization.

Key build/runtime dependencies:
- `RialtoIpcClient`, `RialtoIpcCommon`.
- `RialtoCommon`, `RialtoPlayerCommon`.
- `RialtoEthanLog`.
- POSIX/pthread primitives (`pthread_setname_np`, mutexes, thread lifecycle).

## System Data Models
```mermaid
erDiagram
    IPC_CLIENT ||--o{ IPC_MODULE : serves
    IPC_MODULE ||--o{ EVENT_SUBSCRIPTION : owns

    CLIENT_CONTROLLER ||--o{ CONTROL_CLIENT_REGISTRATION : tracks
    CLIENT_CONTROLLER ||--|| SHARED_MEMORY_HANDLE : manages

    MEDIA_PIPELINE_INSTANCE ||--o{ MEDIA_SOURCE : owns
    MEDIA_PIPELINE_INSTANCE ||--o{ NEED_DATA_REQUEST : tracks

    MEDIA_KEYS_INSTANCE ||--o{ KEY_SESSION : owns
    WEBAUDIO_PLAYER_INSTANCE ||--|| WEBAUDIO_SHM_INFO : uses

    IPC_MODULE ||--o{ RPC_CALL : issues
    IPC_MODULE ||--o{ EVENT_CALLBACK : dispatches

    IPC_CLIENT {
        bool connected
        bool disconnecting
        string socket_source
    }

    EVENT_SUBSCRIPTION {
        int event_tag
        string event_type
    }

    CONTROL_CLIENT_REGISTRATION {
        int control_handle
        string app_state
    }

    SHARED_MEMORY_HANDLE {
        int fd
        uint32 size
        pointer shm_ptr
    }

    MEDIA_PIPELINE_INSTANCE {
        int session_id
        string state
    }

    MEDIA_SOURCE {
        int source_id
        string source_type
    }

    NEED_DATA_REQUEST {
        int request_id
        string source_status
    }

    KEY_SESSION {
        int key_session_id
        string key_system
    }

    WEBAUDIO_SHM_INFO {
        uint32 offset_main
        uint32 length_main
        uint32 offset_wrap
        uint32 length_wrap
    }

    RPC_CALL {
        string service
        string method
        bool success
    }

    EVENT_CALLBACK {
        string event_name
        string dispatch_thread
    }
```

## API Endpoints
### Public API
Rialto client is a C++ API surface, not HTTP. Main object families:

1. Playback API (`IMediaPipeline`)
- Session and source lifecycle, load/play/pause/stop/seek, rendering, buffering and telemetry controls.

2. DRM API (`IMediaKeys`)
- Media keys and key-session lifecycle, license exchanges, key-store/hash and DRM metadata queries.

3. Capabilities API
- Media pipeline capabilities and DRM capability discovery.

4. WebAudio API (`IWebAudioPlayer`)
- Player lifecycle, buffering, device info, and volume controls.

5. Control registration path
- Client registration, application-state updates, healthcheck ack, and shared-memory acquisition.

### Internal IPC API
Client module consumes these protobuf services through IPC stubs:

1. `ControlModule`
- `registerClient`, `getSharedMemory`, `ack`
- Events: `ApplicationStateChangeEvent`, `PingEvent`

2. `MediaPipelineModule`
- Session/source/playback/volume/sync/buffering RPC surface.
- Events include playback state, position/network updates, need-data, QoS, first frame, flush and playback error.

3. `MediaPipelineCapabilitiesModule`
- Mime type and property support queries.

4. `MediaKeysModule`
- Media keys and key-session operations, license and store APIs.
- Events include license request/renewal and key-status updates.

5. `MediaKeysCapabilitiesModule`
- Supported key systems, versions, cert support, robustness levels.

6. `WebAudioPlayerModule`
- Create/destroy/play/pause/eos, buffer available/delay/write, device info, volume.
- Events include web audio player state changes.

### Authentication and Authorization API
No token-based auth protocol exists in this client module.

Security model in this layer:
- Local IPC trust boundary only.
- Socket path/fd provisioning by environment and manager/server orchestration.

### AI and ML API
None. Client module has no AI/ML interface or dependency.

### Data Processing API
No standalone data-processing service is exposed.

Data processing behavior includes:
- Conversion between API domain types and protobuf requests/responses.
- Event decoding and callback dispatching on event threads.
- Shared-memory frame copy/write path for media and web audio flows.

## Deployment Architecture
```mermaid
graph TD
    subgraph AppProcess ["Application Process"]
        App["App Code"]
        ClientLib["RialtoClient shared library"]
        ClientIpcLib["RialtoClientIpcImpl static library"]
        IpcThread["rialto-ipc thread"]
        EventThreads["module event threads"]
    end

    subgraph SessionProcess ["RialtoSessionServer Process"]
        ServerIpc["Server IPC module handlers"]
    end

    subgraph HostRuntime ["Linux Host"]
        Socket["Unix socket path or fd"]
        Shm["Shared memory fd and mmap"]
    end

    App --> ClientLib --> ClientIpcLib
    ClientIpcLib --> IpcThread
    ClientIpcLib --> EventThreads

    IpcThread <-->|protobuf RPC and events| ServerIpc
    IpcThread --> Socket
    ClientLib --> Shm

    classDef proc fill:#e1f5fe,stroke:#0277bd,stroke-width:2px
    classDef host fill:#e8f5e8,stroke:#2e7d32,stroke-width:2px

    class App,ClientLib,ClientIpcLib,IpcThread,EventThreads,ServerIpc proc
    class Socket,Shm host
```

Deployment notes:
- `RialtoClient` is loaded into application process as shared library.
- Client IPC implementation runs in-process and owns dedicated communication/event threads.
- Transport channel is local host Unix socket; payload media data uses shared-memory region.

## Round 1 Validation Findings
Focus: technical alignment with client implementation.

Findings and actions:
1. Verified channel bootstrap uses environment-provided socket path/fd and fails fast when absent.
2. Verified control registration and schema compatibility flow reflects `ControlIpc::registerClient` behavior.
3. Verified shared-memory setup is tied to application state transitions in `ClientController`.

## Round 2 Validation Findings
Focus: diagram syntax and consistency.

Findings and actions:
1. Confirmed Mermaid syntax validity for context/container/sequence/ER/deployment diagrams.
2. Ensured color classes and node references are consistent.
3. Ensured no unsupported HTML edge-label markup is used.

## Round 3 Validation Findings
Focus: operational and onboarding usefulness.

Findings and actions:
1. Added explicit distinction between API-layer objects and IPC adapter layer.
2. Added reconnect and disconnection handling behavior in NFR and container explanations.
3. Added comprehensive internal IPC API section grouped by module capability.
4. Confirmed full required architecture brief section coverage.

## Validation Summary
- Mandatory three-round validation completed.
- Document is implementation-aligned with current `media/client` source and build files.
- C4, sequence, ER, and deployment diagrams included and syntax-checked.
- Brief provides onboarding-ready understanding of client architecture and runtime behavior.
