## 1. Build and Dependency Setup

- [ ] 1.1 Wire `rdk_perf.h` include paths for supported server targets.
- [ ] 1.2 Verify profiling sources use direct RDKPerf dependency without conditional include fallback.

## 2. Profiling Facade and Runtime Configuration

- [ ] 2.1 Introduce a Rialto profiling facade that wraps RDKPerf scope creation and report APIs.
- [ ] 2.2 Implement thread-safe runtime configuration state sourced from /etc/sky/rialto-config.json and consumed in RialtoServer via propagated environment variables (enabled flag, scope mode, and interface allowlist).
- [ ] 2.3 Add unit tests for facade behavior in disabled/enabled modes and scope updates.

## 3. Player-Context API Instrumentation

- [ ] 3.1 Instrument `IMediaPipeline` handling paths with facade-based profiling scopes.
- [ ] 3.2 Instrument `IMediaKeys` handling paths with facade-based profiling scopes.
- [ ] 3.3 Instrument `IWebAudioPlayer` handling paths with facade-based profiling scopes.
- [ ] 3.4 Instrument `IMediaPipelineCapabilities` and `IMediaKeysCapabilities` handling paths with facade-based profiling scopes.
- [ ] 3.5 Verify `IControl`, `IControlClient`, and `IClientLogControl` paths remain excluded from this instrumentation capability.

## 4. Config-Driven Profiling Controls

- [ ] 4.1 Add parsing of profiling enable/disable and scope mode options from /etc/sky/rialto-config.json.
- [ ] 4.2 Add parsing and validation of interface allowlist option from /etc/sky/rialto-config.json, including ignore-with-warning handling for unknown interface names.
- [ ] 4.3 Propagate parsed config through serverManager to RialtoServer environment variables:
	- `RIALTO_SERVER_PROFILING_ENABLED`
	- `RIALTO_SERVER_PROFILING_SCOPE_MODE`
	- `RIALTO_SERVER_PROFILING_INTERFACES`
- [ ] 4.4 Add tests validating:
	- global scope behavior (`scopeMode=all`)
	- interface-level scope behavior (`scopeMode=interface`)
	- unknown-interface ignore-with-warning behavior
	- empty interface allowlist behavior
	- parser normalization behavior (trim/case-insensitive for accepted values)

## 5. Validation, Performance, and Documentation

- [ ] 5.1 Add/extend unit and component tests to validate behavioral parity when profiling is disabled.
- [ ] 5.2 Measure and document overhead in disabled mode and representative enabled scenarios.
- [ ] 5.3 Document operational usage via /etc/sky/rialto-config.json: enable/disable and scope selection.
- [ ] 5.4 Run targeted regression checks for media pipeline, DRM, web audio, and capabilities flows.
