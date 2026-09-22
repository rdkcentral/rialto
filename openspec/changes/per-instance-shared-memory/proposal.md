## Why

Rialto currently allocates one application-wide shared-memory object when the session server becomes active. The object reserves capacity for every configured media pipeline and WebAudio player, even when most instances are never created, and every instance depends on application-global allocation, mapping, partition assignment, and teardown.

Per-instance shared memory aligns allocation with resource ownership: a media pipeline receives one transport buffer for its own audio, video, and subtitle data, while a WebAudio player receives one buffer for its own ring. Creation and descriptor transfer become atomic, destruction releases the corresponding object, and unrelated instances no longer share an address space.

## What Changes

- Allocate one fixed-layout shared-memory object while creating each media pipeline.
- Allocate one fixed-size shared-memory object while creating each WebAudio player.
- Return the object descriptor and mapping size in the corresponding creation RPC response.
- Make the client `MediaPipeline` and `WebAudioPlayer` own their respective mapped handles instead of retrieving an application-global handle from `ClientController`.
- Make server media-pipeline and WebAudio-player instances own their respective server mappings.
- Remove playback partition reservation by session ID or WebAudio handle; all transport offsets become relative to the owning instance's mapping.
- Stop allocating playback shared memory when the session server becomes active.
- Retire `ControlModule.getSharedMemory` from the lockstep client/server protocol and remove application-global playback shared-memory ownership from `PlaybackService` and `ClientController`.
- Preserve the shipping region sizes initially: 7 MiB video, 1 MiB audio, and 256 KiB subtitle per media pipeline, and 10 KiB per WebAudio player.
- Preserve full source-region clearing before each immediate or delayed media need-data notification; clearing optimization is a separate measured change.
- Release instance-local client mappings when server loss invalidates the instance, and require a fresh descriptor before a recreated instance accepts data.
- Require lockstep client/server deployment under schema major version 2; mixed old/new protocol versions are not supported by this change.
- Validate crash and restart behavior in the deployed topology: the Rialto client inside the application process, and the separate RialtoServer process spawned and restarted by RialtoServerManager.

## Capabilities

### New Capabilities

- `per-instance-shared-memory`: Allocate, transfer, map, use, and release an independent shared-memory transport for each media pipeline and WebAudio player.

### Modified Capabilities

None.

## Impact

Affected areas include shared-memory interfaces and factories, playback and instance services, media-pipeline and WebAudio server implementations, creation protobufs and IPC services, client IPC factories, client instance ownership, media frame writing, WebAudio ring writing, strict mocks, and unit/component coverage.

The change is intentionally protocol-breaking under a lockstep deployment policy. It does not change the public `IMediaPipeline` or `IWebAudioPlayer` application APIs, media request offset structures, or media payload formats. Peak allocation at maximum concurrency remains approximately unchanged, while allocation at lower concurrency becomes proportional to active instances.
