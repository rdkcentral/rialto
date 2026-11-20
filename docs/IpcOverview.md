# IPC Overview

This doc covers the protobuf IPC system used by Rialto client and server.

The IPC library uses unix domain sockets for sending messages, the messages themselves are serialised and deserialised
using the Google [protobuf][1] library.


### Structure
* `/ipc` - the IPC library, with examples and debug tool.  This is designed to be a general purpose [protobuf][1]
  based client and server using unix domain sockets.  It is protocol agnostic and is designed to be used with the
  [protobuf][1] C++ [RPC interface][2].  Further details on the library can be found in
  [ipc/README.md](../ipc/README.md).
* `/proto` - this contains the [protobuf][1] `.proto` protocol file(s) for the actual Rialto A/V client and server.
* `/media/client/ipc` - the client side implementation of the Rialto A/V and CDM protocol.
* `/media/server/ipc` - the server side implementation of the Rialto A/V and CDM protocol.


### Dependencies
The IPC library depends on protobuf library and compiler, minimum version is 3.6.


### App Container Changes
Server manager assignes a single named socket per client so thath each client / app has a socket with which to
communicate with the server.


### File descriptors
A message can carry file descriptors. Mark each `int32` field that holds one with the `field_is_fd` option from
`rialtoipc.proto`:

```proto
message ExportedBuffer {
    optional uint64 id = 1;
    optional int32 fd = 2 [(rialto.ipc.field_is_fd) = true];
}
message AcquireResponse {
    repeated ExportedBuffer buffers = 1;
}
```

* **Any depth.** `field_is_fd` works on top-level fields, on fields inside singular or repeated sub-messages at any
  depth, and on repeated `int32` fields (every element is a descriptor). Absent fields and absent sub-messages carry
  nothing. A `field_is_fd` field that is not `int32` fails the message.
* **Walk order.** The descriptors travel in one `SCM_RIGHTS` control message, and each field carries the placeholder
  `-1` on the wire. Sender and receiver match descriptors to fields in one order: fields in declaration order,
  repeated elements in index order, and each present sub-message walked in full at its own field position. Messages
  whose descriptors are all in top-level fields use the same order as before nested support was added.
* **Count mismatch.** A received message whose number of descriptors differs from its number of present
  `field_is_fd` fields is rejected and logged, and every descriptor received with it is closed.
* **Per-message limits.** A request carries at most 16 descriptors, and a reply or event at most 32
  (`kMaxFdsClientToServer` and `kMaxFdsServerToClient`). The sender checks the limit: a request over it fails locally
  and is not sent, a reply over it is replaced by an error reply, and an event over it is not sent (`sendEvent()`
  returns `false`).
* **Receiver ownership.** Each received descriptor is a new descriptor, independent of the sender's copy. On the
  client, descriptors in a reply or event belong to the caller, which must close them. On the server, descriptors in
  a request are valid while the handler runs and are closed when it returns, so a handler that keeps one must `dup()`
  it.
* **Owned replies.** By default the server leaves the descriptors it sends in a reply open; the handler still owns
  them. A handler can call `IController::setReplyFdsOwned(true)` to have the server close every descriptor in the
  reply exactly once after the send attempt, whatever its outcome: sent, failed, replaced by an error reply, or over
  the limit.
* **Delivery callback.** A handler can register `IController::setDeliveryCallback()`. The server calls it exactly
  once, after the send attempt and after closing owned descriptors, with `true` only if the whole success reply was
  written to the client's socket. It reports `false` if the client was gone, the write failed, or an error reply was
  sent instead. A reply written in full counts as delivered even if the client exits before reading it. The callback
  runs on the thread that completed the call with no IPC lock held, so it may call back into the service or send
  events.

Both settings must be made before `done->Run()`. Handlers receive a `google::protobuf::RpcController *` and reach the
IPC extensions with a `dynamic_cast`:

```cpp
void CaptureModuleService::acquire(google::protobuf::RpcController *controller, const AcquireRequest *request,
                                   AcquireResponse *response, google::protobuf::Closure *done)
{
    const uint64_t kFrameId = lockFrame();
    auto *buffer = response->add_buffers();
    buffer->set_id(kFrameId);
    buffer->set_fd(exportFrameFd(kFrameId)); // a new descriptor the server no longer needs after sending

    auto *ipcController = dynamic_cast<firebolt::rialto::ipc::IController *>(controller);
    if (ipcController)
    {
        ipcController->setReplyFdsOwned(true); // the server closes the exported fd after the send attempt
        ipcController->setDeliveryCallback(
            [this, kFrameId](bool delivered)
            {
                if (!delivered)
                    releaseFrame(kFrameId); // the client never received it
            });
    }
    done->Run();
}
```

[1]: https://developers.google.com/protocol-buffers
[2]: https://developers.google.com/protocol-buffers/docs/proto#services
