# Tasks

## 1. Metrics module foundation

- [ ] 1.1 Create the `RialtoMetrics` module skeleton and CMake wiring, and verify the new target builds with the existing media server configuration.
- [ ] 1.2 Implement the bus watch lifecycle and safe teardown logic, and verify the callback is removed during destruction without crashes or warnings.
- [ ] 1.3 Add the thread-safe counter cache keyed by `(sessionId, sourceId)`, and verify query access returns the latest snapshot without races.

## 2. Server-side relay integration

- [ ] 2.1 Wire `GstGenericPlayer` to instantiate and initialize `RialtoMetrics`, and verify the player setup path still initializes the bus correctly.
- [ ] 2.2 Add `MediaPipelineServerInternal` query delegation to the new metrics cache, and verify the returned snapshot values are correct for known source IDs.
- [ ] 2.3 Add protocol-level pass-through handling for query and push events, and verify the response payload matches the expected counter fields and timestamp semantics.

## 3. IPC and client integration

- [ ] 3.1 Extend the IPC service/client stubs for `GetFrameCounters` and `NotifyMetricEvent`, and verify requests and notifications serialize and deserialize successfully.
- [ ] 3.2 Register the async client callback for push events, and verify a server-side metric event reaches the application callback without additional filtering.
- [ ] 3.3 Document the server/client contract and usage expectations, and verify the docs match the implemented request and callback shapes.

## 4. Verification and regression coverage

- [ ] 4.1 Add unit tests for cache correctness, monotonic counters, and bus-message parsing, and verify these tests pass with no regression in the existing metrics-related logic.
- [ ] 4.2 Add integration tests covering end-to-end server relay behavior, and verify metric events propagate from bus callback to client callback without altering unrelated pipeline flows.
- [ ] 4.3 Run the targeted build and test commands for the touched components, and verify the metrics change passes while the existing media pipeline behaviors remain stable.
