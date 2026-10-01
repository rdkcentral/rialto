## Context

Rialto queries per-device hardware capabilities from two HFP (Hardware Feature Profile) YAML
configuration files: `hfp-videodecoder.yaml` and `hfp-audiodecoder.yaml`. Both have been
updated in HFP schema v1.0.0 with structural changes that break the existing C++ API,
protobuf contract, and YAML parser.

**Video change:** `dynamicRange` moves from a single shared list at the decoder-index level to
a per-codec list inside each codec entry (MPEG2, H264, H265, VP9, AV1).

**Audio change:** Per-codec capability fields (`maxBitrateInBps`, `maxChannels`,
`maxSampleRateInHz`, `maxBitDepth`) move from codec level to per-profile level. Profiles are
now named map keys instead of flat strings. Additionally:
- `DOLBY_EAC3` is split out from `DOLBY_AC3` as a separate codec
- `DOLBY_MAT` and `WMA` are removed from the capability set
- `maxBitDepth` becomes required (was `std::optional`)

The change spans five layers for both audio and video:

| Layer | File |
|---|---|
| Public API (video) | `media/public/include/VideoDecoderCapabilities.h` |
| Public API (audio) | `media/public/include/AudioDecoderCapabilities.h` |
| YAML parser | `wrappers/source/YamlCppWrapper.cpp` |
| Protobuf contract | `proto/mediapipelinecapabilitiesmodule.proto` |
| Server IPC | `media/server/ipc/source/MediaPipelineCapabilitiesModuleService.cpp` |
| Client IPC | `media/client/ipc/source/MediaPipelineCapabilitiesIpc.cpp` |

RDK-V devices carry no YAML config; Rialto already handles `CONFIG_NOT_FOUND` safely.

## Goals / Non-Goals

**Goals:**
- Align per-codec `dynamicRange` reporting with HFP video schema v1.0.0
- Align per-profile audio capability reporting with HFP audio schema v1.0.0
- Split `DOLBY_EAC3` from `DOLBY_AC3` as a distinct codec
- Remove unsupported codecs (`DOLBY_MAT`, `WMA`) from the capability API
- Correctly serialize/deserialize updated structures over the Rialto IPC channel
- No regressions on RDK-V (config-absent) devices

**Non-Goals:**
- Adding new video/audio codec types beyond the schema definition
- Spatial rendering / Dolby Atmos playback path
- Dynamic capability updates (HDMI hotplug, display output detection)
- Changes to the rialto-gstreamer layer (separate proposal)
- RDK-V devices (no YAML config files present)

## Decisions

### D1: Per-codec video capability structs with `std::optional`

**Decision**: Define one struct per video codec (`Mpeg2CodecCapability`, `H264CodecCapability`,
`H265CodecCapability`, `Vp9CodecCapability`, `Av1CodecCapability`), each holding typed `profiles`
and `dynamicRanges` vectors. Hold each in `VideoCodecCapabilities` as
`std::optional<XxxCodecCapability>`.

**Rationale**: `std::optional` naturally encodes "codec absent in YAML" vs. "codec present but
with empty lists". A flat profile vector cannot make this distinction. Typed structs allow
per-codec API evolution without touching sibling codecs.

**Alternatives considered**:
- *Single generic `CodecCapability` struct*: Loses per-codec type identity; IDE completion degraded.
- *`std::shared_ptr` with `nullptr` sentinel*: Heap allocation for logically value data.

### D2: Remove top-level `dynamicRanges` from `VideoDecoderCapability`

**Decision**: `VideoDecoderCapability` will not carry a shared `dynamicRanges` field.

**Rationale**: A shared list implies all codecs have identical dynamic-range support, which is
false (e.g. VP9 does not support DOLBY_VISION, MPEG2 supports only SDR).

**Alternatives considered**:
- *Keep shared list as deprecated alias*: Persists a misleading API; requires server to either
  pick one codec's list arbitrarily or compute a union.

### D3: Per-profile audio capabilities via `std::map<Profile, AudioProfileCapability>`

**Decision**: Replace flat codec-level capability fields (`maxBitrateInBps` etc.) with a
`std::map<ProfileEnum, AudioProfileCapability>` for named-profile codecs (AAC, DolbyAc3,
DolbyEac3, RealAudio, USAC, DTS, AVS). Single-profile codecs (MPEG, Opus, PCM, Vorbis) use a
single `AudioProfileCapability base` field.

**Rationale**: The new YAML schema defines capabilities per named-profile key. Different profiles
of the same codec genuinely have different limits (e.g. AAC LC vs xHE-AAC max channels differ).
`std::map` gives O(1) lookup by profile and makes the per-profile structure explicit in the API.

**Alternatives considered**:
- *Keep flat fields, add separate `perProfileCaps` map*: Redundant data; risk of inconsistency
  between top-level and per-profile fields.
- *`std::vector<pair<Profile, Caps>>`*: Loses map semantics; callers must iterate.

### D4: Split `DOLBY_EAC3` from `DOLBY_AC3`

**Decision**: Define a new `DolbyEac3Capability` struct / `DolbyEac3Profile` enum
(`PLUS`, `PLUS_JOC`) and a new `dolbyEac3` field in `AudioDecoderCapability`. Remove the
implicit EAC3 coverage from `DolbyAc3Capability`. `DolbyAc3Profile` keeps only `STANDARD`.

**Rationale**: EAC3 (Dolby Digital Plus) is a distinct codec from AC3 (Dolby Digital). Combining
them under one struct made it impossible to register accurate GStreamer caps and to report
correct capability data to W3C `mediaCapabilities`.

### D5: Remove `DOLBY_MAT` and `WMA`

**Decision**: Remove `DolbyMatCapability`, `WmaCapability`, their profile enums, and the
corresponding fields from `AudioDecoderCapability`. Remove the codec enums from the proto.

**Rationale**: These codecs are no longer part of the supported capability set per HFP schema
v1.0.0. Retaining them would report false capability data to callers.

### D6: `maxBitDepth` becomes required

**Decision**: Change `maxBitDepth` from `std::optional<uint32_t>` to `uint32_t` in
`AudioProfileCapability`. The proto field remains `optional` at the wire level for
backward compatibility but the C++ API treats absence as a schema error.

**Rationale**: HFP v1.0.0 mandates `maxBitDepth` in every profile entry. Making it required in
C++ enforces completeness at parse time rather than at every call site.

## Risks / Trade-offs

- **Video wire-format change** → Updated `VideoCodecCapabilities` proto messages break
  compatibility with old clients. Mitigation: client and server deployed together.
- **Audio wire-format change** → `ProfileEntry` message pattern and new DolbyEac3 messages
  are incompatible with old clients. Mitigation: same release deployment.
- **YAML schema mismatch on device** → Old YAML + new binary → parser finds no per-codec
  `dynamicRange` / per-profile keys → empty/nullopt capabilities (graceful degradation).
  Mitigation: YAML and binary updated in the same release.
- **`DolbyAc3Profile` enum narrowing** → Any caller checking for EAC3 via `DolbyAc3Capability`
  will silently miss it after the split. Mitigation: document the breaking change; consumers
  must migrate to `dolbyEac3`.
- **Removing `dolbyMat` / `wma`** → Any existing consumer checking these fields gets
  `std::nullopt`. Mitigation: document removal; no known external consumers.

## Migration Plan

**Video (already implemented; retain for rollback reference):**
1. Update `VideoDecoderCapabilities.h` — add `XxxCodecCapability` structs, update
   `VideoCodecCapabilities` to optional fields, remove top-level `dynamicRanges`.
2. Update `YamlCppWrapper.cpp` — `buildVideoDecoderCapability` reads per-codec `dynamicRange`.
3. Update proto — new per-codec capability messages inside `GetSupportedVideoCapabilitiesResponse`.
4. Update `MediaPipelineCapabilitiesModuleService.cpp` — per-codec serialization.
5. Update `MediaPipelineCapabilitiesIpc.cpp` — per-codec deserialization.

**Audio (pending):**
6. Update `AudioDecoderCapabilities.h` — add `AudioProfileCapability`, `DolbyEac3Capability`,
   `MpegAudioProfile`, rewrite codec structs, remove `DolbyMat`/`WMA`.
7. Rewrite audio parsers in `YamlCppWrapper.cpp` — map-key format, per-profile fields,
   add `DOLBY_EAC3` / `MPEG_AUDIO`, remove `DOLBY_MAT` / `WMA`.
8. Update proto audio messages — `ProfileEntry` pattern, `DolbyEac3Capability`,
   `MpegAudioProfile`, remove DolbyMat/WMA.
9. Rewrite `convertAudioDecoderCapability` in `MediaPipelineCapabilitiesModuleService.cpp`.
10. Rewrite audio `convert*Capability` functions in `MediaPipelineCapabilitiesIpc.cpp`.
11. Add/update unit tests for all audio changes.

**Rollback**: `git revert` the six files listed in the proposal. RDK-V devices unaffected.

## Open Questions

- Which audio codecs use `base` (single-profile) vs `map` (named-profile)?
  → Named-profile: AAC, DolbyAc3, DolbyEac3, RealAudio, USAC, DTS, AVS.
     Single-profile (`base`): MPEG, Opus, PCM, Vorbis, FLAC, MP3.
- Proto field numbers for new `DolbyEac3Capability` and `MpegAudioProfile` messages?
  → Use next available numbers; document in the `.proto` file comment.
