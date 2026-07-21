#proposal: HFP[Hardware Feature Profile] schema v1.0.0
Why

- The HFP audio and video decoder YAML schema has been updated: The core structural change is that for video
  capabilities `dynamicRange` has moved from a single shared list at the video decoder-index level to a per-codec
  list inside each codec entry (MPEG2, H264, H265, VP9, AV1). The core structural change is that for audio capabilities is (`maxBitrateInBps`, `maxChannels`, `maxSampleRateInHz`, `maxBitDepth`) have moved from **codec level** to **per-profile level**, and profiles are now named map keys instead of flat strings.
- The old schema for video applied one `dynamicRange` list (e.g. SDR, HLG, HDR10,  DOLBY_VISION) to ALL codecs
  uniformly, which was inaccurate — different codecs support different HDR formats (e.g. VP9 does not support DOLBY_VISION, MPEG2 supports only SDR). The old schema for audio applied a single set of capabilities to all profiles
  of a codec uniformly, which was inaccurate — different profiles of the same codec can have different channel counts and bitrates (e.g. AAC LC vs AAC xHE-AAC have different max channels).
- `DOLBY_AC3` and `DOLBY_EAC3` were previously combined under one codec (`DOLBY_AC3`). This was incorrect — EAC3 (Enhanced AC-3 / Dolby Digital Plus) is a distinct codec from AC3 (Dolby Digital). Splitting them allows accurate
  GStreamer caps registration.
- `DOLBY_MAT` and `WMA` have been removed from the schema as these codecs are no longer part of the supported capability set.
- `maxBitDepth` was previously optional — it is now required in all Audio profiles, ensuring complete capability data is always present.
- Aligning Rialto with the new schema enables accurate, per-codec dynamic range reporting so that apps and browsers can make correct W3C `mediaCapabilities` decisions per codec for both audio and Video.

What Changes

- Update `AudioDecoderCapabilities.h`:

  - Add `AudioProfileCapability` struct (per-profile caps: bitrate, channels,
    sampleRate, bitDepth)
  - Add `DolbyEac3Profile` enum (`PLUS`, `PLUS_JOC`)
  - Add `MpegAudioProfile` enum (`LAYER_1`, `LAYER_2`)
  - Change `DolbyAc3Profile` enum: keep `STANDARD` only
  - Change all codec structs: replace flat fields with `std::map<Profile, AudioProfileCapability>` (for named-profile codecs) or `AudioProfileCapability base` (for single-profile codecs)
  - Remove `DolbyMatCapability`, `WmaCapability`, `DolbyMatProfile`, `WmaProfile`
  - Add `DolbyEac3Capability`
  - Remove `dolbyMat` and `wma` from `AudioDecoderCapability` struct
  - Add `dolbyEac3` to `AudioDecoderCapability` struct
  - Change `maxBitDepth` from `std::optional<uint32_t>` to `uint32_t` (required)
- Update `YamlCppWrapper.cpp`:

  - Rewrite all audio profile parsers to handle map-key format
  - Add `parseDolbyEac3Profiles()` for new codec
  - Add `parseMpegAudioProfiles()` for `LAYER_1`/`LAYER_2`
  - Add `parseBaseProfile()` for single-profile codecs
  - Add `parseAudioProfileCapability()` helper to read per-profile fields
  - Remove `getDolbyMatProfiles()`, `getWmaProfiles()` parsers
  - Remove `getCommonAudioParams()` (capabilities now inside profiles)
  - Update `buildAudioDecoderCapability()`: add `DOLBY_EAC3`, remove
    `DOLBY_MAT`/`WMA` cases
  - Update YAML parser to read `dynamicRange` from inside each codec node
    instead of from the decoder-index level
- Update `VideoDecoderCapabilities.h`:

  - Add per-codec capability structs (`Mpeg2CodecCapability`, `H264CodecCapability`,
    `H265CodecCapability`, `Vp9CodecCapability`, `Av1CodecCapability`) each
    containing both `profiles` and `dynamicRanges`
  - Update `VideoCodecCapabilities` struct: replace five flat profile vectors with
    five `std::optional<*CodecCapability>` fields
  - Remove top-level `dynamicRanges` from `VideoDecoderCapability` struct
- Update `proto/mediapipelinecapabilitiesmodule.proto`:

  - Update protobuf messages to reflect per-codec dynamic range structure
  - Add `AudioProfileCapability` message
  - Add `DolbyEac3Profile` enum, `MpegAudioProfile` enum
  - Remove `DolbyMatProfile`, `WmaProfile` enums
  - Change `DolbyAc3Profile` to `STANDARD` only
  - Add `AUDIO_CODEC_DOLBY_EAC3` to `AudioCodec` enum
  - Remove `AUDIO_CODEC_DOLBY_MAT`, `AUDIO_CODEC_WMA`
  - Rewrite all codec capability messages to use `repeated ProfileEntry`
    pattern (profile + capability) or `AudioProfileCapability base`
  - Remove `DolbyMatCapability`, `WmaCapability` messages
  - Add `DolbyEac3Capability` message
  - Update `AudioDecoderCapability`: remove `dolby_mat`/`wma`,
    add `dolby_eac3`
- Update `MediaPipelineCapabilitiesModuleService.cpp` (Server IPC):

  - Update server IPC serialization (`MediaPipelineCapabilitiesModuleService.cpp`)
    to populate per-codec dynamic ranges
  - Rewrite `convertAudioDecoderCapability()` to serialize new per-profile
    structs into protobuf `ProfileEntry` messages
  - Add converters for `DolbyEac3Profile`, `MpegAudioProfile`
  - Remove converters for `DolbyMatProfile`, `WmaProfile`
- Update `MediaPipelineCapabilitiesIpc.cpp` (Client IPC):

  - Update client IPC deserialization (`MediaPipelineCapabilitiesIpc.cpp`) to
    reconstruct per-codec dynamic ranges from protobuf
  - Rewrite all `convert*Capability()` functions to deserialize
    `ProfileEntry` protobuf messages into `std::map<Profile, AudioProfileCapability>`
  - Add `convertAudioProfileCapability()` shared helper
  - Add `convertDolbyEac3Capability()`, `convertMpegAudioCapability()`
  - Remove `convertDolbyMatCapability()`, `convertWmaCapability()`
  - Update `convertAudioDecoderCapability()` mapping

Non-Goals

- Will not add new video and audio codecs beyond the schema definition
- Will not implement spatial rendering / Dolby Atmos playback path
- Will not change GStreamer audio playback pipeline (only capability
  registration is affected in rialto-gstreamer — see separate proposal)
- Will not implement dynamic capabilities (HDMI, display output, HDR detection)
- Will not affect RDK-V devices (no YAML config files on RDK-V)

Impacted Areas

| Area                  | Impact Type                             | Location / Path                                  |
| --------------------- | ----------------------------------------| -------------------------------------------------|
| Public API            | Modified struct                         | `media/public/include/VideoDecoderCapabilities.h`|
| Public API            | Modified struct + enums                 | `media/public/include/AudioDecoderCapabilities.h`|
| YAML Parser           | Logic change + Full rewrite of audio section | `wrappers/source/YamlCppWrapper.cpp`        |
| Wrapper Interface     | No change                               | `wrappers/interface/IYamlCppWrapper.h`           |
| Protobuf IPC contract | Modified messages + enums               | `proto/mediapipelinecapabilitiesmodule.proto`    |
| Server IPC            | Serialization rewrite| `media/server/ipc/source/MediaPipelineCapabilitiesModuleService.cpp`|
| Client IPC            | Deserialization rewrite        | `media/client/ipc/source/MediaPipelineCapabilitiesIpc.cpp`|
| GStreamer layer       | No change (rialto repo)        | N/A — see rialto-gstreamer proposal                       |

---

Success Criteria

- `YamlCppWrapper` reads `dynamicRange` from inside each codec node in `hfp-videodecoder.yaml`
- `YamlCppWrapper` reads per-profile capabilities (bitrate, channels, sampleRate, bitDepth) from
  inside each named profile key in `hfp-audiodecoder.yaml`
- `VideoCodecCapabilities.mpeg2->dynamicRanges` contains only `[SDR]`
- `VideoCodecCapabilities.vp9->dynamicRanges` does **not** contain `DOLBY_VISION`
- `VideoDecoderCapability` has no top-level `dynamicRanges` field
- `AudioDecoderCapability.dolbyEac3` is populated when YAML contains
  `DOLBY_EAC3` codec with `PLUS` or `PLUS_JOC` profiles
- `AudioDecoderCapability.dolbyAc3` contains only `STANDARD` profile
- `AudioDecoderCapability.dolbyMat` is `nullopt` (codec removed)
- `AudioDecoderCapability.wma` is `nullopt` (codec removed)
- `maxBitDepth` is always present (no longer optional) in all profiles
- `MpegAudioCapability` contains `LAYER_1` and `LAYER_2` profiles
- Server IPC correctly serializes `ProfileEntry` messages for all
  map-profile codecs (AAC, DolbyAc3, DolbyEac3, RealAudio, USAC, DTS, AVS) and per-codec dynamic
  ranges into protobuf
- Client IPC correctly deserializes protobuf `ProfileEntry` messages
  back into `std::map<Profile, AudioProfileCapability>` and for video also deserializes protobuf
  back into per-codec structs
- New unit tests cover: per-profile parsing, DOLBY_EAC3 split from
  DOLBY_AC3, WMA/DOLBY_MAT absence, BASE profile codecs, required
  maxBitDepth
- New unit tests cover per-codec dynamic range parsing and optional codec presence checks
- All existing unit tests pass with no regressions

Rollback Plan

- **Code:** Revert the following files via `git revert`:
  - `media/public/include/VideoDecoderCapabilities.h`
  - `media/public/include/AudioDecoderCapabilities.h`
  - `wrappers/source/YamlCppWrapper.cpp`
  - `proto/mediapipelinecapabilitiesmodule.proto`
  - `media/server/ipc/source/MediaPipelineCapabilitiesModuleService.cpp`
  - `media/client/ipc/source/MediaPipelineCapabilitiesIpc.cpp`
- **YAML Config:** If new YAML is deployed to device but code is reverted, Rialto
  safely falls back to `getSupportedMimeTypes()` (legacy path) — existing safe behaviour.
- **RDK-V devices:** Unaffected — config files are absent on RDK-V and Rialto
  already defaults to existing behaviour via `CONFIG_NOT_FOUND` path.
- 

## References

- Jira: RDKEMW-15078
- Epic: CPESP-9957
- Audio Schema: `hfp-audiodecoder-schema.yaml` v1.0.0
- Video Schema: `hfp-videodecoder-schema.yaml`
- HLA: W3C Media Capabilities HLA [VL to Rialto]