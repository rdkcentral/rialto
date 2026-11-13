## 1. Reference-player metric expectation
- [ ] 1.1 Implement `notifyMetric` in the reference-player pipeline callback; record received metric events safely across callback and CLI threads, and cover callback recording with focused tests.
- [ ] 1.2 Add `--expect-metric <type>` parsing and help text for the four supported video metric types; reject unknown types and require a media URL when the option is used.
- [ ] 1.3 Arm the expected-type wait before playback/injection, use the existing bounded CLI timeout, and return a failing exit status with a useful diagnostic if no matching type arrives.
- [ ] 1.4 Test matching, missing, unrelated-type, and invalid-argument cases, including callbacks arriving asynchronously.

## 2. Reference-player fault stimuli
- [ ] 2.1 Add bounded feeder controls for `VIDEO_FRAME_DROP`, `VIDEO_DECODE_ERROR`, `VIDEO_FRAME_REPEAT`, and `BUFFER_UNDERFLOW`; verify normal playback is unchanged when no stimulus is selected.
- [ ] 2.2 Implement late-frame injection by delaying, not omitting, a valid video sample past its presentation deadline; verify the sample is still sent to Rialto.
- [ ] 2.3 Implement decode-error injection by corrupting one selected video access unit; verify playback termination is reported separately from an expected-metric timeout.
- [ ] 2.4 Implement frame-repeat and underflow stimuli by separately stalling the next frame and pausing feeder delivery during active playback; verify underflow is not inferred from need-data or `notifyBufferUnderflow()` callbacks.
- [ ] 2.5 Resolve “a single repeat still produces a pair” before asserting event count, phase, or payload cardinality; use the existing metric contract for emitted event fields.

## 3. End-to-end verification
- [ ] 3.1 Add reference-player CLI support for `--inject-fault <type>` and require an active media pipeline; document that the stimulus does not guarantee a metric on unsupported targets.
- [ ] 3.2 Run each supported stimulus with a matching `--expect-metric` against a real Rialto server/client and verify the resulting event reaches `notifyMetric` before the timeout.
- [ ] 3.3 Verify a deliberately mismatched expected type fails, a matching type passes, and legacy `notifyBufferUnderflow()`/QoS behavior remains unchanged.
- [ ] 3.4 Run the reference-player host tests and targeted Rialto server/client unit or component suites; record which target/device integration checks are required when host CI cannot exercise the real IPC path.