## Context

Rialto is a cross-platform media framework that abstracts media pipeline operations across different SoC platforms. The codebase currently contains platform-specific behavior scattered across the media server gstplayer, wrappers, and serverManager configuration layers.

Known SoC-specific behavior exists for Amlogic (via amlhalasink, doAudioEasingonSoc, isSocAudioFadeSupported) and a SoC-specific configuration file (rialto-soc.json loaded via RIALTO_CONFIG_SOC_PATH). There is no single document or specification that maps these dependencies.

This design covers the approach to discovering, categorizing, and recording SoC-related features across the repository for the purpose of documentation and future auditing.

## Goals / Non-Goals

**Goals:**
- Define a consistent keyword set for SoC-related markers in Rialto
- Categorize markers by type: vendor-specific, generic SoC capability, configuration, media pipeline
- Record code locations for each discovered marker
- Produce a structured inventory usable for platform audits and follow-up changes

**Non-Goals:**
- Modifying any runtime code or platform behavior
- Adding new SoC support or vendor abstractions
- Refactoring existing platform-specific code
- Automating future SoC audits beyond this change

## Decisions

### Decision: Use manual keyword search for the initial inventory
**Rationale:** The codebase does not have a runtime SoC registry. The most reliable audit approach is a targeted search using a defined keyword set across the repository source files.

**Alternatives considered:**
- Automated static analysis tool: overkill for a documentation-only change and not currently available in the build pipeline.
- Runtime profiling: not applicable since this is a code structure audit, not a performance audit.

### Decision: Organize inventory by marker category rather than by file
**Rationale:** Grouping by vendor or feature type makes the inventory more reusable for platform bring-up and future porting work than a flat file-by-file listing.

**Alternatives considered:**
- Group by file: easier to produce but harder to use when investigating a specific SoC or feature.

### Decision: Scope the initial inventory to confirmed markers only
**Rationale:** Confirmed markers are those already present in the repository. Future markers for RTK, BCM, QCOM, and MTK should be left as extension points rather than speculative entries.

**Alternatives considered:**
- Include placeholder entries for unconfirmed vendors: adds noise and may mislead future readers.

## Risks / Trade-offs

- [Risk: Inventory becomes stale as code evolves] → Mitigation: Reference specific file paths and line context so future engineers can quickly verify or update entries.
- [Risk: Markers are missed if keyword set is too narrow] → Mitigation: Document the keyword set used so future audits can extend it systematically.
- [Risk: Proposal's scope creeps into runtime changes] → Mitigation: Enforce the Non-Goals boundary in spec and tasks artifacts.

## Migration Plan

No runtime deployment or rollback is required. This change produces documentation artifacts only.

Steps:
1. Complete keyword search using the defined set.
2. Populate the inventory with confirmed markers, categories, and code locations.
3. Archive the change via `openspec archive`.
4. The inventory becomes available in `openspec/specs/soc-feature-inventory/` for follow-up changes.

## Open Questions

- Should the inventory include test file references alongside production code references, or production code only?
- Should future SoC vendors such as RTK and BCM be tracked as separate capabilities in follow-up changes, or extended within this capability?
