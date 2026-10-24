## Context

See proposal.md for the business motivation and scope. The implementation constraint is that the metrics relay must stay a thin pass-through: GStreamer bus messages are consumed by a dedicated server-side metrics component, cached for query access, and forwarded by existing IPC and client callback paths with no new public event contract.

This design is focused on the server-side architecture needed to turn bus-level metrics into a stable, testable query/push flow that matches the existing media pipeline APIs.

## Goals / Non-Goals

**Goals:**
- Add a dedicated `RialtoMetrics` component that owns metric parsing, caching, and forwarding.
- Register a GStreamer bus watch and parse `media-pipeline-metric` custom messages without disrupting playback.
- Provide a query path keyed by `(sessionId, sourceId)` for current frame counters.
- Provide a push path that forwards the same counter snapshot to clients using the existing IPC callback flow.
- Keep the public contract compatible with the current first-frame and media pipeline event model.

**Non-Goals:**
- Subscription management or client-side filtering in Phase 1.
- Message buffering, aggregation, or historical time-series storage.
- Changing the format of upstream `media-pipeline-metric` messages.
- Adding a new public metrics API beyond the existing query/callback path.

## Decisions

### D1: Put all metric ownership in `RialtoMetrics`

`GstGenericPlayer` remains a coordinator, not a metrics implementation site. It owns the bus reference and creates the metrics component, but the parsing, cache, and forwarding logic live in `RialtoMetrics`. This isolates the change, keeps `MediaPipelineServerInternal` thin, and makes the bus callback lifetime explicit.

Alternatives considered:
- Put counter storage in `MediaPipelineServerInternal`: rejected because it mixes IPC/service concerns with bus callback ownership and would spread lifecycle management across multiple classes.
- Split logic between the player and server internals: rejected because it creates hidden coupling and makes thread-safety harder to reason about.

### D2: Use a single thread-safe cache keyed by `(sessionId, sourceId)`

The cache stores the last known snapshot for each source and exposes a simple query API that returns the current counters and timestamp. This is enough for Phase 1 query semantics and the push callback flow, while remaining cheap to maintain under GStreamer bus activity.

Alternatives considered:
- Store the data in multiple local maps across player/server layers: rejected because it creates inconsistent state and duplicate ownership.
- Add multi-tier aggregation with buffering: rejected because it exceeds the Phase 1 requirement and adds unnecessary latency.

### D3: Keep the IPC contract as a pass-through, not a new event model

The protobuf additions should carry the same counter snapshot fields already used by the metrics producer and client consumer. The server sends the same counter snapshot as a query response and an async event, without introducing a new event type or public callback contract.

Alternatives considered:
- New subscription-based event model: rejected because it adds lifecycle and state-tracking complexity not needed for the initial relay.
- Add a new public metrics callback on the client API: rejected because it would duplicate the existing pipeline event contract.

### D4: Preserve backward compatibility by making the new flow additive

The change introduces a new metrics relay path while keeping the existing media pipeline, QoS, and stats logic untouched. Existing flows remain authoritative for non-metrics behavior, and the new interfaces are only activated where the metrics path is required.

Alternatives considered:
- Replace the existing stats path with metrics: rejected because it changes behavior for unrelated consumers and increases regression risk.

## Risks / Trade-offs

- [Risk] Bus callback lifetime and teardown ordering could create use-after-free or stale watches. Mitigation: `RialtoMetrics` owns the watch registration and removal, and `GstGenericPlayer` ensures destruction occurs after the bus is no longer active.
- [Risk] Concurrent bus callback and query access can race on the shared cache. Mitigation: guard the cache with a mutex or equivalent lock and ensure all read/write paths go through the same component.
- [Risk] IPC and GStreamer timing skew could make the snapshot appear stale. Mitigation: timestamp the snapshot when it is received and keep the field semantics explicit in the protobuf contract.
- [Risk] A broad Phase 1 relay could expose noisy metrics to all clients. Mitigation: document that subscription filtering is deferred and make the current design intentionally simple and pass-through.

## Migration Plan

1. Add the `RialtoMetrics` module and build target.
2. Register the GStreamer bus watch and parse incoming custom metric messages.
3. Add the thread-safe counter cache and query accessor used by the server.
4. Extend protobuf and IPC service/client code for the query and push flow.
5. Wire `GstGenericPlayer` and `MediaPipelineServerInternal` to the metrics component.
6. Add unit and integration tests for cache behavior, IPC serialization, and end-to-end signaling.
7. Validate the relay path and confirm the existing non-metrics pipeline remains unchanged.

Rollback: remove the metrics module from the player setup path, leave bus watch registration absent, and restore the prior server/client behavior. Because the changes are additive, the fallback path is the pre-existing media pipeline behavior.

## Open Questions

None at this stage. The design resolves the implementation path without requiring new product decisions beyond the Phase 1 pass-through scope already defined in the proposal.
