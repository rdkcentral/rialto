## ADDED Requirements

### Requirement: Per-codec capability structs
The system SHALL define per-codec capability structs (`Mpeg2CodecCapability`,
`H264CodecCapability`, `H265CodecCapability`, `Vp9CodecCapability`,
`Av1CodecCapability`), each containing a `profiles` vector and a `dynamicRanges`
vector of strings.

#### Scenario: Codec struct contains both profiles and dynamic ranges
- **WHEN** a codec entry is present in the HFP YAML with both `profiles` and `dynamicRange` lists
- **THEN** the corresponding codec capability struct SHALL be populated with those profiles and dynamic ranges

#### Scenario: Codec absent from YAML
- **WHEN** a codec entry is absent from the HFP YAML configuration
- **THEN** the corresponding `std::optional<XxxCodecCapability>` field in `VideoCodecCapabilities` SHALL be `std::nullopt`

### Requirement: VideoCodecCapabilities uses optional per-codec fields
The system SHALL define `VideoCodecCapabilities` with five `std::optional` fields —
`mpeg2`, `h264`, `h265`, `vp9`, `av1` — each holding their respective
`XxxCodecCapability` struct.

#### Scenario: Only supported codecs are populated
- **WHEN** the HFP YAML lists a subset of codecs (e.g. H264 and H265 only)
- **THEN** only those codec fields in `VideoCodecCapabilities` SHALL contain values; the rest SHALL be `std::nullopt`

### Requirement: VideoDecoderCapability has no top-level dynamicRanges
The `VideoDecoderCapability` struct SHALL NOT contain a top-level `dynamicRanges`
field. Dynamic range information SHALL only be accessible via per-codec structs.

#### Scenario: No shared dynamic range list
- **WHEN** a consumer inspects the `VideoDecoderCapability` returned by the API
- **THEN** there SHALL be no `dynamicRanges` field at the decoder level

### Requirement: YAML parser reads dynamicRange per codec node
The YAML parser (`YamlCppWrapper`) SHALL read `dynamicRange` from inside each
individual codec node in `hfp-videodecoder.yaml`, not from the decoder-index level.

#### Scenario: MPEG2 codec has only SDR dynamic range
- **WHEN** the YAML defines `dynamicRange: [SDR]` inside the MPEG2 codec node
- **THEN** `VideoCodecCapabilities.mpeg2->dynamicRanges` SHALL equal `["SDR"]`

#### Scenario: VP9 codec does not have DOLBY_VISION
- **WHEN** the YAML defines `dynamicRange: [SDR, HLG, HDR10]` inside the VP9 codec node
- **THEN** `VideoCodecCapabilities.vp9->dynamicRanges` SHALL NOT contain `"DOLBY_VISION"`

#### Scenario: H265 codec has multiple dynamic ranges
- **WHEN** the YAML defines `dynamicRange: [SDR, HLG, HDR10, DOLBY_VISION]` inside the H265 codec node
- **THEN** `VideoCodecCapabilities.h265->dynamicRanges` SHALL contain all four values

### Requirement: Protobuf encodes per-codec capability
The protobuf schema SHALL define a `PerCodecCapability` message with `repeated string profiles`
and `repeated string dynamic_ranges`, and use it for each codec field in
`GetVideoDecoderCapabilitiesResponse`.

#### Scenario: Absent codec not transmitted
- **WHEN** a codec is absent (i.e. `std::nullopt`) during server serialization
- **THEN** the corresponding `optional PerCodecCapability` field SHALL NOT be set in the protobuf message

#### Scenario: Present codec fully transmitted
- **WHEN** a codec capability struct is present (i.e. has a value)
- **THEN** both `profiles` and `dynamic_ranges` SHALL be set in the corresponding `PerCodecCapability` field

### Requirement: Server IPC serializes per-codec dynamic ranges
`MediaPipelineCapabilitiesModuleService` SHALL populate per-codec `PerCodecCapability`
fields in the protobuf response, including both profiles and dynamic ranges for each
codec that is present.

#### Scenario: Server serializes all present codecs
- **WHEN** the server retrieves `VideoDecoderCapabilities` with H264 and VP9 populated
- **THEN** the protobuf response SHALL have `h264` and `vp9` `PerCodecCapability` fields set, each with correct profiles and dynamic_ranges

### Requirement: Client IPC deserializes per-codec dynamic ranges
`MediaPipelineCapabilitiesIpc` SHALL reconstruct per-codec `XxxCodecCapability`
structs from the `PerCodecCapability` protobuf fields, and set absent codecs to
`std::nullopt`.

#### Scenario: Client reconstructs per-codec structs
- **WHEN** the protobuf response contains `h265` and `av1` `PerCodecCapability` fields
- **THEN** `VideoCodecCapabilities.h265` and `VideoCodecCapabilities.av1` SHALL be populated with the correct profiles and dynamicRanges

#### Scenario: Client handles absent codec fields
- **WHEN** the protobuf response does not have a `PerCodecCapability` field set for MPEG2
- **THEN** `VideoCodecCapabilities.mpeg2` SHALL be `std::nullopt`
