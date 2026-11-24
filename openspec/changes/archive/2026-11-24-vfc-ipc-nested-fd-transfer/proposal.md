## Why

Video frame capture returns each frame's DMA-BUF FDs in `exported_dma_bufs[].fd`, a `field_is_fd` field inside a repeated sub-message (V1.1 proposal §3.6, §10). The Rialto IPC layer only finds FDs in top-level, singular `int32` fields of the outer message, so these FDs would never be attached or restored. The SoC capture spec (§6.1, §7.3) also requires the server to close its own copies once they are sent, and to release the frame and keep the invalidation flag pending when delivery fails. Today the server never closes FDs it sends, and a failed `sendmsg` is only logged, so the service never learns about it. Every later capture change that moves frames depends on fixing this transport first.

## What Changes

- FD discovery covers `field_is_fd` fields at any depth: singular and repeated sub-messages, and repeated `int32` FD fields. It visits fields in one fixed, documented order, and sender and receiver use the same order. This applies to requests, replies and events.
- The receiver restores each received FD into the field it came from, using the same walk. Too many or too few FDs fail the message, as today, and every received FD is closed.
- A service handler can mark a reply's FDs as owned. The server then closes them after the send attempt, whether or not it succeeds. This is opt-in: unmarked replies keep today's behaviour, so the long-lived shared-memory FD sent by `ControlModuleService` is not closed.
- A service handler can learn whether its reply was delivered, meaning `sendmsg` wrote the whole message. A reply that cannot be delivered (the client is gone, the socket is closed, a short or failed send, or serialization fails) is reported as failed. The design document chooses the mechanism; this settles roadmap question A6. A send that succeeds but whose client dies before using the FDs still counts as delivered, and disconnect cleanup handles it.
- A message whose FD count exceeds the receiving side's per-message limit (16 from client to server, 32 from server to client) is rejected by the sender with a logged error. Today the receiver silently drops the extras.
- Messages that only use top-level `field_is_fd` fields are unchanged on the wire, so existing protos (`controlmodule.proto`, `servermanagermodule.proto`) and older peers keep working.

## Capabilities

### New Capabilities

- `ipc-fd-transfer`: How the Rialto IPC layer finds, sends, restores and owns file descriptors carried in `field_is_fd` protobuf fields for requests, replies and events. Also covers per-message limits and how reply delivery outcomes reach the service handler.

### Modified Capabilities

None. The project has no existing specs.

## Impact

- **Code:**
  - `ipc/server/source/IpcServerImpl.cpp`: `getResponseFileDescriptors`, `addRequestFileDescriptors`, `sendReply`, the reply and event send paths, and `readMessageFds` limits.
  - `ipc/client/source/IpcChannelImpl.cpp`: `addReplyFileDescriptors` and the request FD collection.
  - The shared FD-walk helper can live in `ipc/common`.
- **Public IPC API:** additive. Server-side controller or closure support for owned reply FDs and delivery notification goes in `ipc/server/include`. No existing signature changes.
- **Protos:** no change to `rialtoipc.proto`. `field_is_fd` simply becomes valid on nested fields.
- **Tests:**
  - New IPC unit tests with a test proto holding 0, 1 and 3 nested repeated FDs, and too-many and too-few cases.
  - A server FD-leak check for owned replies.
  - An injected send failure that reaches the handler.
  - The existing shared-memory and session-socket FD tests must stay green.
- **Downstream:** unblocks roadmap change R9 (`vfc-server-acquire-release`), which sends owned frame FDs and releases the frame when delivery fails.
