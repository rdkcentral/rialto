## Context

The shipping server creates one `SharedMemoryBuffer` during `PlaybackService::switchToActive()`. Its size is calculated from configured maximum media-pipeline and WebAudio counts. Generic partitions contain fixed video, audio, and subtitle regions; WebAudio partitions contain a fixed ring region. Server instances claim partitions by session ID or player handle. The control service transfers the single descriptor and total size to `ClientController`, which maps it once and exposes the mapping to all client media pipelines and WebAudio players.

This design couples unrelated instance lifetimes, reserves maximum configured capacity before an instance exists, requires global partition bookkeeping, and makes `ControlModule.getSharedMemory` an application-level prerequisite for instance data transport.

The replacement uses one memfd and one client mapping per transport-producing instance. The client and server deploy in lockstep, so the new creation responses are mandatory and there is no global-buffer fallback.

In the deployed topology, the Rialto client library runs inside the consumer application's process and container. RialtoServer is a separate process spawned, monitored, and restarted by RialtoServerManager. Shared-memory crash and restart behavior is therefore validated across those real process boundaries; WPEFramework is not part of this transport lifecycle.

## Goals / Non-Goals

**Goals:**
- Allocate shared memory only when a media pipeline or WebAudio player is successfully created.
- Give every media pipeline and WebAudio player an independent shared-memory object.
- Transfer the descriptor and mapping size atomically with instance creation.
- Make shared-memory lifetime match the owning instance lifetime on both client and server.
- Preserve current media payload formats, request metadata, and fixed region capacities.
- Remove session-ID and handle-based partition reservation.
- Fail creation without publishing a usable instance when allocation, server mapping, descriptor transfer, or client mapping fails.
- Cover allocation isolation, cleanup, and failure behavior with unit and component tests.

**Non-Goals:**
- Dynamically size a media buffer from attached source types.
- Resize a buffer after instance creation.
- Change `MediaPlayerShmInfo`, `WebAudioShmInfo`, media metadata serialization, or frame-writer payload format.
- Support mixed old/new Rialto client and server versions.
- Share a transport object between instances.
- Change public application-facing media-pipeline or WebAudio APIs.

## Decisions

### Allocate an independent object during each creation RPC

`MediaPipelineService::createSession` will allocate a media-pipeline buffer before constructing and publishing `MediaPipelineServerInternal`. `WebAudioPlayerService::createWebAudioPlayer` will do the equivalent for a WebAudio buffer. The created server instance retains a shared owner for its buffer.

Creation succeeds only after allocation, server mapping, instance construction, and response preparation succeed. Failure unwinds all temporary resources and does not insert the instance into service maps or client-session tracking.

Alternatives considered:
- Lazily allocate on first media request: rejected because it delays allocation failure until playback and complicates descriptor delivery.
- Allocate on source attachment: rejected because source attachment is incremental and would require resizing or replacement rules.
- Keep one lazily created global object: rejected because it retains cross-instance coupling and maximum-capacity allocation.

### Use role-specific shared-memory abstractions

The global `ISharedMemoryBuffer` partition model will be replaced or split into role-specific abstractions. A media-pipeline buffer exposes direct audio, video, and subtitle region access without `MediaPlaybackType` or session ID. A WebAudio buffer exposes its ring region without a player handle.

The initial layouts remain:

```text
Media pipeline mapping (8.25 MiB)
  video:    offset 0,                  length 7 MiB
  audio:    offset 7 MiB,              length 1 MiB
  subtitle: offset 8 MiB,              length 256 KiB

WebAudio mapping (10 KiB)
  ring:     offset 0,                  length 10 KiB
```

All offsets sent to a client are relative to the beginning of that instance's mapping. Existing metadata reservation within a media-type region remains unchanged. Newly extended memfd pages remain kernel-zeroed without an eager constructor `memset`. The server preserves shipping behavior by clearing the complete pipeline-local source region before every immediate or delayed need-data notification; optimizing those repeated clears is outside this ownership change.

Alternatives considered:
- Construct the existing global class with counts `(1, 0)` or `(0, 1)`: acceptable as a short implementation step, but rejected as the target design because its partition API preserves obsolete concepts.
- Negotiate region sizes at creation: deferred because current source requirements are not known at pipeline creation and fixed sizing isolates this ownership change.

### Transfer descriptors in creation responses

`CreateSessionResponse` will add mandatory-for-this-version `shm_fd` and `shm_size` fields. `CreateWebAudioPlayerResponse` will add equivalent fields. Descriptor fields use `(rialto.ipc.field_is_fd) = true` so the IPC transport passes duplicated descriptors with Unix ancillary data.

The numeric protobuf value is not an independently reusable descriptor. The receiving IPC layer owns the received descriptor until it transfers ownership to `SharedMemoryHandle`; all failure paths close it exactly once.

A successful response must contain a non-negative descriptor and non-zero size. Missing or invalid transport fields make client instance creation fail, and the client sends or triggers instance destruction when necessary to release a server instance that was created before local mapping failed.

Alternatives considered:
- Keep `ControlModule.getSharedMemory`: rejected because the control service cannot identify an instance-specific object.
- Add a second post-creation get-buffer RPC: rejected because it introduces a partially initialized state and an extra round trip.

### Make the client instance own its mapping

`MediaPipeline` will retain its own `ISharedMemoryHandle` and use it when creating frame writers. `WebAudioPlayer` will retain its own handle and use it for ring writes. `ClientController` will no longer initialize, expose, or terminate playback shared memory during application-state changes.

Pending media requests must be cleared before replacing or releasing a mapping. On `INACTIVE` or `UNKNOWN`, each instance blocks new writes, clears pending users, releases its mapping and descriptor, and remains unusable until a fresh descriptor is installed. A handle cannot be reused by a recreated server instance. If lifecycle behavior recreates an instance, the recreation result supplies a new descriptor and the client installs the new mapping before accepting data requests.

### Handle abrupt process loss through ownership

The Rialto client shares the consumer application's process lifetime. If that application process crashes, the kernel closes its IPC socket, descriptor duplicates, and mappings. The separately running RialtoServer observes the socket disconnect and destroys all media pipelines and WebAudio players associated with that client, releasing the remaining server mappings and descriptors.

RialtoServer is spawned and monitored by RialtoServerManager. If RialtoServer crashes while the application process survives, kernel teardown closes the server copies but client duplicates would keep the anonymous memfds alive. Client connection-loss/application-state invalidation therefore explicitly releases every instance-local handle and rejects writes. ServerManager may then spawn a replacement RialtoServer, but data transport resumes only after the application creates or recreates an instance and receives a fresh descriptor from that new server generation. Old session IDs, handles, mappings, and pending requests are never reused across generations.

If both processes terminate, kernel teardown releases all copies without application cleanup. A stalled RialtoServer event loop may delay application-disconnect cleanup until ServerManager recovery terminates or replaces the server, but admission limits bound the retained instances.

### Make server lifetime deterministic

The server-side instance and its shared-memory object have the same externally observable lifetime. Destroying a session or WebAudio player removes the instance from the service map; once outstanding server users release their references, the mapping is unmapped and the server descriptor is closed. IPC disconnect cleanup follows the same destruction path.

Client destruction unmaps and closes only the descriptor received by that client. Closing either side's duplicate does not invalidate the other side while it remains open.

### Remove global playback allocation and partition bookkeeping

`PlaybackService::switchToActive` will no longer allocate a playback buffer, and `switchToInactive` will not reset one. It will continue to clear media pipelines and WebAudio players, which releases all per-instance objects. Maximum instance counts remain admission limits but no longer determine allocation size.

`mapPartition` and `unmapPartition` calls disappear from media-pipeline and WebAudio construction/destruction. Session IDs and WebAudio handles continue to identify IPC and service instances, but not memory regions.

### Require lockstep protocol deployment

This change does not preserve old-client/new-server or new-client/old-server operation. The root project/schema major version will change from 1 to 2 so existing major-version compatibility checks reject incompatible peers before instance creation. No transitional global buffer will be allocated for old clients.

This policy avoids maintaining two ownership models and prevents an old client from receiving an ambiguous application-global descriptor when the server owns multiple independent objects.

## Failure and Concurrency Semantics

- Allocation failure fails only the instance being created; existing instances remain usable.
- Server `mmap`, resize, or sealing failure closes the temporary descriptor and fails creation.
- Client mapping failure closes the received descriptor and causes the newly created server instance to be destroyed.
- Simultaneous instance creation allocates independent objects and requires no shared partition lock.
- A descriptor and mapping are never published to another instance.
- Destroying one instance cannot alter offsets, capacity, or mapping validity for another instance.
- Inactive transition and IPC disconnect destroy every owned instance before server shutdown proceeds.
- No media request may be emitted until the server instance owns a valid mapped buffer and creation has completed.
- No client media write may proceed unless that client instance owns a valid mapped handle.

## Security and Resource Accounting

Per-instance mappings prevent one pipeline from addressing another pipeline's transport through a valid offset. Existing bounds checks remain required for every media and WebAudio write. Sizes received over IPC are validated against non-zero and representable mapping limits before `mmap`.

Admission limits continue to cap instance count. Each admitted media pipeline reserves 8.25 MiB and each admitted WebAudio player reserves 10 KiB. Failed and destroyed instances release both address-space mappings and descriptors. Logging identifies allocation and cleanup by instance ID without logging descriptor contents as durable identifiers.

## Migration Plan

1. Add per-instance shared-memory factories and direct region APIs with unit coverage.
2. Add descriptor and size fields to media-pipeline and WebAudio creation responses.
3. Allocate and retain buffers in server instance creation; remove partition claim/release behavior.
4. Update client IPC creation results and map descriptors into instance-owned handles.
5. Route media frame and WebAudio writes through instance-owned mappings.
6. Remove `ClientController` global shared-memory lifecycle and `ControlModule.getSharedMemory` usage.
7. Remove `PlaybackService` global allocation and obsolete partition APIs.
8. Update protocol version compatibility and reject mixed versions.
9. Add deployed-topology process tests using a dedicated application child and a RialtoServer process spawned/restarted through ServerManager; cover abrupt application death, server death, restart with fresh descriptors, and both-side termination.
10. Run targeted unit/component/process suites, then full unit, component, coverage, valgrind, native build, formatting, lint, and static analysis checks.

Rollback requires reverting client and server together because deployment is lockstep. There is no data migration.

## Risks / Trade-offs

- More descriptors and mappings at peak concurrency -> bounded by existing admission limits and negligible at expected instance counts.
- Creation response succeeds server-side but client mapping fails -> client cleanup must destroy the server instance and close the received descriptor.
- Broad factory/mock churn -> stage interface changes and update strict mocks with each layer.
- Lifecycle recreation accidentally retains a stale mapping -> reset pending requests and old handles before installing a newly returned descriptor.
- Protocol mismatch -> reject peers through schema compatibility checks rather than falling back.
- Fixed buffers may still over-allocate for audio-only pipelines -> accepted initially to isolate the ownership change; dynamic sizing remains future work.

## Resolved Decisions and Follow-ups

- Use one internal memfd allocation/mapping owner wrapped by role-specific media-pipeline and WebAudio layouts.
- Increment the root project/schema major version from 1 to 2 so existing compatibility checks enforce lockstep deployment.
- Use RAII on the client: assign the IPC instance guard before mapping, consume the received descriptor through a handle factory, and let constructor unwinding destroy the server instance if local mapping fails.
- Defer new production allocation metrics to a follow-up; this change retains useful logging and adds test-only resource accounting.
- Defer optimization of full source-region clearing until it can be measured independently from the ownership migration.
