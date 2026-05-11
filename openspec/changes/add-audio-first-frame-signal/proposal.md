# Proposal: Add First Audio Frame Notification Support in Rialto

## Summary
Introduce support in Rialto for notifying clients when the **first audio frame** has been received by the playback pipeline.

This proposal adds an end-to-end notification path from the GStreamer playback layer through Rialto server and IPC layers to the public client-facing API.

The implementation should remain **platform-agnostic**. Rialto should:
- search for a supported first-audio-frame indication such as `first-audio-frame` or `first-audio-frame-callback`
- use it when present on the relevant element
- otherwise fall back to a temporary static probe on the sink pad
- generate the first-audio-frame event when the first audio frame is observed
- remove the probe immediately after the first detection

---

## Problem Statement
Today, Rialto exposes several playback-related events such as buffering, underflow, errors, and playback state changes. However, there is no dedicated event that informs the client when the **first audio frame** has been received.

This creates a gap for use cases that need to distinguish between:
- playback being started internally,
- data being fed into the pipeline,
- and audio becoming available to the end user.

Without a first-frame notification:
- application startup metrics are less accurate,
- audio readiness cannot be tracked reliably,
- observability into audio startup behavior is limited,
- diagnostics for silent-start scenarios are harder.

---

## Goals
The proposed change should:

1. Detect when the first audio frame is reported or observed in the underlying playback pipeline.
2. Use a platform-agnostic detection strategy based on supported signal/action names rather than platform-specific implementation branches.
3. Fall back to pad-probe-based detection where no suitable first-frame indication exists.
4. Propagate the event through the Rialto server architecture.
5. Transport the event across IPC from server to client.
6. Expose the event through a client-facing Rialto interface.
7. Preserve Rialto’s existing event propagation patterns and threading model.
8. Be testable through unit, component, and end-to-end validation.

---

## Non-Goals
This proposal does not aim to:
- define QoS or startup latency policy,
- measure exact render timestamps at platform level,
- change playback state machine semantics,
- introduce first-frame handling for non-audio media unless explicitly extended later,
- redesign existing notification infrastructure,
- embed SoC-specific logic directly into Rialto implementation paths.

---

## Motivation
A dedicated first-frame notification is useful for several reasons:

- **User experience:** applications can know when audio has actually started.
- **Telemetry:** startup-time measurements can use first-frame as a meaningful milestone.
- **Debugging:** teams can better isolate cases where playback begins but audible output is delayed.
- **Consistency:** Rialto can provide a unified first-audio-frame notification even when underlying platforms expose different mechanisms.

This feature is especially relevant in systems where “playback started” does not imply that audio is already audible.

---

## Proposed Design

### High-Level Design
Add a new first-frame event flow with the following stages:

1. Detect the first audio frame in the GStreamer player layer using a platform-agnostic strategy.
2. Convert the detection into a scheduled internal Rialto task.
3. Notify the server-side media pipeline/client layer.
4. Send a new IPC/protobuf event to the client process.
5. Receive and handle that event in the client IPC layer.
6. Notify the public Rialto client interface.

---

## Proposed Detection Strategy

The detection mechanism should be **capability-based**, not platform-based.

### Preferred detection method
During element setup, Rialto should inspect the relevant audio element for supported first-frame indications such as:

- `first-audio-frame`
- `first-audio-frame-callback`

If one of these supported indications is available, Rialto should connect/use it appropriately.

### Fallback detection method
If no supported first-frame indication is available:

- install a static probe on the sink pad,
- wait for the first valid audio frame/buffer to pass through the pad,
- generate the internal first-audio-frame event,
- remove the probe immediately after the first frame is observed.

This allows Rialto to provide a consistent first-audio-frame notification even when the backend does not expose a dedicated first-frame indication.

### Background context
Some SoC implementations may expose first-audio-frame behavior differently. For example:
- `first-audio-frame` may be exposed as an action on some sinks
- `first-audio-frame-callback` may be exposed as a signal on some decoders

These are useful examples for requirements and validation, but the Rialto implementation should not branch on SoC or vendor identity.

---

## Proposed Event Flow

### Step 1: Detect first frame in the playback backend
The GStreamer integration layer should detect the first audio frame using one of the following mechanisms:

- a supported first-frame indication such as `first-audio-frame`
- a supported first-frame indication such as `first-audio-frame-callback`
- a fallback static sink-pad probe if no supported indication exists

The first available valid mechanism should be used.

### Step 2: Schedule internal handling
The callback/action/probe path should not perform heavy work directly on the GStreamer thread.

Instead, it should schedule an internal task, for example:
- `FirstFrameReceived`

This keeps behavior aligned with existing Rialto task-based event handling patterns.

### Step 3: Notify the server pipeline
The scheduled task should inform the server-side media pipeline or player client that the first frame for a given media source has been received.

### Step 4: Send IPC event
The server IPC layer should construct and send a new event message, for example:
- `FirstFrameReceivedEvent`

The event should contain enough context to identify:
- playback session
- associated media source

### Step 5: Receive event on client side
The client IPC layer should subscribe to the new event and dispatch it to the client-side media pipeline.

### Step 6: Notify public client API
The client-facing Rialto interface should expose a notification method such as:
- `notifyFirstFrameReceived(int32_t sourceId)`

This allows applications or upper layers to respond when the first audio frame becomes available.

---

## Proposed API Changes

### Public Client Interface
Add a new callback to the public media pipeline client interface:

```cpp
virtual void notifyFirstFrameReceived(int32_t sourceId) = 0;
```

### Client IPC Interface
Add support for forwarding the same notification through the client IPC abstraction.

### Server Interfaces
Add corresponding notification methods in server-facing interfaces so the event can propagate consistently across layers.

---

## Proposed Protocol Changes

Add a new protobuf event, for example:

```proto
message FirstFrameReceivedEvent {
    optional int32 session_id = 1 [default = -1];
    optional int32 source_id = 2 [default = -1];
}
```

### Rationale
- `session_id` identifies the active playback session.
- `source_id` identifies which attached source produced the first frame.

This matches existing Rialto event patterns and keeps the event transport lightweight.

---

## Architectural Considerations

### Threading
The first-frame indication may originate from:
- a GStreamer signal,
- a GStreamer action/callback path,
- or a sink pad probe.

In all cases, handling should be deferred to Rialto’s worker/task model rather than processed inline.

### Layering
The design should preserve the current Rialto layering:
- backend/player event source
- server internal handling
- IPC transport
- client IPC handling
- public client notification

### Backward Compatibility
This proposal is additive:
- existing clients remain unaffected unless they implement or consume the new callback,
- existing event behavior is unchanged,
- no existing API needs to be removed.

### Source Scope
Initial scope should focus on:
- **audio first-frame notification**

Video-first-frame support can be considered later if there is a clear requirement.

### Probe Lifecycle
For probe-based detection:
- the probe must be installed only when needed,
- the probe must trigger only on the first valid audio frame,
- the probe must be removed immediately after the first-frame event is generated.

This avoids unnecessary runtime overhead and duplicate notifications.

### Platform Agnosticism
Rialto should not:
- check vendor/platform names,
- branch on SoC-specific identifiers,
- hardcode Broadcom/Realtek-specific implementation paths.

Rialto should instead:
- inspect element capabilities,
- search for supported first-frame indication names,
- use those indications if available,
- otherwise use the generic fallback probe method.

---

## Alternatives Considered

### 1. Reuse playback state changes
A playback-state event could be interpreted as “media started.”

**Why not preferred:**  
Playback state transitions do not necessarily mean the first audio frame is audible/received. This loses precision.

### 2. Add platform-specific implementation branches
Rialto could explicitly branch for different SoCs or vendors.

**Why not preferred:**  
This makes the implementation less maintainable, less reusable, and inconsistent with a platform-agnostic middleware design.

### 3. Require all backends to expose a single standard callback
This would simplify implementation and make detection uniform.

**Why not preferred:**  
Existing backends expose different mechanisms, and some may not provide a dedicated first-audio-frame indication at all.

### 4. Emit the event directly from the callback/probe thread
This would simplify implementation.

**Why not preferred:**  
It would violate existing Rialto patterns and may create thread-safety risks.

---

## Risks and Mitigations

| Risk | Mitigation |
|---|---|
| Supported first-frame mechanisms differ across backend elements | Use a capability-based search for supported indication names such as `first-audio-frame` and `first-audio-frame-callback` instead of branching by platform/vendor. |
| Some sinks/elements do not expose any supported first-frame indication | Install a static probe on the sink pad, generate the event when the first valid audio frame is observed, then remove the probe immediately. |
| Ambiguity around “received” semantics across different backends | Document the feature as a first-audio-frame notification reported or inferred by the backend pipeline, and define expected semantics clearly for consumers. |
| Duplicate first-frame notifications | Ensure the event is emitted only once per relevant source/session. Remove the probe after the first event and guard against repeated callback/signal delivery if needed. |
| Notification ordering with other playback events | Follow Rialto’s existing worker-thread and main-thread scheduling model so ordering remains consistent with current event propagation patterns. |
| Interface expansion across many layers | Keep naming and propagation patterns consistent, and add focused unit/component tests for each layer. |
| Pad probe introduces runtime overhead | Use the probe only as a fallback when no supported indication is available, and remove it immediately after detecting the first audio frame. |
| Pad probe may trigger on non-meaningful buffers or unexpected pad activity | Restrict the probe to the relevant sink pad and define clear criteria for what constitutes the first valid audio frame/buffer before emitting the event. |
| SoC-specific knowledge leaks into the implementation | Keep SoC examples in documentation/proposal context only; implementation must depend on detected capabilities and supported indication names, not vendor identity. |

---

## Testing Strategy

### Unit Tests
Add tests to validate:
- discovery of supported indication names such as `first-audio-frame` and `first-audio-frame-callback`,
- correct hookup/usage when a supported indication exists,
- fallback pad probe installation when no supported indication exists,
- probe removal after first detection,
- task creation and scheduling,
- notification forwarding across interfaces,
- IPC subscription and dispatch behavior.

### Component Tests
Add coverage for:
- server-to-client propagation of the new event,
- source/session mapping correctness,
- supported-indication detection behavior,
- fallback probe-based detection behavior,
- interaction with real or stubbed IPC pathways.

### Integration / Fullstack Tests
Validate:
- first audio frame from actual playback triggers the event,
- indication-based detection works when supported by the backend element,
- fallback probe path works on sinks without supported first-frame indications,
- client receives notification during normal audio startup.

### Validation Notes
SoC/platform-specific environments may still be used during validation to confirm that:
- elements exposing `first-audio-frame` behave correctly,
- elements exposing `first-audio-frame-callback` behave correctly,
- elements exposing neither still work through the probe fallback.

These validation cases are useful for confidence but should not affect the generic Rialto implementation approach.

---

## Acceptance Criteria
This proposal is complete when:

1. Rialto searches for supported first-audio-frame indications such as `first-audio-frame` and `first-audio-frame-callback` in a platform-agnostic way.
2. Rialto uses a supported first-frame indication when available on the relevant backend element.
3. Rialto falls back to sink-pad probe detection when no supported indication exists.
4. The fallback probe is removed immediately after the first valid audio frame is detected.
5. A dedicated first-frame event is propagated from server to client.
6. The client-facing interface receives a first-frame notification with the correct source id.
7. Existing playback flows remain unchanged when the feature is unsupported or unused.
8. Unit and component tests cover both supported-indication and probe-based detection paths.
9. Fullstack validation confirms the event is emitted during real playback.
10. The implementation contains no vendor- or SoC-specific branching logic.

---

## Rollout Considerations
This feature can be introduced as an additive enhancement with no required migration for existing clients.

Recommended rollout approach:
1. implement capability-based first-frame indication discovery,
2. implement fallback sink-pad probe detection,
3. validate across supported audio backends,
4. enable telemetry/consumer usage where needed,
5. document indication/probe semantics for integrators.

---

## Open Questions
1. Should the external API be named generically (`notifyFirstFrameReceived`) or audio-specifically (`notifyFirstAudioFrameReceived`)?
2. What exact buffer/condition should qualify as the first valid audio frame in the fallback probe path?
3. Should supported indication names be centralized in a utility/helper for easier future extension?
4. Should probe installation happen for any sink lacking a supported indication, or only for selected classes of elements?
5. Should duplicate first-frame notifications be suppressed explicitly per source/session in all paths?

---

## One-Line Proposal Summary
Add an end-to-end Rialto event that notifies clients when the first audio frame is received, using platform-agnostic discovery of supported first-frame indications and a fallback sink-pad probe when no such indication exists.

