## Context

The previous change `inventory-soc-related-features` defined requirements for what a SoC inventory should contain. A codebase scan has already been performed and SoC markers identified across media, serverManager, and wrappers components.

This design describes how the inventory document will be structured and where it will live.

## Goals / Non-Goals

**Goals:**
- Produce `docs/soc-inventory.md` with all discovered SoC markers grouped by category
- Make marker locations traceable to source files

**Non-Goals:**
- Automated scanning tooling
- Runtime changes or new SoC platform support
- CI enforcement of the document

## Decisions

**Document location: `docs/soc-inventory.md`**
The existing `docs/` folder already contains `index.html` and `IpcOverview.md`, making it the natural home for reference documentation. Alternatives like `README.md` appending or a separate `soc/` folder were rejected for discoverability reasons.

**Grouping categories:**
Four categories cover all found markers without overlap:
1. Vendor-Specific — markers naming a specific chip vendor (Amlogic)
2. Generic SoC Capability — abstract SoC feature checks (fade, easing)
3. Configuration-Level — env vars and JSON config references
4. Media Pipeline — SoC hooks inside GST pipeline code

## Risks / Trade-offs

- [Risk] Document becomes stale as code evolves → Mitigation: link to `inventory-soc-related-features` spec which defines update requirements
- [Risk] Missed markers from incomplete scan → Mitigation: document is versioned in git; engineers can add entries via PR

## Migration Plan

No migration needed. This is a documentation-only addition.
Write `docs/soc-inventory.md` → open PR → merge.
