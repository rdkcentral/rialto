## Why

Rialto currently lacks a standardized, runtime-configurable profiling pathway for its public player APIs. Integrating RDKPerf now enables low-friction latency and call-stack visibility for high-value media paths without requiring permanent instrumentation overhead.

## What Changes

- Introduce a server-side Rialto profiling integration layer that wraps RDKPerf scopes and is enabled/disabled via config propagated to environment variables.
- Integrate profiling hooks across selected server player-context handling paths (media pipeline, DRM/media keys, web audio, and capabilities).
- Add config-file options in /etc/sky/rialto-config.json to enable/disable profiling and select scope as all interfaces or a specific interface allowlist.
- Use a direct dependency on `rdk_perf.h` for this capability in supported server builds.
- Ensure profiling remains off by default and has no behavioral impact when disabled.

## Capabilities

### New Capabilities
- `rdkperf-rialto-api-profiling`: Config-driven RDKPerf profiling integrated into Rialto server player-context API handling paths, with enablement and scope selection (global or interface-level allowlist) driven by /etc/sky/rialto-config.json.

### Modified Capabilities
None.

## Impact

- Affected player-context API handling paths in Rialto server components.
- Affected server-side implementation pathways where public API calls are handled and dispatched.
- Affected serverManager configuration parsing for /etc/sky/rialto-config.json profiling options (enablement and scope selection), with propagation into RialtoServer process environment.
- Affected build/runtime dependency wiring for `rdk_perf.h` in supported server targets.
- Additional observability output in logs/reporting when profiling is enabled.

## Config Samples

Enable profiling for all supported interfaces:

```json
{
	"profiling": {
		"enabled": true,
		"scopeMode": "all"
	}
}
```

Enable profiling for selected interfaces only:

```json
{
	"profiling": {
		"enabled": true,
		"scopeMode": "interface",
		"interfaces": [
			"IMediaPipeline",
			"IMediaKeys",
			"IWebAudioPlayer"
		]
	}
}
```

Disable profiling:

```json
{
	"profiling": {
		"enabled": false
	}
}
```
