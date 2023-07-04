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

[1]: https://developers.google.com/protocol-buffers
[2]: https://developers.google.com/protocol-buffers/docs/proto#services
