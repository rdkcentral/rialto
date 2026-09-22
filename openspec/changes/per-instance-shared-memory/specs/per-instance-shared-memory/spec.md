## ADDED Requirements

### Requirement: Per-media-pipeline allocation
The system SHALL allocate one independent shared-memory object for each media pipeline during session creation and SHALL NOT share that object with another media pipeline or WebAudio player.

#### Scenario: Media pipeline creation succeeds
- **WHEN** the server admits a media-pipeline creation request
- **THEN** it creates and maps a new shared-memory object before publishing the session
- **THEN** the object contains a 7 MiB video region, a 1 MiB audio region, and a 256 KiB subtitle region
- **THEN** the created server pipeline retains ownership of that object

#### Scenario: Two media pipelines coexist
- **WHEN** two media pipelines are created concurrently or sequentially
- **THEN** each pipeline receives a different shared-memory object
- **THEN** writes and destruction for either pipeline do not change the other pipeline's mapping, offsets, or contents

#### Scenario: Media-pipeline allocation fails
- **WHEN** descriptor creation, resize, sealing, or server mapping fails
- **THEN** media-pipeline creation fails
- **THEN** no session is published or retained
- **THEN** every temporary descriptor and mapping is released

### Requirement: Per-WebAudio-player allocation
The system SHALL allocate one independent 10 KiB shared-memory object for each WebAudio player during player creation and SHALL NOT share that object with another player or media pipeline.

#### Scenario: WebAudio player creation succeeds
- **WHEN** the server admits a WebAudio-player creation request
- **THEN** it creates and maps a new 10 KiB shared-memory object before publishing the player handle
- **THEN** the created server player retains ownership of that object

#### Scenario: Multiple WebAudio players coexist
- **WHEN** multiple WebAudio players exist
- **THEN** each player has a distinct descriptor and mapping
- **THEN** ring wrap offsets are relative to that player's mapping

#### Scenario: WebAudio allocation fails
- **WHEN** shared-memory allocation or mapping fails during WebAudio-player creation
- **THEN** player creation fails without publishing or retaining a player handle
- **THEN** every temporary resource is released

### Requirement: Atomic descriptor transfer during creation
The system MUST transfer the owning instance's shared-memory descriptor and mapping size in the successful creation response using IPC file-descriptor transport.

#### Scenario: Media-pipeline creation response
- **WHEN** media-pipeline creation succeeds
- **THEN** `CreateSessionResponse` contains the session ID, a transferred descriptor, and a non-zero mapping size
- **THEN** the descriptor refers only to that media pipeline's object

#### Scenario: WebAudio-player creation response
- **WHEN** WebAudio-player creation succeeds
- **THEN** `CreateWebAudioPlayerResponse` contains the player handle, a transferred descriptor, and a non-zero mapping size
- **THEN** the descriptor refers only to that WebAudio player's object

#### Scenario: Response transport is invalid
- **WHEN** a successful creation response lacks a valid received descriptor or non-zero representable size
- **THEN** client instance creation fails
- **THEN** the received descriptor, if any, is closed exactly once
- **THEN** the corresponding server instance is destroyed

### Requirement: Instance-owned client mappings
Each client media pipeline and WebAudio player MUST own and use its own mapped shared-memory handle for its complete usable lifetime.

#### Scenario: Media data is written
- **WHEN** a media pipeline handles a valid need-data request
- **THEN** its frame writer uses that pipeline's mapping
- **THEN** all `MediaPlayerShmInfo` offsets are interpreted relative to the beginning of that mapping

#### Scenario: WebAudio data is written
- **WHEN** a WebAudio player writes frames
- **THEN** main and wrap offsets are interpreted relative to that player's mapping
- **THEN** no application-global shared-memory handle is consulted

#### Scenario: Owning instance is destroyed
- **WHEN** a client media pipeline or WebAudio player is destroyed
- **THEN** its mapping is unmapped and its received descriptor is closed after active users release it
- **THEN** other instance mappings remain valid

#### Scenario: Server instance is recreated
- **WHEN** lifecycle recovery recreates an instance
- **THEN** the client clears requests that reference the old mapping
- **THEN** it releases the old handle
- **THEN** it maps and installs the newly returned descriptor before accepting new data requests

#### Scenario: Server loss invalidates a client instance
- **WHEN** the client observes `INACTIVE` or `UNKNOWN` because the server instance is no longer valid
- **THEN** it blocks new writes
- **THEN** it clears pending users before releasing the instance mapping and descriptor
- **THEN** subsequent writes fail until a fresh descriptor is installed

### Requirement: Direct instance-relative region addressing
The server SHALL address transport regions directly within the owning instance's object and SHALL NOT reserve or resolve global partitions by session ID or WebAudio handle.

#### Scenario: Media region information is generated
- **WHEN** the server generates shared-memory information for an audio, video, or subtitle request
- **THEN** it obtains capacity and offsets directly from that pipeline's layout
- **THEN** the offset and length remain within that pipeline's mapping

#### Scenario: Media data is requested
- **WHEN** the server prepares an immediate or delayed need-data notification for a source
- **THEN** it clears the complete pipeline-local region for that source type before notifying the client
- **THEN** the clearing behavior matches the shipping implementation

#### Scenario: Pipeline or player is destroyed
- **WHEN** an instance is destroyed
- **THEN** no partition-unmap operation is required
- **THEN** releasing the instance's buffer owner releases the entire object after outstanding references are gone

### Requirement: No eager application-global playback allocation
The system SHALL NOT allocate playback transport shared memory merely because the session server enters the active state.

#### Scenario: Server becomes active without instances
- **WHEN** the session server enters the active state and no media pipeline or WebAudio player exists
- **THEN** no playback transport memfd is allocated

#### Scenario: Server becomes inactive
- **WHEN** the session server becomes inactive
- **THEN** it destroys all media pipelines and WebAudio players
- **THEN** each per-instance mapping and descriptor is released through normal instance ownership
- **THEN** no application-global playback buffer remains to reset

### Requirement: Admission limits remain independent of allocation layout
The system SHALL continue enforcing maximum media-pipeline and WebAudio-player counts while using those limits only for admission and not for sizing a shared object.

#### Scenario: Capacity is available
- **WHEN** an instance count is below its configured maximum
- **THEN** the server may allocate exactly one object for the newly admitted instance

#### Scenario: Maximum instance count is reached
- **WHEN** an instance creation request would exceed its configured maximum
- **THEN** creation fails before shared-memory allocation
- **THEN** existing instances remain unchanged

### Requirement: Lockstep protocol compatibility
The system MUST require a client and server that both implement per-instance creation-time descriptor transfer and MUST NOT provide a global-buffer compatibility fallback.

#### Scenario: Compatible peers connect
- **WHEN** both peers advertise the protocol version containing per-instance shared memory
- **THEN** creation RPCs use their instance descriptor and size fields
- **THEN** `ControlModule.getSharedMemory` is not used for playback transport

#### Scenario: Incompatible peer connects
- **WHEN** either peer does not support mandatory per-instance descriptor transfer
- **THEN** schema compatibility negotiation rejects the incompatible connection or instance creation
- **THEN** the server does not allocate a transitional application-global playback buffer

### Requirement: Bounds and isolation enforcement
The system MUST validate every mapping size and transport offset so an instance cannot access beyond its own mapped object.

#### Scenario: Valid media request bounds
- **WHEN** the server emits media shared-memory information
- **THEN** metadata and media ranges fit completely inside the matching source-type region and pipeline mapping

#### Scenario: Invalid client write bounds
- **WHEN** a requested media or WebAudio write would exceed the owning mapping or advertised range
- **THEN** the client rejects the write
- **THEN** no bytes are written to shared memory

#### Scenario: One instance is destroyed during another's use
- **WHEN** one media pipeline or WebAudio player is destroyed while another is writing
- **THEN** only the destroyed instance's resources are released
- **THEN** the active instance continues using its distinct mapping

### Requirement: Abrupt process-loss cleanup
The system MUST release per-instance shared-memory resources after consumer-process death or server-process loss without relying on graceful instance destruction by the failed process. The consumer application process hosts the Rialto client, while RialtoServer is a separate process spawned and monitored by RialtoServerManager.

#### Scenario: Consumer application crashes
- **WHEN** the application process hosting the Rialto client exits without sending instance-destroy RPCs
- **THEN** the kernel closes its IPC socket, descriptor duplicates, and mappings
- **THEN** the separately running RialtoServer observes the disconnect and destroys every instance associated with that client
- **THEN** the remaining server mappings and descriptors are released
- **THEN** the configured instance capacity is available to a replacement application process

#### Scenario: RialtoServer crashes while the application survives
- **WHEN** the application process observes loss of the ServerManager-spawned RialtoServer connection
- **THEN** every affected client instance clears pending mapping users
- **THEN** it releases its descriptor and mapping even though its duplicate could keep the memfd alive
- **THEN** it rejects data writes until a newly created instance supplies a fresh descriptor

#### Scenario: RialtoServer restarts
- **WHEN** RialtoServerManager replaces a failed RialtoServer while the application process remains alive
- **THEN** old session IDs, player handles, mappings, and pending requests remain invalid
- **THEN** each replacement instance receives a fresh descriptor from the new server generation
- **THEN** data writes resume only after the fresh mapping is installed

#### Scenario: Both processes terminate
- **WHEN** the application process and RialtoServer process terminate
- **THEN** kernel descriptor cleanup releases every anonymous memfd
- **THEN** no persistent named shared-memory object remains
- **THEN** RialtoServerManager can start a replacement server that admits new instances normally
