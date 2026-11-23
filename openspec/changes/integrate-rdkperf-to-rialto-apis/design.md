## Context

Rialto public player-context APIs currently expose no unified, runtime-configurable performance instrumentation contract. Existing profiling is ad hoc and not consistently available across media pipeline, media keys, web audio, and capabilities pathways. RDKPerf provides low-overhead scope timing (`RDKPerf` / `RDKPerfStart`/`RDKPerfStop`) and process/thread report APIs, making it a suitable backend for standardized profiling in Rialto.

This change must preserve production behavior when profiling is disabled, avoid instrumenting non-player APIs (explicitly excluding `IControl`, `IControlClient`, and `IClientLogControl`), and allow operational teams to tune profiling through config without code changes.

## Goals / Non-Goals

**Goals:**
- Add a unified profiling mechanism in Rialto that uses RDKPerf under a runtime-configurable gate.
- Integrate profiling in server player-context public API handling paths:
  - `IMediaPipeline` / `IMediaPipelineClient` flows
  - `IMediaKeys` / `IMediaKeysClient` flows
  - `IWebAudioPlayer` / `IWebAudioPlayerClient` flows
  - `IMediaPipelineCapabilities` and `IMediaKeysCapabilities` flows
- Configure profiling through /etc/sky/rialto-config.json with support for global enable-all or interface-level allowlist selection.
- Ensure default behavior remains effectively unchanged when profiling is disabled.

**Non-Goals:**
- No instrumentation of control/log-control APIs (`IControl`, `IControlClient`, `IClientLogControl`).
- No replacement of RDKPerf reporting internals or output format.
- No broad tracing framework migration beyond RDKPerf integration.
- No CI overhead budget gates in this iteration (to be considered in a follow-up change).

## Decisions

1. Introduce a Rialto profiling facade instead of direct RDKPerf calls scattered across modules.
- Rationale: Encapsulates dependency and gating logic, provides a single control point for enablement/scope behavior, and limits include/link spread.
- Alternative considered: direct inline `RDKPerf` usage at each callsite. Rejected due to coupling and maintainability risks.

2. Runtime configuration is centralized and thread-safe.
- Rationale: Public APIs may execute on different threads; enablement and scope checks must be deterministic and low-latency.
- Alternative considered: per-module static flags. Rejected due to inconsistent behavior and difficult operations.

3. API-level instrumentation scope is method entry/exit with optional targeted sub-scopes for known expensive operations.
- Rationale: Provides actionable high-level latency maps first, then deeper hotspots as needed.
- Alternative considered: only deep internal instrumentation. Rejected because it weakens API-level observability goals.

4. Profiling controls are config-driven via /etc/sky/rialto-config.json and do not add new public API controls in v1; v1 supports global enable-all and interface-level allowlist scoping.
- Rationale: The requested operational model is config-file-based control with practical scope narrowing at interface granularity, while avoiding public API surface expansion.
- Alternative considered: additive public profiling interfaces/factory extensions. Rejected for v1 because config-based control is sufficient.

5. Per-method dynamic scoping is deferred beyond v1.
- Rationale: Interface-level scoping provides most of the operational value with lower config, validation, and test complexity.
- Alternative considered: per-method configuration in v1. Rejected due to schema complexity and significantly expanded test matrix.

6. Build integration uses a direct `rdk_perf.h` dependency in supported server targets.
- Rationale: This change intentionally removes conditional include fallback in profiling sources and relies on a proper build/include setup in target environments.
- Alternative considered: conditional include and fallback behavior in profiling sources. Rejected to keep implementation explicit and simpler.

7. Reporting behavior in v1 relies on standard RDKPerf reporting with no additional cadence controls in Rialto config.
- Rationale: Reporting ownership remains with RDKPerf, reducing v1 complexity and avoiding parallel reporting control semantics.
- Alternative considered: add reporting cadence controls in Rialto config for v1. Rejected and deferred to a follow-up change if needed.

## Configuration Model (v1)

Rialto profiling configuration is read from /etc/sky/rialto-config.json (via serverManager parsing and environment propagation) and supports:
- `enabled`: boolean profiling on/off switch.
- `scopeMode`: `all` or `interface`.
- `interfaces`: list of interface names used only when `scopeMode=interface`.

Example: enable profiling for all supported player-context interfaces.
```json
{
  "profiling": {
    "enabled": true,
    "scopeMode": "all"
  }
}
```

Example: enable profiling only for selected interfaces.
```json
{
  "profiling": {
    "enabled": true,
    "scopeMode": "interface",
    "interfaces": ["IMediaPipeline", "IMediaKeys"]
  }
}
```

## Risks / Trade-offs

- [Runtime overhead when enabled] -> Keep profiling off by default; use lightweight guard checks and scope objects only when enabled.
- [Configuration propagation inconsistency] -> Define a single config source and propagate state through one profiling facade.
- [Configuration races across threads] -> Use lock-free atomics for simple flags and minimal synchronized updates for compound settings.
- [Log/report noise in busy workloads] -> Support scoped enablement to narrow data volume.
- [Dependency availability across platforms] -> Ensure supported build targets provide `rdk_perf.h` include wiring.

## Migration Plan

1. Ensure dependency wiring for RDKPerf on supported server targets.
2. Introduce server profiling facade/state and env-based consumption in RialtoServer.
3. Integrate instrumentation into selected server player-context API handling pathways.
4. Add config parsing and propagation for profiling options (enablement, scope mode, and interface allowlist).
5. Add unit/component tests for disabled/enabled/scope-selection behaviors.
6. Validate overhead in disabled mode and representative enabled scenarios.
7. Rollout with profiling disabled by default; document runtime activation playbook.

Rollback strategy:
- Disable profiling in /etc/sky/rialto-config.json.
- If needed, remove/disable deployment config that enables profiling in server environments.

## Open Questions

None.
