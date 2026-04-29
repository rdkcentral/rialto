# Proposal: Create SoC Inventory Document

## Problem

The previous change `inventory-soc-related-features` defined the specification for what a SoC inventory should contain. However, the actual grouped inventory document — listing all SoC-related markers found in the Rialto codebase with categories and code locations — does not yet exist.

Engineers have no single reference document to consult when auditing platform dependencies.

## Scope

This change will produce the concrete SoC inventory document based on the existing codebase scan.

The work will include:
- a grouped inventory of all confirmed SoC-related markers in Rialto
- grouping by category: vendor-specific, generic SoC capability, configuration-level, media pipeline
- code locations for each marker

The work will not include:
- new runtime code changes
- support for new SoC platforms

## Outcome

A human-readable inventory document at `docs/soc-inventory.md` that can be used for platform audits, onboarding, and future change planning.

## Capabilities

### Capability: soc-inventory-document

This capability covers the creation and maintenance of the concrete SoC feature inventory document in the Rialto repository.
