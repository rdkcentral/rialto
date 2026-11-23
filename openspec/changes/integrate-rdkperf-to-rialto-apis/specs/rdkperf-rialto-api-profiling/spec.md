## ADDED Requirements

### Requirement: Config-Driven Profiling Enablement for Server Player APIs
Rialto server SHALL provide config-driven enablement for RDKPerf profiling over player-context public API handling paths.

#### Scenario: Enable profiling through config
- **WHEN** profiling is enabled through /etc/sky/rialto-config.json and propagated into RialtoServer runtime configuration
- **THEN** subsequent invocations of instrumented player-context APIs SHALL record RDKPerf timing scopes

#### Scenario: Disable profiling through config
- **WHEN** profiling is disabled through /etc/sky/rialto-config.json and propagated into RialtoServer runtime configuration
- **THEN** subsequent invocations of instrumented player-context APIs SHALL stop creating RDKPerf timing scopes

### Requirement: Profiling Scope Selection
Rialto server SHALL support profiling scope selection for player-context APIs via /etc/sky/rialto-config.json using either global/all scope or interface-level allowlist scope.

#### Scenario: Global scope enables all instrumented player interfaces
- **WHEN** profiling is enabled with scope mode set to `all` in /etc/sky/rialto-config.json
- **THEN** instrumented operations in `IMediaPipeline`, `IMediaKeys`, `IWebAudioPlayer`, `IMediaPipelineCapabilities`, and `IMediaKeysCapabilities` pathways SHALL create RDKPerf timing scopes

#### Scenario: Interface-level scope limits instrumentation
- **WHEN** profiling is enabled with scope mode set to `interface` and allowlisted interface names in /etc/sky/rialto-config.json
- **THEN** only instrumented operations in allowlisted interfaces SHALL create RDKPerf timing scopes

#### Scenario: Non-allowlisted interface remains uninstrumented
- **WHEN** profiling is enabled with interface-level allowlist scope and a call is processed through an instrumented player-context interface not present in the allowlist
- **THEN** Rialto SHALL NOT create a profiling scope for that call

#### Scenario: Empty allowlist disables interface-level instrumentation
- **WHEN** profiling is enabled with scope mode set to `interface` and the allowlist is empty
- **THEN** Rialto SHALL NOT create profiling scopes for player-context interfaces

#### Scenario: Invalid interface names are ignored with warning
- **WHEN** profiling is enabled with interface-level allowlist scope and /etc/sky/rialto-config.json contains unknown interface names
- **THEN** Rialto SHALL ignore unknown names, apply profiling only to valid allowlisted interfaces, and emit a warning indicating unsupported interface entries

#### Scenario: Scope mode parsing tolerates case and surrounding whitespace
- **WHEN** /etc/sky/rialto-config.json provides accepted scope mode values with mixed case and/or surrounding whitespace
- **THEN** Rialto SHALL normalize values and apply equivalent scope mode behavior

#### Scenario: Enablement parsing tolerates accepted textual values
- **WHEN** /etc/sky/rialto-config.json provides accepted enablement values represented through propagated runtime values (`1`, `true`, `yes`) with mixed case and/or surrounding whitespace
- **THEN** Rialto SHALL interpret those values as enabled

### Requirement: Config Propagation to RialtoServer Runtime
Rialto serverManager SHALL propagate profiling configuration to RialtoServer runtime using environment variables.

#### Scenario: Profiling config is propagated via environment variables
- **WHEN** profiling configuration is parsed from /etc/sky/rialto-config.json
- **THEN** serverManager SHALL propagate:
	- `RIALTO_SERVER_PROFILING_ENABLED`
	- `RIALTO_SERVER_PROFILING_SCOPE_MODE`
	- `RIALTO_SERVER_PROFILING_INTERFACES`

### Requirement: Player-Context API Instrumentation Coverage
Rialto SHALL instrument player-context public API handling paths and SHALL exclude control/log-control API paths from this capability.

#### Scenario: Player API call is instrumented
- **WHEN** profiling is enabled and scope selection includes the interface handling the call in `IMediaPipeline`, `IMediaKeys`, `IWebAudioPlayer`, `IMediaPipelineCapabilities`, or `IMediaKeysCapabilities` pathways
- **THEN** Rialto SHALL create an RDKPerf scope named for the API operation being handled

#### Scenario: Excluded control APIs remain uninstrumented by this capability
- **WHEN** profiling is enabled and calls are processed through `IControl`, `IControlClient`, or `IClientLogControl` pathways
- **THEN** Rialto SHALL NOT apply instrumentation from this capability to those pathways

### Requirement: Compatibility and Disabled-Mode Behavior
Rialto SHALL preserve existing functional behavior when profiling is disabled and SHALL keep profiling opt-in.

#### Scenario: Disabled mode preserves behavior
- **WHEN** profiling is disabled
- **THEN** player API functional results and callbacks SHALL remain behaviorally equivalent to pre-integration behavior

#### Scenario: Feature opt-in default
- **WHEN** Rialto starts without explicit profiling enablement
- **THEN** profiling SHALL remain disabled by default

### Requirement: Direct RDKPerf Dependency in Supported Server Builds
Rialto server profiling sources SHALL use direct RDKPerf dependency in supported server build targets.

#### Scenario: Profiling sources include RDKPerf directly
- **WHEN** supported server build targets are configured for this capability
- **THEN** profiling sources SHALL include and use `rdk_perf.h` directly (without conditional include fallback in those sources)

### Requirement: RDKPerf Reporting Behavior
Rialto SHALL preserve standard RDKPerf reporting behavior while profiling is enabled.

#### Scenario: Reporting is available when profiling is enabled
- **WHEN** profiling is enabled for player-context API paths
- **THEN** RDKPerf process/thread reporting data SHALL be available via RDKPerf's reporting mechanisms
