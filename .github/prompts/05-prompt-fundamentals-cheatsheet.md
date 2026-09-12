# Prompt Fundamentals Cheat Sheet

Use this as a quick reference when writing structured Copilot prompts.

## Core Rule

1. Structure first.
2. Evidence second.
3. Final output last.

## Golden Principles

1. Create structure first, then fill it.
2. Use a working document to accumulate findings across phases.
3. Enforce sequential phases with mandatory validation before moving on.

## Prompt Skeleton

1. YAML frontmatter
2. Title and scope
3. Hard constraints
4. Context and purpose
5. Output requirements
6. Detailed numbered phases
7. Completion checklist

## Strong Wording Patterns

1. Use MUST for required behavior.
2. Use MUST NOT for prohibited behavior.
3. Put critical gates in warning lines.
4. Define done with measurable checks.

## Phase Template

```markdown
## PHASE X: Name

⚠️ MANDATORY: Complete Phase X-1 before this phase.

Objective:
- What this phase must achieve

Extract:
- Data point 1
- Data point 2

Process:
1. Step one
2. Step two
3. Step three

Update working document:
- Exact section to update
- Exact format to use

Validation before proceeding:
- Condition 1 is true
- Condition 2 is true
- Section status is Completed
```

## Minimum Constraints Block

```markdown
Constraints:
- You MUST complete phases in order.
- You MUST update the working document after every phase.
- You MUST NOT modify application code.
- This task is analysis-only unless explicitly stated otherwise.
- Final output must follow the specified template exactly.
```

## Anti-Pattern Prevention

1. Jumping to conclusions:
- Add mandatory research phases before output phase.
2. Skipping sections:
- Track section status as Not Started, In Progress, Completed.
3. Vague output:
- Require concrete fields: file paths, severity, evidence, examples.
4. Scope creep:
- State boundaries explicitly: analysis only, no code changes.

## Quality Gate Checklist

1. Clear scope and non-scope
2. Explicit output format
3. Sequential phases with dependencies
4. Validation checks in every phase
5. Positive and negative instructions
6. Concrete examples included
7. Completion criteria measurable
8. Repository source-of-truth references listed (code paths and architecture docs)
9. Marker variants included when logs differ from expected canonical phrasing

## Copy-Paste Starter

```markdown
You are performing a structured analysis task.

## Constraints
- You MUST complete all phases in sequence.
- You MUST update the working document after each phase.
- You MUST NOT change source code.
- Do not produce final recommendations until all research phases are complete.

## Working document
Create ./tmp/task-inventory.md with sections:
1. Section A (Not Started)
2. Section B (Not Started)
3. Section C (Not Started)

Status flow:
Not Started -> In Progress -> Completed

## PHASE 0: Setup
Create the working document exactly as specified.
Validation:
- File exists
- All sections present

## PHASE 1: Analyze A
...
Validation:
- Section A marked Completed
- At least 3 concrete findings

## PHASE 2: Analyze B
...
Validation:
- Section B marked Completed
- At least 3 concrete findings

## PHASE 3: Analyze C
...
Validation:
- Section C marked Completed
- At least 3 concrete findings

## PHASE 4: Consolidate
Merge findings into prioritized summary.
Validation:
- No empty sections
- Duplicates removed

## PHASE 5: Final output
Produce final report in required format only.

## Completion checklist
- All sections Completed
- All validations passed
- Output format exactly matched
```