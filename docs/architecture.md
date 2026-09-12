# Rialto System Architecture

Status: Validation Complete ✅
Last Updated: 2026-08-07

## Table of Contents
- [Overview](#overview)
- [How To Use This Architecture](#how-to-use-this-architecture)
- [Architecture Partitioning](#architecture-partitioning)
- [Module Architecture Briefs](#module-architecture-briefs)
- [C4 System Context](#c4-system-context)
- [C4 Container View](#c4-container-view)
- [Primary Runtime Flows](#primary-runtime-flows)
  - [Flow 1: Application Playback Lifecycle](#flow-1-application-playback-lifecycle)
  - [Flow 2: ServerManager Session Lifecycle and Healthcheck](#flow-2-servermanager-session-lifecycle-and-healthcheck)
  - [Flow 3: Shared Memory Data Path](#flow-3-shared-memory-data-path)
- [Cross-Cutting Design Rules](#cross-cutting-design-rules)
- [Configuration and Environment](#configuration-and-environment)
- [Technology Stack Summary](#technology-stack-summary)
- [Validation Summary](#validation-summary)

## Overview
This document reframes Rialto architecture as a system map built from module-level architecture briefs.

Instead of duplicating each subsystem internals, this page answers:
1. Which module owns what responsibilities.
2. How client, session server, and server manager interact end-to-end.
3. Which detailed brief to read next for each concern.

## How To Use This Architecture
Suggested reading path:
1. Read this file for system boundaries and runtime interactions.
2. Open the module brief that matches your current task.
3. Use module docs for detailed APIs, data models, and validation notes.

## Architecture Partitioning
Rialto is partitioned into the following main areas:

1. Server
- `media/server/*` contains the session server runtime and media/DRM execution logic.

2. Client
- `media/client/*` contains client-side APIs and adapters used by applications.

3. Server manager
- `serverManager/*` controls session lifecycle, health checks, and runtime orchestration.

4. IPC
- `ipc/*` provides generic local transport primitives used by the platform.
- `media/client/ipc`, `media/server/ipc`, and `serverManager/ipc` provide domain-specific IPC adapters.

5. Public API
- `rialto/media/public/include` defines the public Rialto C++ API surface.

6. Common
- `common/*` contains shared functions and structures used across the project.

## Module Architecture Briefs
Primary architecture references:
- Client architecture: `../media/client/architecture.md`
- Session server IPC architecture: `../media/server/ipc/architecture.md`
- Generic IPC transport architecture (root IPC module): `../ipc/architecture.md`
- IPC server transport deep dive: `../ipc/server/architecture.md`
- Server manager architecture: `../serverManager/architecture-brief.md`
- Logging architecture: `../logging/architecture.md`

These briefs are intentionally complementary:
- `docs/architecture.md` defines cross-module views.
- Module briefs define component internals and APIs.

## C4 System Context
```mermaid
graph TD
    App["Application"]

    subgraph Rialto ["Rialto System"]
        Client["Rialto Client API and IPC"]
        SessionServer["Rialto Session Server"]
        GenericIpc["Generic IPC Transport"]
    end

    subgraph ExternalOrchestration ["External Orchestration"]
        ServerManager["Rialto ServerManager\n(shared library in separate systemd process)"]
    end

    subgraph Platform ["Platform Dependencies"]
        Linux["Linux Unix socket and shared memory"]
        Gst["GStreamer"]
        Cdm["OpenCDM"]
    end

    App -->|C++ API calls| Client
    Client <-->|protobuf RPC and events| SessionServer
    ServerManager <-->|ServerManagerModule RPC/events| SessionServer
    Client --> GenericIpc
    SessionServer --> GenericIpc
    ServerManager --> GenericIpc
    Client --> Linux
    SessionServer --> Linux
    SessionServer --> Gst
    SessionServer --> Cdm

    classDef app fill:#fff3e0,stroke:#ef6c00,stroke-width:2px
    classDef core fill:#e1f5fe,stroke:#0277bd,stroke-width:2px
    classDef ext fill:#e8f5e8,stroke:#2e7d32,stroke-width:2px
    classDef orchestrator fill:#fffde7,stroke:#f9a825,stroke-width:2px

    class App app
    class Client,SessionServer,GenericIpc core
    class ServerManager orchestrator
    class Linux,Gst,Cdm ext
```

## C4 Container View
```mermaid
graph TD
    subgraph AppProcess ["Application Process"]
        AppCode["App Code"]
        ClientMain["RialtoClient\nmedia/client/main"]
        ClientIpc["media/client/ipc"]
    end

    subgraph SessionProcess ["Session Server Process"]
        ServerIpc["media/server/ipc"]
        ServerMain["media/server/main and service"]
        GstPlayer["GstGenericPlayer\nmedia/server/gstplayer"]
    end

    subgraph ManagerProcess ["RialtoServerManagerProcess"]
        SmService["serverManager/service and common"]
        SmIpc["serverManager/ipc"]
    end

    subgraph Transport ["Shared IPC Libraries"]
        IpcClient["ipc/client"]
        IpcServer["ipc/server"]
        IpcCommon["ipc/common"]
    end

    AppCode --> ClientMain
    ClientMain --> ClientIpc
    ClientIpc --> IpcClient
    IpcClient --> IpcCommon
    IpcCommon --> IpcServer
    IpcServer --> ServerIpc
    ServerIpc --> ServerMain
    ServerMain --> GstPlayer

    SmService --> SmIpc
    SmIpc --> IpcClient
    SmIpc --> IpcCommon
    SmIpc --> IpcServer
    SmIpc --> ServerIpc

    classDef proc fill:#e1f5fe,stroke:#0277bd,stroke-width:2px
    classDef transport fill:#ede7f6,stroke:#512da8,stroke-width:2px

    class AppCode,ClientMain,ClientIpc,ServerIpc,ServerMain,GstPlayer,SmService,SmIpc proc
    class IpcClient,IpcServer,IpcCommon transport
```

## Primary Runtime Flows
### Flow 1: Application Playback Lifecycle
```mermaid
sequenceDiagram
    participant App as Application
    participant Client as media/client
    participant Session as media/server/ipc
    participant Core as media/server/main

    App->>Client: register client
    Client->>Session: ControlModule.registerClient
    Session-->>Client: control_handle and app state

    App->>Client: createMediaPipeline
    Client->>Session: MediaPipelineModule.createMediaPipeline
    Session-->>Client: media pipeline handle

    App->>Client: load/play/pause/seek
    Client->>Session: MediaPipelineModule RPC
    Session->>Core: playback service calls
    Core-->>Session: state and data callbacks
    Session-->>Client: playback events
    Client-->>App: app callbacks
```

### Flow 2: ServerManager Session Lifecycle and Healthcheck
```mermaid
sequenceDiagram
    participant SM as Server Manager
    participant SessionServer as RialtoSessionServer

    SM->>SessionServer: ServerManagerModule.setConfiguration
    SessionServer-->>SM: StateChangedEvent(UNINITIALIZED -> INACTIVE)

    SM->>SessionServer: ServerManagerModule.setState(ACTIVE or INACTIVE)
    SessionServer-->>SM: StateChangedEvent(target state)

    SM->>SessionServer: ServerManagerModule.ping(id)
    SessionServer-->>SM: AckEvent(id)
```

### Flow 3: Shared Memory Data Path (Media Pipeline)
```mermaid
sequenceDiagram
    participant Client as media/client
    participant Session as media/server/ipc
    participant Linux as shm and fd layer

    Client->>Session: ControlModule.getSharedMemory
    Session-->>Client: fd and size
    Client->>Linux: map shared memory region

    Session-->>Client: NeedMediaDataEvent
    Client->>Linux: write media bytes
    Client->>Session: MediaPipelineModule.haveData
```

WebAudio uses a different SHM flow (push mode): no `NeedMediaDataEvent` round-trip. The app writes PCM data into the WebAudio ring buffer and notifies the server using `WebAudioPlayerModule.writeBuffer`.

## Cross-Cutting Design Rules
1. Layered separation
- Domain logic should not depend on transport internals.

2. Local-only communication
- All runtime RPC is local IPC over Unix socket transport.

3. Explicit ownership and cleanup
- Session/key/player resources are tracked per client and cleaned up on disconnect.

4. Schema compatibility
- Client/server schema compatibility is validated during client registration.

5. Event-driven runtime
- Async events are processed through dedicated event threads and callbacks.

## Configuration and Environment
Key runtime configuration inputs:
1. Session manager configuration file (JSON):
- Contains application/service limits, socket settings, ping/recovery controls, and logging levels.

2. Client connection environment:
- `RIALTO_SOCKET_PATH` or `RIALTO_SOCKET_FD` for control/media IPC attachment.

3. Session socket permissions:
- Optional owner/group/permission settings configured for runtime IPC sockets.

## Technology Stack Summary
- Language: C++17
- Build: CMake
- IPC serialization: Protocol Buffers
- Local transport: Unix domain sockets with fd passing
- Media runtime: GStreamer
- DRM integration: OpenCDM

## Validation Summary
This reframed document was validated for:
1. Boundary clarity between client, server IPC, server runtime, and server manager.
2. Consistency with module-level architecture briefs created in this repository.
3. Mermaid syntax correctness for context, container, and sequence diagrams.
4. Practical onboarding value: clear "where to look" map before module deep dives.
