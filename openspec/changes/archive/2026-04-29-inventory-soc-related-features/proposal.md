Set-Content "c:\\Users\\bn480\\OneDrive - Comcast\\Desktop\\rialto\_sdd\\rialto\\openspec\\changes\\inventory-soc-related-features\\proposal.md" -Value (

@"

\# Proposal: Inventory SoC-Related Features



\## Problem



Rialto does not currently have a clear, structured inventory of SoC-related features, vendor-specific code paths, and platform-dependent behavior. This makes it harder to identify where the codebase depends on chipset-specific functionality such as Amlogic,AMLHAL,RTK,BCM and it increases the effort required for platform audits, bring-up, maintenance, and future porting work.



At present, SoC-related behavior appears to be distributed across media, wrapper, and configuration code, but there is no single place that documents the relevant keywords, implementation points, or feature groupings. As a result, engineers must rely on ad hoc searches and tribal knowledge to understand platform dependencies.Most of the implementation is present in element set up folder



\## Scope



This change will identify and document SoC-related features and platform-specific markers in the Rialto repository.



The work will include:

\- vendor-specific markers such as Amlogic and any other discovered SoC or chipset identifiers

\- generic SoC capability hooks and helper APIs

\- SoC-specific media sink or plugin detection

\- SoC-dependent configuration paths and configuration loading points

\- a searchable keyword inventory that can be reused for future audits



The work will not include:

\- implementation changes to runtime behavior

\- refactoring of existing production code

\- adding support for new SoCs or vendors

\- changing current platform selection logic



\## Outcome



The result of this change will be a documented inventory of SoC-related features, keywords, and code locations in Rialto.



This inventory should:

\- make SoC-specific behavior easier to discover

\- help engineers distinguish vendor-specific behavior from generic functionality

\- support future design work, platform reviews, and follow-up OpenSpec changes

\- provide a repeatable basis for extending the audit to additional vendor markers such as RTK, BCM,AML and similar platform identifiers



\## Expected Benefits



\- Reduced effort when reviewing platform dependencies

\- Better visibility into vendor-bound code paths

\- Easier future maintenance and onboarding

\- Clearer basis for follow-up changes related to SoC abstraction or platform support


## Capabilities

### Capability: soc-feature-inventory

This capability defines how Rialto identifies, groups, and documents SoC-related features, vendor-specific markers, and platform-dependent code paths.

It covers:
- vendor-specific identifiers such as Amlogic and other discovered SoC markers
- generic SoC capability hooks
- SoC-specific configuration paths
- media sink or plugin names that indicate platform-dependent behavior
- code locations associated with each identified marker

"@

)

