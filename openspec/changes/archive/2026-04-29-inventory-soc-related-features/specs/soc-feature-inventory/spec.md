## ADDED Requirements

### Requirement: Identify SoC-specific markers
The system SHALL define a searchable inventory of SoC-related markers used in the Rialto repository.

The inventory MUST include:
- vendor-specific identifiers such as Amlogic (amlhalasink), RTK, BCM, and any other discovered SoC markers
- generic SoC capability hooks such as isSocAudioFadeSupported and doAudioEasingonSoc
- SoC-specific configuration paths such as RIALTO_CONFIG_SOC_PATH and rialto-soc.json
- SoC-dependent media sink or plugin names
- other platform-specific identifiers discovered during the audit

#### Scenario: Repository audit captures vendor and SoC markers
- **WHEN** an audit of the Rialto repository is performed for SoC-related behavior
- **THEN** the resulting inventory includes vendor-specific identifiers, generic SoC hooks, configuration paths, and media sink or plugin markers

### Requirement: Group SoC-related features by category
The system SHALL group identified SoC-related markers by category.

The grouping MUST distinguish between:
- vendor-specific markers such as Amlogic, RTK, BCM
- generic SoC capability markers such as isSocAudioFadeSupported
- configuration-level markers such as RIALTO_CONFIG_SOC_PATH
- media pipeline or sink-level markers such as amlhalasink

#### Scenario: Inventory distinguishes marker categories
- **WHEN** SoC-related markers are recorded in the inventory
- **THEN** each marker is assigned to exactly one category that distinguishes vendor-specific, generic capability, configuration-level, or media pipeline behavior

### Requirement: Record code locations for SoC-related markers
The system SHALL associate each identified SoC-related marker with one or more code locations in the repository.

Each recorded location MUST include:
- the file path
- the relevant function or code context
- a brief description of the platform-dependent behavior

#### Scenario: Inventory links markers to code
- **WHEN** a SoC-related marker is added to the inventory
- **THEN** the inventory records one or more repository file locations where that marker or related behavior appears

### Requirement: Exclude runtime behavior changes
The system MUST limit this change to discovery and documentation of SoC-related features.

The system MUST NOT introduce new runtime behavior, modify platform logic, or add support for new SoCs as part of this change.

#### Scenario: Audit remains documentation-only
- **WHEN** the SoC feature inventory is produced
- **THEN** the change affects documentation and specification artifacts only and does not alter any runtime platform behavior
