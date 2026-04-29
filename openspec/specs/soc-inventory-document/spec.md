# soc-inventory-document Specification

## Purpose
TBD - created by archiving change create-soc-inventory-document. Update Purpose after archive.
## Requirements
### Requirement: Grouped SoC keyword inventory document exists
The repository SHALL contain a human-readable document at `docs/soc-inventory.md` that lists all SoC-related markers discovered in the Rialto codebase, grouped by category.

#### Scenario: Document present in docs folder
- **WHEN** an engineer navigates to `docs/soc-inventory.md`
- **THEN** the file SHALL exist and contain at least one grouped section of SoC markers

### Requirement: Vendor-specific markers are listed
The inventory document SHALL include a section for vendor-specific SoC markers (e.g. Amlogic, RTK, BCM) with the keyword, file path, and a brief description.

#### Scenario: Amlogic marker listed
- **WHEN** the inventory document is opened
- **THEN** the vendor-specific section SHALL contain `amlhalasink` with its source file paths and description

### Requirement: Generic SoC capability markers are listed
The inventory document SHALL include a section for generic SoC capability checks not tied to a single vendor.

#### Scenario: isSocAudioFadeSupported listed
- **WHEN** the inventory document is opened
- **THEN** the generic SoC section SHALL list `isSocAudioFadeSupported` and `doAudioEasingonSoc` with their source file paths

### Requirement: Configuration-level markers are listed
The inventory document SHALL include a section for configuration-level SoC markers, including environment variables and config file references.

#### Scenario: rialto-soc.json config reference listed
- **WHEN** the inventory document is opened
- **THEN** the configuration section SHALL list `RIALTO_CONFIG_SOC_PATH` and `rialto-soc.json` with their source file paths

