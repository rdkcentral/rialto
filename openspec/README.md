# OpenSpec Usage Guide

This guide explains how to use OpenSpec on the Rialto project to plan, track, and archive changes in a structured, spec-driven way.

---

## What OpenSpec Does

OpenSpec manages the lifecycle of a change through a fixed set of artifacts:

| Artifact | Purpose |
|----------|---------|
| `proposal.md` | Describes the problem and what will change |
| `specs/` | Defines WHAT the system must do (requirements + scenarios) |
| `design.md` | Explains HOW the change will be implemented |
| `tasks.md` | Checklist of implementation work to complete |

When a change is archived, its specs are promoted into `openspec/specs/` as the permanent record.

**OpenSpec does not write content for you, execute tasks, or scan the codebase. That is the engineer's responsibility.**

---

## Prerequisites

- OpenSpec CLI installed and on your PATH
- All commands must be run from the **repo root**:

```powershell
cd "c:\path\to\rialto"
```

---

## Step-by-Step Workflow

### 1. Create a new change

```powershell
openspec new change "<change-name>"
```

Use a short kebab-case name describing the work, e.g. `add-drm-capability-audit`.

This creates:
```
openspec/changes/<change-name>/
    proposal.md
```

---

### 2. Check what needs to be done next

```powershell
openspec status --change "<change-name>"
```

Example output:
```
Progress: 1/4 artifacts complete
[x] proposal
[ ] design
[ ] specs
[-] tasks (blocked by: design, specs)
```

---

### 3. Get instructions for each artifact

```powershell
openspec instructions --change "<change-name>" proposal
openspec instructions --change "<change-name>" specs
openspec instructions --change "<change-name>" design
openspec instructions --change "<change-name>" tasks
```

Each command prints:
- What the artifact must contain
- Required format rules
- The exact file path to write to
- A template to start from

**Read the instructions before filling in each artifact.**

---

### 4. Fill in the artifacts

Write the content yourself based on the instructions output. Key rules:

**specs:**
- One `spec.md` per capability: `specs/<capability-name>/spec.md`
- Requirements use `### Requirement:` headers
- Scenarios use exactly `####` (4 hashtags): `#### Scenario:`
- Use WHEN / THEN format for scenarios
- Every requirement needs at least one scenario

**tasks:**
- Use checkbox format: `- [ ] 1.1 Task description`
- Group tasks under `## 1. Group Name` headings
- Mark tasks complete with `- [x]` as you finish them

---

### 5. Validate before archiving

```powershell
openspec validate
```

This checks all changes and specs for structural correctness. Fix any reported errors before proceeding.

---

### 6. Mark all tasks complete

Edit your `tasks.md` and change `- [ ]` to `- [x]` for every completed task. OpenSpec will block archiving if any tasks remain unchecked.

---

### 7. Archive the change

```powershell
openspec archive "<change-name>"
```

OpenSpec will:
1. Check all tasks are complete
2. Promote specs into `openspec/specs/`
3. Move the change to `openspec/changes/archive/YYYY-MM-DD-<change-name>/`

---

## Directory Layout

```
openspec/
    README.md               ← this file
    changes/
        <change-name>/      ← active changes (in-progress)
        archive/            ← completed changes
    specs/                  ← promoted specs (permanent record)
```

---

## Common Mistakes

| Mistake | Fix |
|---------|-----|
| Running `openspec` from inside a change folder | Always run from repo root |
| Using `###` for scenarios instead of `####` | Scenarios require exactly 4 hashtags |
| Archiving with incomplete tasks | Mark all tasks `[x]` first |
| `openspec validate --change ...` | Not supported; use plain `openspec validate` |
| Forgetting `openspec instructions` | Always read instructions before writing an artifact |

---

## Example: Rialto SoC Audit (Reference)

Two completed change examples are available in this repo:

- `openspec/changes/archive/2026-04-29-inventory-soc-related-features/` — defined the audit requirements
- `openspec/changes/archive/2026-04-29-create-soc-inventory-document/` — produced the inventory document

Resulting specs:
- `openspec/specs/soc-feature-inventory/spec.md`
- `openspec/specs/soc-inventory-document/spec.md`

Output artifact:
- `docs/soc-inventory.md`

These serve as working examples of the full proposal → specs → design → tasks → archive flow.
