# Rialto Repository - Comprehensive Description

## Overview

**Rialto** is a media playback and streaming framework developed by RDK Central (Sky UK) for secure audio/video content delivery on set-top boxes and streaming devices. It provides a robust, distributed architecture for handling protected media playback, DRM-protected content, and audio/video streaming with low-level hardware integration.

## Project Information

- **Organization**: rdkcentral (RDK Central - Reference Design Kit)
- **Copyright**: 2020-2022 Sky UK
- **License**: Apache License, Version 2.0
- **Version**: 1.0.0
- **Language**: C++17
- **Build System**: CMake 3.10+

## Purpose and Use Cases

### Primary Purpose
Rialto enables applications (such as web browsers, streaming apps, and media players) to play DRM-protected video content (Netflix, YouTube, Amazon Prime, etc.) by offloading media processing to a secure, isolated server process with direct access to hardware decoders and DRM systems.

### Target Platforms
- Set-top boxes (STBs)
- Streaming devices
- RDK-based platforms (Firebolt)
- Embedded Linux systems

### Problems Solved
1. **Security Isolation**: Separates untrusted application code from secure media/DRM processing
2. **Hardware Efficiency**: Single secure server manages hardware decoders, avoiding per-application overhead
3. **DRM Compliance**: Implements EME (Encrypted Media Extensions) standards for content protection
4. **Resource Management**: Centralized control of playback sessions, video resources, and concurrent streams
5. **Cross-Platform Abstraction**: Unified API hiding platform-specific video/audio hardware

## Architecture

Rialto follows a **client-server architecture with IPC isolation**:

```
┌─────────────────────┐           ┌─────────────────────┐
│   Application       │           │  Rialto Server      │
│   (Untrusted)       │           │  (Trusted/Secure)   │
│                     │           │                     │
│  ┌──────────────┐   │           │  ┌──────────────┐   │
│  │ Rialto Client│◄──┼───IPC─────┼─►│ Media Server │   │
│  │   Library    │   │  (Unix    │  │              │   │
│  └──────────────┘   │  Sockets) │  └──────┬───────┘   │
│                     │           │         │           │
└─────────────────────┘           │         ▼           │
                                  │  ┌──────────────┐   │
                                  │  │   Hardware   │   │
                                  │  │   Decoders   │   │
                                  │  └──────────────┘   │
                                  └─────────────────────┘
```

### Key Architectural Components

#### 1. **Media Component** (`/media`)
Core audio/video playback engine with client/server split:
- **Client** (`media/client`): Client-side API implementation that communicates with server
- **Server** (`media/server`): Server-side media pipeline, hardware abstraction, and processing
- **Common** (`media/common`): Shared definitions and utilities
- **Public** (`media/public`): Public API interfaces

**Key Interfaces**:
- `IMediaPipeline`: Media playback control (play, pause, seek, stop)
- `IMediaKeys`: DRM/EME key management for protected content
- `IMediaPipelineCapabilities`: Query supported codecs and formats
- `IWebAudioPlayer`: Web Audio API support for PCM audio playback

#### 2. **Server Manager** (`/serverManager`)
Manages the lifecycle of media server processes and client applications:
- Application state management (INACTIVE → ACTIVE → NOT_RUNNING)
- Resource allocation and limits (max playback sessions per app)
- Dynamic configuration (logging levels, socket paths)
- Session server state monitoring

**Components**:
- `IServerManager`: Public API for server lifecycle control
- `IServerManagerClient`: Client-side implementation
- Server configuration and policy management

#### 3. **IPC Framework** (`/ipc`)
Custom Protobuf-based RPC system over Unix domain sockets:
- **Not using gRPC** - custom implementation with specific features:
  - File descriptor passing (for shared memory buffers)
  - Asynchronous event notifications
  - Client management (pid/uid tracking)
  - Per-client service registration

**Libraries**:
- Client-side library
- Server-side library
- Common library (shared code)

#### 4. **Protocol Definitions** (`/proto`)
Protobuf `.proto` files defining the wire protocol:
- Media pipeline messages
- Media keys/DRM messages
- Web audio player messages
- Server manager messages

#### 5. **Additional Components**
- **Logging** (`/logging`): Centralized logging infrastructure
- **Common** (`/common`): Shared utilities, constants, and interfaces
- **Stubs** (`/stubs`): Test doubles and mock implementations
- **Wrappers** (`/wrappers`): System call and library wrappers for testing
- **Scripts** (`/scripts`): Build, test, and utility scripts
- **Tests** (`/tests`): Integration and component tests

## Key Features

### 1. Media Pipeline
- **MSE Support**: Media Source Extensions for adaptive streaming
- **Session Management**: Create/destroy playback sessions
- **Multi-Source**: Attach audio, video, and subtitle sources
- **Playback Control**: Play, pause, seek, stop, set playback rate
- **Event Notifications**: Async callbacks for state changes, errors, buffering
- **Position Reporting**: Current playback position queries

### 2. DRM and Content Protection
- **EME Implementation**: Encrypted Media Extensions standard
- **License Management**: Key session creation and license handling
- **DRM System Support**: Vendor-agnostic plugin architecture
- **Capability Negotiation**: Query supported DRM systems and content types
- **Secure Decryption**: Hardware-backed decryption in isolated server

### 3. Media Capabilities
- **Codec Detection**: Query supported video/audio codecs
- **Format Support**: Check resolution, frame rate, bitrate capabilities
- **DRM Capabilities**: Verify DRM system support for content types

### 4. Web Audio Support
- **PCM Playback**: Raw audio data playback
- **Mixing**: Mix web audio with main media output
- **Volume Control**: Per-player volume management
- **State Management**: Play, pause, stop controls

### 5. Server Management
- **Application Lifecycle**: State transition management
- **Resource Limits**: Configurable max sessions per application
- **Health Monitoring**: Server state and health checks
- **Dynamic Configuration**: Runtime logging level changes

### 6. IPC Capabilities
- **Async Events**: Server-to-client event streaming
- **File Descriptor Passing**: Shared memory for video frames
- **Client Awareness**: Per-client context and permissions
- **Event Subscription**: Subscribe/unsubscribe from server events

## Technologies and Dependencies

### Core Technologies
- **C++17**: Modern C++ with standard library
- **CMake**: Build system (version 3.10+)
- **Protocol Buffers**: Message serialization (version 3.6+)
- **Unix Domain Sockets**: IPC transport layer

### Development Tools
- **Doxygen**: API documentation generation
- **CppLint**: Code style checking
- **Cppcheck**: Static analysis
- **Valgrind**: Memory leak detection (suppression file included)
- **GTest/GMock**: Unit testing framework
- **GCov/LCOV**: Code coverage

### Build Options
- Coverage builds (`COVERAGE_ENABLED`)
- Debug/Release modes (`RIALTO_BUILD_TYPE`)
- Logging level controls (FATAL, ERROR, WARN, MILESTONE, INFO, DEBUG)
- Link-time optimization (LTO)

## Building and Testing

### Build Scripts
- `build_ut.py`: Build and run unit tests
- `build_ct.py`: Build and run component tests

### Documentation
- Project webpage: https://rdkcentral.github.io/rialto/
- Coverage reports and Doxygen documentation available online

### External Documentation
- Coding Guidelines: https://wiki.rdkcentral.com/display/ASP/Rialto+Coding+Guidelines
- Building with Yocto: https://wiki.rdkcentral.com/display/ASP/Building+Rialto+using+Yocto
- Unit Tests Guide: https://wiki.rdkcentral.com/display/ASP/Unit+Tests
- Running Rialto: https://wiki.rdkcentral.com/display/ASP/Running+YouTube+with+Rialto

## Directory Structure

```
rialto/
├── media/              # Core A/V playback implementation
│   ├── client/         # Client-side API
│   ├── server/         # Server-side implementation
│   ├── common/         # Shared code
│   └── public/         # Public interfaces
├── serverManager/      # Process lifecycle management
│   ├── service/        # Server manager service
│   ├── ipc/            # Server manager IPC
│   └── public/         # Public interfaces
├── ipc/                # IPC framework
│   ├── client/         # Client IPC library
│   ├── server/         # Server IPC library
│   └── common/         # Shared IPC code
├── proto/              # Protobuf protocol definitions
├── logging/            # Logging infrastructure
├── common/             # Common utilities
├── stubs/              # Test doubles
├── wrappers/           # System wrappers
├── tests/              # Integration tests
├── scripts/            # Build and utility scripts
├── docs/               # Documentation
├── cmake/              # CMake modules
└── pkg-config/         # Package config files
```

## Use Case Example: YouTube Playback

1. **Application Launch**: User opens YouTube app
2. **Server Manager**: Spawns dedicated Rialto server process for the app
3. **Session Creation**: App creates MediaPipeline session via client API
4. **Content Setup**: App attaches video/audio sources (MSE buffers)
5. **DRM Setup**: If content is protected, creates MediaKeys session
6. **Playback**: App calls play(), server decodes and renders to hardware
7. **Events**: Server sends position updates, buffering events back to app
8. **Cleanup**: On exit, session destroyed, resources released

## Security Model

### Isolation Benefits
- **Process Separation**: App crashes don't affect media server
- **Privilege Separation**: Server runs with minimal required privileges
- **Memory Protection**: Separate address spaces prevent memory attacks
- **DRM Security**: Keys and decryption isolated from application code

### Access Control
- Client identification via Unix socket credentials (pid/uid)
- Per-client resource limits enforced by server manager
- Socket file permissions control which processes can connect

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on:
- Code style and formatting (`.clang-format`)
- Commit message templates
- Pull request process
- Testing requirements

## License

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) file for details.

## Related Projects

Rialto is part of the RDK (Reference Design Kit) ecosystem:
- **RDK Central**: Open-source platform for set-top boxes
- **Firebolt**: Application framework for streaming devices
- **Lightning**: UI framework for apps
- **RDK Video**: Video acceleration stack

## Support and Documentation

- **Project Page**: https://rdkcentral.github.io/rialto/
- **RDK Wiki**: https://wiki.rdkcentral.com/
- **GitHub Issues**: Report bugs and request features
- **Doxygen Docs**: API reference (generated from code)

---

**Last Updated**: 2026-02-21

This description was generated by analyzing the Rialto repository structure, source code, documentation, and build system. It provides a comprehensive overview of the project's purpose, architecture, features, and usage.
