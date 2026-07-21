## ADDED Requirements

### Requirement: AudioProfileCapability struct
The system SHALL define an `AudioProfileCapability` struct containing `maxBitrateInBps`,
`maxChannels`, `maxSampleRateInHz`, and `maxBitDepth`, all as `uint32_t` (required, not
optional).

#### Scenario: All capability fields are present
- **WHEN** an audio profile entry is parsed from the HFP YAML
- **THEN** the resulting `AudioProfileCapability` SHALL have all four fields populated with
  the values from the YAML

#### Scenario: maxBitDepth is always required
- **WHEN** the HFP YAML profile entry contains a `maxBitDepth` value
- **THEN** `AudioProfileCapability.maxBitDepth` SHALL be set to that value and SHALL NOT be
  `std::nullopt`

### Requirement: Per-profile audio capabilities via map
The system SHALL represent audio codec capabilities as a `std::map<ProfileEnum,
AudioProfileCapability>` for named-profile codecs (AAC, DolbyAc3, DolbyEac3, RealAudio,
USAC, DTS, AVS). Single-profile codecs (Opus, PCM, Vorbis, FLAC, MP3, MPEG) SHALL use an
`AudioProfileCapability base` field.

#### Scenario: Named-profile codec populates map entries
- **WHEN** the HFP YAML defines an AAC codec with profiles `LC` and `HE_AAC_V1`
- **THEN** `AacCapability.profiles` SHALL be a map with keys `AacProfile::LC` and
  `AacProfile::HE_AAC_V1`, each mapping to the corresponding `AudioProfileCapability`

#### Scenario: Single-profile codec uses base field
- **WHEN** the HFP YAML defines an Opus codec entry
- **THEN** `OpusCapability.base` SHALL be populated with the capability values from the
  single profile in the YAML

#### Scenario: Map key lookup returns correct per-profile caps
- **WHEN** a consumer queries `AacCapability.profiles.at(AacProfile::HE_AAC_V2)`
- **THEN** the returned `AudioProfileCapability` SHALL reflect the HE-AAC v2-specific
  values (e.g. different `maxChannels` from LC)

### Requirement: DolbyEac3 split from DolbyAc3
The system SHALL define a separate `DolbyEac3Capability` struct with `DolbyEac3Profile`
enum (`PLUS`, `PLUS_JOC`), and a `dolbyEac3` field in `AudioDecoderCapability`.
`DolbyAc3Profile` SHALL contain only `STANDARD`; there SHALL be no EAC3 profile in
`DolbyAc3Capability`.

#### Scenario: DOLBY_EAC3 codec entry populates dolbyEac3 field
- **WHEN** the HFP YAML contains a `DOLBY_EAC3` codec with `PLUS` and `PLUS_JOC` profiles
- **THEN** `AudioDecoderCapability.dolbyEac3` SHALL be populated with both profiles mapped
  to their respective `AudioProfileCapability` values

#### Scenario: DolbyAc3 contains only STANDARD profile
- **WHEN** the HFP YAML defines a `DOLBY_AC3` codec
- **THEN** `AudioDecoderCapability.dolbyAc3` SHALL only contain entries for
  `DolbyAc3Profile::STANDARD` and SHALL NOT contain any EAC3 profile

#### Scenario: EAC3 not accessible via dolbyAc3
- **WHEN** a consumer accesses `AudioDecoderCapability.dolbyAc3`
- **THEN** the returned map SHALL NOT contain `DolbyEac3Profile::PLUS` or
  `DolbyEac3Profile::PLUS_JOC` (they are only in `dolbyEac3`)

### Requirement: MpegAudio profiles LAYER_1 and LAYER_2
The system SHALL define `MpegAudioProfile` enum with values `LAYER_1` and `LAYER_2`.
`MpegAudioCapability` SHALL use `std::map<MpegAudioProfile, AudioProfileCapability>`.

#### Scenario: MPEG_AUDIO codec populates both layers
- **WHEN** the HFP YAML defines a `MPEG_AUDIO` codec with `LAYER_1` and `LAYER_2` entries
- **THEN** `AudioDecoderCapability.mpegAudio` SHALL contain map entries for both
  `MpegAudioProfile::LAYER_1` and `MpegAudioProfile::LAYER_2`

## REMOVED Requirements

### Requirement: DolbyMat capability
**Reason**: `DOLBY_MAT` is no longer part of the supported capability set per HFP schema
v1.0.0.
**Migration**: Consumers checking `AudioDecoderCapability.dolbyMat` SHALL treat its absence
as `std::nullopt`; no replacement capability exists.

#### Scenario: dolbyMat field is absent
- **WHEN** the HFP YAML is parsed according to schema v1.0.0
- **THEN** `AudioDecoderCapability.dolbyMat` SHALL be `std::nullopt`

### Requirement: WMA capability
**Reason**: `WMA` is no longer part of the supported capability set per HFP schema v1.0.0.
**Migration**: Consumers checking `AudioDecoderCapability.wma` SHALL treat its absence as
`std::nullopt`; no replacement capability exists.

#### Scenario: wma field is absent
- **WHEN** the HFP YAML is parsed according to schema v1.0.0
- **THEN** `AudioDecoderCapability.wma` SHALL be `std::nullopt`

## MODIFIED Requirements

### Requirement: YAML parser reads audio capabilities per-profile
The YAML parser (`YamlCppWrapper`) SHALL read audio capability fields (`maxBitrateInBps`,
`maxChannels`, `maxSampleRateInHz`, `maxBitDepth`) from inside each named profile key in
`hfp-audiodecoder.yaml`, not from the codec level.

#### Scenario: Per-profile capability parsing for AAC
- **WHEN** the HFP YAML defines AAC with named profile keys each containing capability
  sub-fields
- **THEN** each profile key SHALL map to its own `AudioProfileCapability` with the correct
  `maxBitrateInBps`, `maxChannels`, `maxSampleRateInHz`, and `maxBitDepth` values

#### Scenario: Absent profile key yields no map entry
- **WHEN** the HFP YAML defines an AAC codec without a `HE_AAC_V2` profile key
- **THEN** `AacCapability.profiles` SHALL NOT contain an entry for `AacProfile::HE_AAC_V2`

### Requirement: Protobuf encodes per-profile audio capabilities
The protobuf schema SHALL define an `AudioProfileCapability` message and a `ProfileEntry`
message pairing a profile enum value with an `AudioProfileCapability`. Named-profile codec
messages SHALL use `repeated ProfileEntry`.

#### Scenario: Server serializes ProfileEntry for each map entry
- **WHEN** the server serializes an `AacCapability` with two profile map entries
- **THEN** the protobuf response SHALL contain two `ProfileEntry` messages, each with the
  correct profile enum value and the corresponding `AudioProfileCapability` fields

#### Scenario: Client deserializes ProfileEntry into map
- **WHEN** the client receives a protobuf response with two AAC `ProfileEntry` messages
- **THEN** `AacCapability.profiles` SHALL be reconstructed as a map with two entries, keys
  matching the received profile enum values
