# Design

## Context

See proposal.md for why this change is needed, and `specs/ipc-fd-transfer/spec.md` for the required behaviour.

Today the IPC layer handles FDs in four separate places. Each one loops over `descriptor->field(i)` of the outer message only:

| Direction | Collect (sender) | Restore (receiver) |
|---|---|---|
| Client request to server | `IpcChannelImpl.cpp` ~1040 (collect for `sendmsg`) | `IpcServerImpl.cpp:850` `addRequestFileDescriptors` |
| Server reply or event to client | `IpcServerImpl.cpp:1114` `getResponseFileDescriptors` (replaces the value with `-1` before serializing) | `IpcChannelImpl.cpp:932` `addReplyFileDescriptors` |

On the wire, an FD field is present with the placeholder `-1`, and the real descriptors travel in one `SCM_RIGHTS` control message, in walk order. The receivers read at most 16 FDs per message (server, `readMessageFds(&msg, 16)`) and 32 (client).

The reply path works like this. `processMethodCall` creates a `ServerControllerImpl` and passes `NewCallback(this, &ServerImpl::handleResponse, controller, response)` as `done`. When the handler calls `done->Run()`, which may happen on any thread, `handleResponse` builds the `msghdr` (`populateReply`, or `populateErrorReply` if failed), deletes the controller and response, and calls `sendReply`, which returns `void`. Handlers see the controller as `google::protobuf::RpcController*` and `dynamic_cast` it to `ipc::IController` when they need IPC extras (see `ControlModuleService::registerClient`).

## Goals / Non-Goals

**Goals:**
- One FD-walk implementation shared by all four sites, so sender and receiver order cannot drift.
- Owned FDs and a delivery result for replies, without changing any existing handler.
- Wire compatibility for every existing proto.

**Non-Goals:**
- Owned FDs or delivery callbacks for events. `IClient::sendEvent()` already returns `bool`, and no event carries FDs today. This can be added later if needed.
- Raising the 16/32 per-message limits. A capture frame needs at most `maxResourcesPerFrame` FDs, which is well under 32.
- Detecting whether the client actually imported the FDs. Per the spec, a full write counts as delivered, and disconnect cleanup covers the rest.
- Any capture-specific types. This change is generic IPC.

## Decisions

### D1. One recursive walker in `ipc/common`

Add an internal helper, for example `ipc/common/source/FdFieldWalker.{h,cpp}`. It visits each present FD field of a message, in the order defined by the spec, through a callback that receives `(Message*, FieldDescriptor*, int index)` with `index = -1` for singular fields. The order is:
1. fields in `descriptor->field(i)` order (declaration order, which is what today's code uses);
2. for a repeated `int32` FD field, each element in index order;
3. for a singular message field, recurse if `HasField`; for a repeated message field, recurse into each element in index order.

All four sites become thin callers: collect (read the value, write `-1`), or restore (write `fd()` from the received vector). Mismatched counts and non-`int32` FD fields fail the same way as today.

*Why:* for top-level-only messages, this order matches the current loop exactly, so existing protos are byte-for-byte unchanged. Sharing the code removes the risk of the two ends drifting apart.
*Alternative:* each site does its own recursion. Rejected because there would be four copies of the order rule.
*Alternative:* a descriptor-indexed map sent in the payload. Rejected because it changes the wire format for no gain.

The walker has no explicit depth limit. It only follows sub-messages that are present, and protobuf's own parse recursion limit bounds received messages.

### D2. Ownership and delivery live on `ipc::IController`

Add two non-pure virtual methods to `ipc::IController`. Their defaults keep existing mocks and implementers compiling.

```cpp
// Close every FD in the response after the send attempt, whatever the outcome.
virtual void setReplyFdsOwned(bool owned) {}
// Invoked exactly once after the send attempt with true only if the whole success reply was written.
virtual void setDeliveryCallback(std::function<void(bool delivered)> callback) {}
```

`ServerControllerImpl` stores both. A handler sets them, through the usual `dynamic_cast`, before calling `done->Run()`.

*Why the controller:* it is per-call, already exists, is already reached from handlers, and lives exactly until the reply is sent. This settles roadmap question A6 with a callback rather than having IPC clean up by itself, because only the service knows how to undo its side effect (for capture, release the HAL frame and keep the invalidation flag pending).
*Alternative:* a special response wrapper type. Rejected because generated service signatures fix the response type.
*Alternative:* return the result from `done->Run()`. Rejected because `Closure::Run` returns `void`.

### D3. Send path returns a result and closes owned FDs

- `sendReply` returns `bool`: true only when the client was found, the socket is open, the message is valid, and `sendmsg` wrote the whole `iov_len`.
- `getResponseFileDescriptors`, which uses D1, runs before the success-or-error branch, so the FDs are known even when an error reply is sent instead.
- In `handleResponse`, after `sendReply`, and before deleting the controller:
  1. if owned, close each collected FD exactly once, ignoring `-1` and duplicates;
  2. invoke the delivery callback with `delivered = sent && !controller->m_failed && reply was not replaced`.
- If the walk hits a non-`int32` FD field, or the count is over 32, the response is replaced with `populateErrorReply(..., "Internal error - ...")`. That is reported as not delivered, and owned FDs are still closed.

The callback runs on the thread that called `done->Run()`, while no IPC lock is held (the `m_clientsLock` guard in `sendReply` has been released). This lets it call back into the service, for example to release a frame.

### D4. Sender-side limit checks

Each side checks the count before building `SCM_RIGHTS`:
- the client's request path checks against 16 and fails the call through the client controller;
- the server's reply path checks against 32 (D3);
- the server's event path (`IpcServerImpl.cpp` ~1336) checks against 32 and returns `false` from `sendEvent`.

The limits become named constants shared with `readMessageFds`, so the two numbers cannot differ.

### D5. Receivers own what they receive

This is unchanged: received FDs arrive as `FileDescriptor` RAII objects, are written into the message, and are released only after every field is placed, so on failure they are all closed. D1 only changes how fields are found.

## Risks / Trade-offs

- [Order drift with a future peer that does its own walk] → The order is normative in the spec, and unit tests pin it with a mixed top-level/nested test proto.
- [An owned FD listed twice in one reply, so closed twice] → Close de-duplicates by value. A handler should not put the same FD in two fields; the IPC debug log notes it.
- [Delivery callback re-enters IPC] → It runs with no IPC lock held (D3), so re-entering is safe. A test calls a second RPC from the callback.
- [Adding virtuals to `IController` changes its vtable (ABI)] → `ipc` is built and shipped together with the server, and nothing outside the tree implements `IController`. The defaults keep the source compatible.
- [Full write still not a successful import] → Accepted by design (spec, "Delivered but client dies afterwards"). R10's disconnect cleanup owns that case.

## Migration Plan

No migration. Existing protos and handlers behave the same. Rollback is reverting the change, and no proto or persisted state is affected.

## Open Questions

None that affect the specs or the task breakdown.
