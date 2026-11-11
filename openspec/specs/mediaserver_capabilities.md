# Spec: Media Server Decoder Capabilities

## Status
Drafted from PR #467 branch analysis (`master...origin/MediaPipelineCapabilitiesNewConfig`).

## Scope
This spec defines how Rialto media server exposes audio/video decoder capabilities loaded from YAML configuration and returned through server/client capabilities APIs.

In scope:
- Public capability models for audio and video decoder capabilities.
- YAML parsing and mapping rules for capability data.
- Server-side retrieval and IPC serialization.
- Client-side IPC deserialization.

Out of scope:
- MIME type support queries unrelated to decoder capability payload shape.
- Changes to server-manager orchestration.

## Source of Truth
Primary contracts:
- `media/public/include/IMediaPipelineCapabilities.h`
- `media/public/include/AudioDecoderCapabilities.h`
- `media/public/include/VideoDecoderCapabilities.h`
- `proto/mediapipelinecapabilitiesmodule.proto`

Capability loading and transport:
- `wrappers/interface/IYamlCppWrapper.h`
- `wrappers/source/YamlCppWrapper.cpp`
- `media/server/gstplayer/source/GstCapabilities.cpp`
- `media/server/ipc/source/MediaPipelineCapabilitiesModuleService.cpp`
- `media/client/ipc/source/MediaPipelineCapabilitiesIpc.cpp`

Related OpenSpec deltas from PR branch:
- `openspec/changes/hfp-schema-v1-migration/specs/audio-decoder-capabilities/spec.md`
- `openspec/changes/hfp-schema-v1-migration/specs/video-decoder-capabilities/spec.md`

## Problem Statement
Decoder capabilities were previously not sourced from HFP YAML in a structured, schema-driven way across server and client capability APIs. The system needs consistent, per-codec/per-profile capability data that can be parsed once on server startup and transported over existing capabilities IPC endpoints.

## Goals
- Load audio and video decoder capabilities from YAML configuration files.
- Expose capabilities through existing `IMediaPipelineCapabilities` methods.
- Preserve capability fidelity through protobuf server/client transport.
- Support optional codec presence and per-profile details.

## Non-Goals
- Introduce new RPC methods for decoder capabilities (reuse existing methods).
- Add a new top-level video dynamic range list.
- Change non-capabilities media pipeline behavior.

## Public API Contract
`IMediaPipelineCapabilities` provides:
- `AudioDecoderCapabilities getSupportedAudioCapabilities()`
- `VideoDecoderCapabilities getSupportedVideoCapabilities()`

These methods MUST return decoded capability models sourced from YAML-backed server data.

## Audio Capability Model

### Core struct
`AudioProfileCapability` contains:
- `maxBitrateInBps` (implemented as `uint64_t`)
- `maxChannels` (`uint32_t`)
- `maxSampleRateInHz` (`uint32_t`)
- `maxBitDepth` (`uint32_t`)

### Representation rules
- Named-profile codecs use `std::map<ProfileEnum, AudioProfileCapability>`.
- Single-profile codecs use a `base` field of type `AudioProfileCapability`.

### Codec coverage
Audio decoder capabilities include optional codec fields such as PCM, AAC, MPEG Audio, MP3, ALAC, SBC, Dolby AC3, Dolby AC4, Dolby EAC3, Dolby TrueHD, FLAC, Vorbis, Opus, RealAudio, USAC, DTS, AVS.

### Schema-driven behavior highlights
- Dolby EAC3 is modeled separately from Dolby AC3.
- MPEG Audio supports `LAYER_1` and `LAYER_2` profile mapping.
- Fields removed by HFP schema migration (for example legacy DolbyMat and WMA structures) are not represented in the new capability model.

## Video Capability Model

### Per-codec structure
`VideoCodecCapabilities` contains optional codec entries:
- `mpeg2`, `h264`, `h265`, `vp9`, `av1`

Each present codec entry includes:
- `profiles` (typed profile vector with level and max bitrate)
- `dynamicRanges` (`std::vector<DynamicRange>`)

### Dynamic range rule
- Dynamic range is per codec.
- There is no top-level `dynamicRanges` field in `VideoDecoderCapability`.

## YAML Input and Parsing Rules

### Files
Capabilities are loaded from dedicated YAML decoder capability config files (audio and video), parsed by `YamlCppWrapper`.

### Audio parsing
- Audio codec entries are read under decoder capabilities lists.
- Named-profile codecs parse profile-keyed maps.
- Single-profile codecs parse a base profile capability.
- Unknown/unsupported profile names are ignored by conversion helpers.

### Video parsing
- Video codec entries parse profiles and `dynamicRange` under each codec node.
- Profile builders map YAML names to typed enums and extract `maxLevel` and `maxBitrateInBps`.

### Error handling
- Parser reports success/failure via `DecoderCapabilitiesStatus`.
- Server logs warnings when YAML capabilities cannot be loaded.

## Server Behavior
- `GstCapabilities` acquires `IYamlCppWrapper` from wrapper factory.
- During construction, server attempts to populate in-memory audio and video decoder capability models.
- YAML load failure does not crash capability service construction; warning logs are emitted and default/empty models may be returned.

## IPC Contract

### Protobuf service
`MediaPipelineCapabilitiesModule` already exposes:
- `getSupportedAudioCapabilities`
- `getSupportedVideoCapabilities`

PR #467 extends payload schemas for richer capability data, including:
- Audio per-profile entry messages and `AudioProfileCapability` message.
- Video per-codec capability messages with codec-specific dynamic ranges.

### Serialization
Server IPC module MUST serialize only present optional codec capability fields.

### Deserialization
Client IPC module MUST reconstruct:
- optional codec entries (`std::nullopt` when absent)
- per-profile maps or vectors
- enum mappings for profiles/levels/dynamic ranges

## Compatibility and Migration
- Existing capability method names are preserved.
- Data model shape changes are additive/restructuring within capability payloads.
- Consumers must rely on optional presence checks for codecs and profile maps.

## Acceptance Criteria
- Audio capabilities returned by client API match YAML-defined profile capabilities for each supported codec.
- Video capabilities returned by client API include per-codec profiles and per-codec dynamic ranges.
- Absent codec entries remain unset (`std::nullopt`) after server parse and client decode.
- Protobuf round-trip preserves profile, level, bitrate, and dynamic range values.
- Unit and component tests for server/client capabilities pass for the new model shape.

## Risks and Follow-ups
- YAML key mismatches (profile naming drift) can silently reduce populated capability entries.
- Strictness for required base-profile semantics should be consistently enforced in parser logic.
- Capability schema versioning policy should be documented for downstream consumers.