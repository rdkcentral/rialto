## Context

`GstGenericPlayer` teardown can race with a periodic playback-info timer during rapid channel switching. If timer callbacks run while pipeline state is being destroyed, callback logic may observe invalid or partially released resources. The proposal defines a teardown-focused fix and corresponding unit-test precision requirements.

Current gaps:
- `termPipeline()` teardown path does not guarantee playback-info timer cancellation before pipeline resources are released.
- Unit tests rely on broad timer expectations in shared helpers, which can hide missing cancellation behavior.

Constraints:
- No public API, IPC, protobuf, or payload changes.
- Behavior changes must be internal to gstplayer teardown.
- Existing normal playback behavior must remain unchanged.

## Goals / Non-Goals

**Goals:**
- Ensure `termPipeline()` stops playback-info timer activity before teardown completes.
- Prevent playback-info callbacks from running against teardown-in-progress state.
- Make unit tests explicit and scenario-specific for timer creation and cancellation.
- Validate the crash-fix path with an active timer during teardown.

**Non-Goals:**
- Changing playback-info frequency or payload format.
- Refactoring unrelated timer lifecycles outside playback-info teardown coverage.
- Introducing new external interfaces.

## Decisions

1. Add explicit playback-info timer shutdown in `termPipeline()`
- Decision: Ensure teardown stops playback-info timer activity by calling `stopNotifyPlaybackInfoTimer()` during teardown flow.
- Rationale: The observed crash condition is teardown-specific; cancellation must be deterministic and close to teardown orchestration.
- Alternative considered: Rely on implicit timer destruction via object lifetime only.
- Why not: Implicit destruction is order-dependent and can still allow callback execution in a teardown window.

2. Enforce ordering: timer cancellation before teardown completion
- Decision: Keep helper-driven timer shutdown ordered before teardown completion so active timers are canceled before pipeline finalization.
- Rationale: Explicit order removes ambiguity about callback survivability during teardown.
- Alternative considered: Cancel later after broader resource cleanup.
- Why not: Late cancellation still permits callback overlap with state destruction.

3. Make test expectations local to behavior-specific tests
- Decision: Remove unconstrained timer factory expectations from shared teardown helpers and move expectations into relevant tests/helpers.
- Rationale: Broad expectations (`Times(AnyNumber())`) reduce test precision and can mask missing lifecycle behavior.
- Alternative considered: Keep shared broad expectations and add only one extra teardown test.
- Why not: Existing brittleness remains and can regress silently.

4. Validate real bug path with active timer
- Decision: At least one teardown test must model active timer (`isActive() == true`) and expect `cancel()`.
- Rationale: This directly verifies the intended crash-fix behavior.
- Alternative considered: Validate only inactive timer teardown paths.
- Why not: Inactive path bypasses cancellation and does not prove the fix.

## Risks / Trade-offs

- [Risk] Teardown ordering change may affect tests that assumed prior call sequencing.
  → Mitigation: Update expectations in impacted tests and keep behavior assertions scoped to required interactions.

- [Risk] Additional `isActive()`/`cancel()` interactions could be over-asserted and make tests flaky.
  → Mitigation: Use precise expectations only in relevant tests and avoid unconstrained shared mocks.

- [Risk] Potential duplicated cancellation logic if stop paths already cancel timers elsewhere.
  → Mitigation: Keep cancellation idempotent and guard by timer existence/activity.

- [Trade-off] More explicit tests increase maintenance effort.
  → Mitigation: Encapsulate repeated setup in focused helper methods with clear intent (creation vs cancellation).
