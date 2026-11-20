# Tasks

Unit tests run with `./build_ut.py -s ipc` (suite `RialtoIpcUnitTests`, `tests/unittests/ipc/`). Add `-gf <filter>` to run one test. "Spec" means `specs/ipc-fd-transfer/spec.md`; "D#" means a decision in `design.md`.

## 1. Shared FD walker (D1)

- [x] 1.1 Add an internal `FdFieldWalker` (`ipc/common/source/FdFieldWalker.{h,cpp}`) that visits every present `field_is_fd` field in the spec's order (declaration order, repeated elements by index, sub-messages recursed at their own position) and reports any non-`int32` FD field as an error. Add it to `ipc/common/CMakeLists.txt` and verify that `ipc` builds.
- [x] 1.2 Extend `tests/unittests/ipc/proto/testmodule.proto` with FD test messages: top-level only; a repeated sub-message with a nested FD; a repeated `int32` FD field; mixed top-level, nested, top-level; a nested optional sub-message left unset; a non-`int32` field marked `field_is_fd`. Verify that the test proto compiles.
- [x] 1.3 Add walker unit tests (`tests/unittests/ipc/FdFieldWalkerTest.cpp`) that pin the visit order for every 1.2 message, check that unset fields and sub-messages are skipped, and check the non-`int32` error. Verify with `./build_ut.py -s ipc -gf '*FdFieldWalker*'`.

## 2. Nested FDs end to end (spec: FD fields at any depth, Deterministic FD order, FD count mismatch)

- [x] 2.1 Replace the server's top-level loops with the walker: `getResponseFileDescriptors` (collect, write `-1`) and `addRequestFileDescriptors` (restore) in `ipc/server/source/IpcServerImpl.cpp`. Verify that the existing `RialtoIpcTest` cases still pass.
- [x] 2.2 Replace the client's top-level loops with the walker: request FD collection and `addReplyFileDescriptors` in `ipc/client/source/IpcChannelImpl.cpp`. Verify that the existing `RialtoIpcTest` cases still pass.
- [x] 2.3 Add round-trip tests in `tests/unittests/ipc/IpcTest.cpp` over a real socket pair for requests, replies and events using the 1.2 messages: 0, 1 and 3 nested repeated FDs, a repeated FD field, and the mixed order. Assert that each received FD refers to the same file as the one sent (compare `fstat` `st_ino`/`st_dev`) and is a different descriptor number. Verify with `./build_ut.py -s ipc`.
- [x] 2.4 Add mismatch tests that send with too few and too many attached FDs (inject at the raw socket) and assert the message is rejected, every received FD is closed (check the open-FD count in `/proc/self/fd`), and the error is logged. Verify with `./build_ut.py -s ipc`.
- [x] 2.5 Add a compatibility check: `ControlModuleService::getSharedMemory` and the server-manager session-socket FD still work. Verify that `./build_ut.py -s manager serveripc clientipc` and the control component tests (`./build_ct.py`, `ControlTest`) pass unchanged.

## 3. Per-message FD limits (D4; spec: Per-message FD limit)

- [x] 3.1 Turn the 16 (client to server) and 32 (server to client) limits into named constants shared by `readMessageFds` on both sides. Verify that grep finds no remaining literal `16`/`32` FD limits in `ipc/`.
- [x] 3.2 Enforce the limits on the sender: a client request over 16 fails the call locally and sends nothing; a server reply over 32 becomes an error reply; a server event over 32 makes `sendEvent` return `false`. Add one test per direction in `IpcTest.cpp` and verify with `./build_ut.py -s ipc`.

## 4. Owned reply FDs and delivery callback (D2, D3; spec: Owned reply FDs, Reply delivery is reported)

- [x] 4.1 Add non-pure virtual `setReplyFdsOwned(bool)` and `setDeliveryCallback(std::function<void(bool)>)`, with no-op defaults, to `ipc/server/include/IIpcController.h`, with Doxygen comments that state the once-only, after-send, no-lock-held contract. Verify that `tests/unittests/ipc/mocks/IpcControllerMock.h` and `tests/unittests/serverManager/mocks/ControllerMock.h` compile unchanged, and add `MOCK_METHOD`s for the new calls to `IpcControllerMock.h`.
- [x] 4.2 Store both settings in `ServerControllerImpl` (`ipc/server/source/IpcServerControllerImpl.{h,cpp}`). Verify that the IPC unit tests build.
- [x] 4.3 Make `ServerImpl::sendReply` return `bool` (true only on a full `sendmsg` to a connected client). Collect reply FDs before the success-or-error branch in `handleResponse`. After sending, close owned FDs once (skipping `-1` and duplicates), then invoke the delivery callback with `sent && success reply not replaced`, after `m_clientsLock` has been released. Verify that the existing reply tests pass.
- [x] 4.4 Add owned-FD tests in `IpcTest.cpp`: success, client disconnected before the send, `SetFailed` after filling FD fields, a reply over the limit, and an unowned reply whose FD stays open. Assert the server's open-FD count returns to its baseline in every owned case. Verify with `./build_ut.py -s ipc`.
- [x] 4.5 Add delivery-callback tests: delivered once on success; not delivered when the client is gone, on an injected `sendmsg` failure or short write (socket mock or a closed peer), and on an error reply; no callback means behaviour is unchanged; a callback that issues another RPC does not deadlock. Verify with `./build_ut.py -s ipc`.

## 5. Documentation

- [x] 5.1 Add a "File descriptors" section to `docs/IpcOverview.md` that covers `field_is_fd` at any depth, the walk order, the per-message limits, receiver ownership, owned replies, and the delivery callback, with a short handler example. Verify that the section's example matches the API from 4.1.

## 6. Integration check

- [ ] 6.1 Run the full unit and component suites (`./build_ut.py` and `./build_ct.py`) and confirm no regressions. Then run `./build_ut.py -s ipc -val` and confirm valgrind reports no FD or memory leaks in the new tests.

## Workflow follow-up

- Run `openspec validate vfc-ipc-nested-fd-transfer --strict` before review.
- Archive the change after review, so `ipc-fd-transfer` becomes a main spec that R9 (`vfc-server-acquire-release`) can depend on.
