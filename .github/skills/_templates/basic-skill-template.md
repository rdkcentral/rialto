# Basic Skill Template

Copy this file to `.github/skills/<skill-name>/SKILL.md` and replace placeholders.

```markdown
---
name: <skill-name-kebab-case>
description: 'Use when the user wants to <specific workflow>. Trigger words: <keyword1>, <keyword2>, <keyword3>.'
argument-hint: '<Optional: what inputs user should provide>'
# Optional:
# user-invocable: true
# disable-model-invocation: false
---

# <Skill Title>

<One-line purpose statement. Keep it specific and domain-focused.>

## When to Use

Use this skill when:
- <condition 1>
- <condition 2>
- <condition 3>

Do not use this skill when:
- <out-of-scope case 1>
- <out-of-scope case 2>

## Input

Required:
- <input 1>
- <input 2>

Optional:
- <input 3>

If required input is missing, ask the user before continuing.

## Steps

1. **Clarify goal**
- Confirm user objective and success criteria.
- If ambiguous, ask a focused question.
- STOP and wait for user response when needed.

2. **Gather evidence/context**
- Read only the files/logs relevant to the request.
- Capture exact evidence (paths, lines, timestamps when applicable).

3. **Apply workflow logic**
- Execute the domain-specific checks/process.
- Keep a concise working summary of findings and assumptions.

4. **Decision point**
- Present options or findings that need user choice.
- STOP and wait for user direction before irreversible actions.

5. **Produce output**
- Generate the requested artifact/summary in the agreed format.
- Include confidence, gaps, and next action.

## Output

Return:
- <artifact/file path or summary type>
- <required sections>
- <verdict/status format>

## Guardrails

- Do NOT proceed past decision points without user confirmation.
- Do NOT modify files outside <allowed scope>.
- Do NOT claim conclusions without evidence.
- If data is missing or conflicting, mark as `Inconclusive` and explain why.
- If a step fails, report the error and ask how to proceed.
```

## Notes

- Keep `description` concrete and keyword-rich for reliable skill discovery.
- Keep SKILL.md short; split large details into `references/` files.
- Folder name should match the `name` field exactly.
