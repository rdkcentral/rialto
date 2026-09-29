## Context

The Rialto session server exposes decoder capabilities through `IMediaPipelineCapabilities`
(`getSupportedAudioCapabilities`, `getSupportedVideoCapabilities`). Internally these methods
use either HFP YAML files or a GStreamer element-query fallback. The ServerManager has no
visibility into codec availability and each session server process independently loads YAML
at startup, creating redundant file reads.

This change centralises capability loading in the ServerManager, introduces typed proto fields
in `SetConfigurationRequest` to forward the data to each session server, and replaces the
existing `IMediaPipelineCapabilities` capability methods with a new `IMediaCapabilities`
interface used by both the client library and internal Rialto consumers (`rialtomse*` sinks).
When the ServerManager sends no capabilities (HFP YAML absent), the session server retains the
existing GStreamer element-query path as a fallback — the new interface always produces a
result.

## Goals / Non-Goals

**Goals:**
- Move `AudioDecoderCapabilities` / `VideoDecoderCapabilities` to `common/` so both the
  ServerManager and session server can include them without cross-layer dependencies.
- Add `IMediaCapabilities` to the ServerManager public API backed by `IYamlCppWrapper`.
- Forward capabilities via typed proto fields in `SetConfigurationRequest`; no new IPC socket.
- Implement a two-path `GstCapabilities`: Path A uses pre-loaded data from the ServerManager;
  Path B runs the GStreamer element-query when no data is supplied.
- Replace `IMediaPipelineCapabilities` capability methods on the client side with
  `IMediaCapabilities`. Remove legacy; no side-by-side support.
- Migrate `rialtomse*` sinks and all other consumers to `IMediaCapabilities`.

**Non-Goals:**
- Use COM-RPC, D-Bus, or any mechanism other than the existing Unix socket + protobuf.

## Decisions

### Use typed proto message fields instead of `optional bytes`

The proto fields in `SetConfigurationRequest` use typed `AudioCapabilities` /
`VideoCapabilities` messages rather than raw `bytes`. This makes the wire format
self-describing, eliminates manual serialise/deserialise shim code, and allows proto tooling
to validate field presence. The messages can be defined inline in `servermanagermodule.proto`
or extracted to a shared proto imported by both `servermanagermodule.proto` and
`mediapipelinecapabilitiesmodule.proto`.

Alternatives considered:
- `optional bytes`: rejected — requires separate shim classes (`CapabilitySerialiser`,
  `CapabilityDeserialiser`) and causes static-initializer ordering issues when importing
  cross-package message types. Typed fields in the same package avoid both problems.

### GStreamer element-query is the fallback, not YAML re-load

When the ServerManager sends no capabilities (HFP YAML absent on the platform), the session
server's `GstCapabilities` runs the GStreamer element-query mechanism. This is the **same
code path** that `IMediaPipelineCapabilities` uses today. The GStreamer-query path is retained
inside `GstCapabilities` as a permanent fallback, not removed.

Alternatives considered:
- Re-load YAML in the session server on absent fields: rejected — the ServerManager's
  `IMediaCapabilities` already tried YAML and found it absent; having the session server retry
  produces the same empty result with double file I/O.
- Return empty capabilities when no data: rejected — `IMediaCapabilities` must always return a
  usable answer; empty capabilities would silently break codec negotiation on platforms that
  do not ship HFP YAML files.

### Capabilities stored in `SessionServerAppManager`, forwarded as call parameters

`SessionServerAppManager` owns the lifecycle of each session server. It loads capabilities at
construction time via `IMediaCapabilities` and passes them as `std::optional` parameters on
each `performSetConfiguration()` call. This is consistent with how all other `SetConfiguration`
params flow today (`socketName`, `maxResource`, `logLevels`).

Alternatives considered:
- Store capabilities as a member in `Client`: rejected — `Client` is a thin IPC wrapper; it
  should not own orchestration state.
- Re-load capabilities on each `performSetConfiguration()` call: rejected — YAML is stable
  across the process lifetime; one load at startup is sufficient.

### Remove `IMediaPipelineCapabilities` capability methods; no side-by-side

`getSupportedAudioCapabilities()` and `getSupportedVideoCapabilities()` are removed from
`IMediaPipelineCapabilities`. The GStreamer-query logic that implements them today moves
into `GstCapabilities` Path B. No parallel interface is provided; all consumers are migrated
to `IMediaCapabilities` in this change.

Alternatives considered:
- Keep both interfaces side-by-side: rejected — increases maintenance surface and creates
  ambiguity about which interface clients should call. A clean cut is simpler.

### `IMediaCapabilities` client interface uses new dedicated MediaCapabilitiesModule IPC service

The new client-side `IMediaCapabilities` calls through a dedicated `MediaCapabilitiesModule`
protobuf IPC service (in `proto/mediacapabilitiesmodule.proto`). This provides a clean,
dedicated transport for capability queries, improving separation of concerns and allowing
independent scaling/caching of capability requests separate from media pipeline operations.

## Risks / Trade-offs

- Struct move (`AudioDecoderCapabilities`, `VideoDecoderCapabilities` from `media/` to
  `common/`) — all existing include paths must be updated; a missed include is a compile
  error caught immediately.
- Proto change adds a new `MediaCapabilitiesModule` service and two optional fields to
  `SetConfigurationRequest` — backward-compatible addition; existing deployments without the
  new fields continue to work (session server falls back to Path B).
- Removing `IMediaPipelineCapabilities` capability methods breaks any out-of-tree consumers;
  mitigation is the migration of all known consumers (`rialtomse*` sinks) in this change.
- Path B (GStreamer fallback) now runs on every session server that receives no capabilities
  from the ServerManager — same performance as today; no regression.

## Migration Plan

1. Move capability structs to `common/`; update all includes.
2. Add `IMediaCapabilities` + factory in `serverManager/public/` and `serverManager/service/`.
3. Define typed proto messages; extend `SetConfigurationRequest`.
4. Update `SessionServerAppManager` → `IController` → `Client` to thread optional params.
5. Add `CapabilitySerialiser` in `serverManager/ipc/`.
6. Extend `configureServices()` and update `GstCapabilities` with two-path model.
7. Add client `IMediaCapabilities` interface; remove legacy capability methods from
   `IMediaPipelineCapabilities`; add `MediaCapabilitiesIpc`; update `rialtomse*` sinks.
8. Wire build targets; add and update unit tests; validate.

Rollback: proto fields are optional (no wire-breaking change); removing the capability
serialisation from `SessionServerAppManager` and the client wiring restores the prior state.
The GStreamer fallback path is never removed, so Path B always remains available.
