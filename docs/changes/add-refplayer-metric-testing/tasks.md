## 1. Reference-player metric expectation
- [x] 1.1 Implement `notifyMetric` in the reference-player pipeline callback; record received metric events safely across callback and CLI threads, and cover callback recording with focused tests.
- [x] 1.2 Add `--expect-metric <type>` parsing and help text for the four supported video metric types; reject unknown types and require a media URL when the option is used.
- [x] 1.3 Arm the expected-type wait before playback/injection, use the existing bounded CLI timeout, and return a failing exit status with a useful diagnostic if no matching type arrives.
- [x] 1.4 Test matching, missing, unrelated-type, and invalid-argument cases, including callbacks arriving asynchronously.
- [x] 1.5 Print every received `notifyMetric` event with all `MetricEvent` fields, explicitly showing absent optional values; cover matching and non-matching events and optional-field presence in focused tests.

## 2. Reference-player fault stimuli
- [x] 2.1 Add bounded feeder control configuration for `VIDEO_FRAME_DROP`, `VIDEO_DECODE_ERROR`, `VIDEO_FRAME_REPEAT`, and `BUFFER_UNDERFLOW`; verify normal playback is unchanged when no stimulus is selected. Individual fault actions are covered by 2.2-2.4.
- [x] 2.2 Implement late-frame injection by delaying, not omitting, a valid video sample past its presentation deadline; verify the sample is still sent to Rialto.
- [x] 2.3 Implement decode-error injection by corrupting one selected video access unit; verify playback termination is reported separately from an expected-metric timeout.
- [x] 2.4 Implement frame-repeat and underflow stimuli by separately stalling the next frame and pausing feeder delivery during active playback; verify underflow is not inferred from need-data or `notifyBufferUnderflow()` callbacks.
- [x] 2.5 Treat “a single repeat still produces a pair” as an informal producer observation, not an event-cardinality contract; require at least one `VIDEO_FRAME_REPEAT` callback by type, log its payload, and make no unsupported count or phase assertion.

## 3. End-to-end verification
- [x] 3.1 Add reference-player CLI support for `--inject-fault <type>` and require an active media pipeline; document that the stimulus does not guarantee a metric on unsupported targets.
- [ ] 3.2 Run each supported stimulus with a matching `--expect-metric` against a real Rialto server/client and verify the resulting event reaches `notifyMetric` before the timeout.
- [ ] 3.3 Verify a deliberately mismatched expected type fails, a matching type passes, and legacy `notifyBufferUnderflow()`/QoS behavior remains unchanged.
- [ ] 3.4 Run the reference-player host tests and targeted Rialto server/client unit or component suites; record which target/device integration checks are required when host CI cannot exercise the real IPC path.