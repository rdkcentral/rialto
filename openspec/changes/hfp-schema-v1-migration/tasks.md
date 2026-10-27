## 1. Video Public API — VideoDecoderCapabilities header

- [x] 1.1 Update `media/public/include/VideoDecoderCapabilities.h`: add `Mpeg2CodecCapability`, `H264CodecCapability`, `H265CodecCapability`, `Vp9CodecCapability`, `Av1CodecCapability` structs each containing typed `profiles` and `dynamicRanges` vectors
- [x] 1.2 Update `VideoCodecCapabilities` struct: replace five flat profile vectors with five `std::optional<XxxCodecCapability>` fields (`mpeg2`, `h264`, `h265`, `vp9`, `av1`)
- [x] 1.3 Update `VideoDecoderCapability` struct: remove top-level `dynamicRanges` field

## 2. Audio Public API — AudioDecoderCapabilities header

- [x] 2.1 Add `AudioProfileCapability` struct with `maxBitrateInBps`, `maxChannels`, `maxSampleRateInHz`, `maxBitDepth` (all `uint32_t`, not optional)
- [x] 2.2 Add `DolbyEac3Profile` enum (`PLUS`, `PLUS_JOC`) and `DolbyEac3Capability` struct (`std::map<DolbyEac3Profile, AudioProfileCapability>`)
- [x] 2.3 Add `MpegAudioProfile` enum (`LAYER_1`, `LAYER_2`) and update `MpegAudioCapability` to use `std::map<MpegAudioProfile, AudioProfileCapability>`
- [x] 2.4 Update `DolbyAc3Profile` enum: keep `STANDARD` only; update `DolbyAc3Capability` to `std::map<DolbyAc3Profile, AudioProfileCapability>`
- [x] 2.5 Rewrite all named-profile codec capability structs (AAC, RealAudio, USAC, DTS, AVS) to use `std::map<ProfileEnum, AudioProfileCapability>`
- [x] 2.6 Rewrite single-profile codec capability structs (Opus, PCM, Vorbis, FLAC, MP3) to use `AudioProfileCapability base`
- [x] 2.7 Remove `DolbyMatCapability`, `WmaCapability`, `DolbyMatProfile`, `WmaProfile`
- [x] 2.8 Update `AudioDecoderCapability` struct: add `dolbyEac3`, remove `dolbyMat` and `wma`

## 3. YAML Parser — Video changes

- [x] 3.1 Update `buildVideoDecoderCapability()` in `wrappers/source/YamlCppWrapper.cpp`: read `dynamicRange` from inside each codec node (`MPEG2_VIDEO`, `H264_AVC`, `H265_HEVC`, `VP9`, `AV1`); populate `XxxCodecCapability.dynamicRanges`; set `std::nullopt` for absent codecs

## 4. YAML Parser — Audio changes

- [x] 4.1 Add `parseAudioProfileCapability()` helper to read per-profile fields (`maxBitrateInBps`, `maxChannels`, `maxSampleRateInHz`, `maxBitDepth`) from a YAML profile-capability node
- [x] 4.2 Rewrite `buildAudioDecoderCapability()` to handle named map-key profile format for AAC, DolbyAc3, DolbyEac3, RealAudio, USAC, DTS, AVS codecs
- [x] 4.3 Add `parseDolbyEac3Profiles()` for the new `DOLBY_EAC3` codec entry
- [x] 4.4 Add `parseMpegAudioProfiles()` for `LAYER_1` / `LAYER_2`
- [x] 4.5 Add `parseBaseProfile()` helper for single-profile codecs (Opus, PCM, Vorbis, FLAC, MP3)
- [x] 4.6 Remove `getDolbyMatProfiles()`, `getWmaProfiles()` parsers and their `DOLBY_MAT` / `WMA` handling in `buildAudioDecoderCapability()`
- [x] 4.7 Remove `getCommonAudioParams()` (capabilities now live inside profiles)

## 5. Protobuf Contract — Audio changes

- [x] 5.1 Add `AudioProfileCapability` message to `proto/mediapipelinecapabilitiesmodule.proto` with `max_bitrate_in_bps`, `max_channels`, `max_sample_rate_in_hz`, `max_bit_depth` fields
- [x] 5.2 Add `DolbyEac3Profile` enum (`DOLBY_EAC3_PROFILE_PLUS`, `DOLBY_EAC3_PROFILE_PLUS_JOC`) and `DolbyEac3Capability` message using `ProfileEntry`-pattern
- [x] 5.3 Add `MpegAudioProfile` enum (`MPEG_AUDIO_PROFILE_LAYER_1`, `MPEG_AUDIO_PROFILE_LAYER_2`) and update `MpegAudioCapability` message
- [x] 5.4 Update `DolbyAc3Profile` to `STANDARD` only; rewrite `DolbyAc3Capability` to use `ProfileEntry`-pattern
- [x] 5.5 Rewrite all named-profile codec capability messages (AAC, RealAudio, USAC, DTS, AVS) to use `repeated ProfileEntry` pattern
- [x] 5.6 Add `AUDIO_CODEC_DOLBY_EAC3` to `AudioCodec` enum; remove `AUDIO_CODEC_DOLBY_MAT`, `AUDIO_CODEC_WMA`
- [x] 5.7 Remove `DolbyMatCapability`, `WmaCapability` messages and their `DolbyMatProfile`, `WmaProfile` enums
- [x] 5.8 Update `AudioDecoderCapability` message: add `dolby_eac3`, remove `dolby_mat` and `wma` fields

## 6. Server IPC — Serialization

- [x] 6.1 Update `convertVideoCodecCapabilities()` in `MediaPipelineCapabilitiesModuleService.cpp`: handle `std::optional` per-codec fields; populate typed profile lists and per-codec `dynamic_ranges`
- [x] 6.2 Remove `dst->add_dynamic_ranges()` call from `convertVideoDecoderCapability()` (top-level field removed)
- [x] 6.3 Rewrite `convertAudioDecoderCapability()`: add serializer for `DolbyEac3`, `MpegAudio`; remove `DolbyMat` / `WMA` converters; serialize `ProfileEntry` for all map-profile codecs
- [x] 6.4 Add `convertDolbyEac3Profile()`, `convertMpegAudioProfile()` helpers; remove `convertDolbyMatProfile()`, `convertWmaProfile()`

## 7. Client IPC — Deserialization

- [x] 7.1 Update `convertVideoCodecCapabilities()` in `MediaPipelineCapabilitiesIpc.cpp`: reconstruct `std::optional<XxxCodecCapability>` with typed profiles and per-codec `dynamicRanges`; set absent codecs to `std::nullopt`
- [x] 7.2 Remove `result.dynamicRanges` reconstruction from `convertVideoDecoderCapability()` (field removed)
- [x] 7.3 Add `convertAudioProfileCapability()` shared helper
- [x] 7.4 Rewrite all `convert*Capability()` audio functions to deserialize `ProfileEntry` into `std::map<Profile, AudioProfileCapability>`
- [x] 7.5 Add `convertDolbyEac3Capability()`, `convertMpegAudioCapability()`; remove `convertDolbyMatCapability()`, `convertWmaCapability()`
- [x] 7.6 Update `convertAudioDecoderCapability()` mapping: add `dolbyEac3`, remove `dolbyMat` / `wma`

## 8. Unit Tests

- [x] 8.1 Update video YAML wrapper tests: per-codec `dynamicRange` parsing, absent codec → `nullopt`, empty list
- [x] 8.2 Add audio YAML wrapper tests: per-profile capability parsing, `DOLBY_EAC3` split, `WMA`/`DOLBY_MAT` absence, `BASE` profile codecs, required `maxBitDepth`
- [x] 8.3 Update `MediaPipelineServiceMock` and `MediaPipelineCapabilitiesMock` for changed return types
- [x] 8.4 Update `MediaPipelineCapabilitiesIpcMock` and `MediaPipelineCapabilitiesModuleMock` for updated method signatures
- [x] 8.5 Update/add `MediaPipelineCapabilitiesModuleService` tests: video per-codec serialization, audio `ProfileEntry` serialization, `nullopt`/absent codec cases
- [x] 8.6 Update/add `MediaPipelineCapabilitiesIpc` tests: video per-codec deserialization, audio `ProfileEntry` deserialization, absent-codec `nullopt` cases
- [x] 8.7 Run all existing unit tests and confirm no regressions
