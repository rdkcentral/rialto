# Spec Delta

## Purpose

Defines how the Rialto IPC layer carries file descriptors in protobuf messages between client and server: which fields carry them, how they are matched to the descriptors sent alongside the message, who owns and closes them, and how a service learns whether a reply carrying them was delivered.

## ADDED Requirements

### Requirement: FD fields at any depth
The IPC layer SHALL carry a file descriptor for every present `field_is_fd` field in a request, reply or event. This includes fields in singular or repeated sub-messages at any depth, and every element of a repeated FD field. A `field_is_fd` field whose type is not `int32` SHALL fail the message.

#### Scenario: Top-level FD field
- **WHEN** a reply sets one top-level `field_is_fd` field to an open descriptor
- **THEN** the receiver gets the message with that field holding a valid descriptor for the same open file

#### Scenario: FDs in a repeated sub-message
- **WHEN** a reply contains a repeated sub-message with three elements, each setting its nested `field_is_fd` field
- **THEN** the receiver gets three valid descriptors, each in the element it was sent from

#### Scenario: Repeated FD field
- **WHEN** a message sets a repeated `int32` `field_is_fd` field with two elements
- **THEN** the receiver gets both elements as valid descriptors, in the same element order

#### Scenario: Absent fields carry nothing
- **WHEN** a message leaves an optional `field_is_fd` field or an optional sub-message unset
- **THEN** no descriptor is sent for it, and the field stays unset at the receiver

#### Scenario: Non-int32 field marked as FD
- **WHEN** a message marks a field that is not `int32` with `field_is_fd`
- **THEN** the message is not delivered as valid, and the error is logged

### Requirement: Deterministic FD order
Sender and receiver SHALL match descriptors to fields by one traversal order: fields in declaration order, repeated elements in index order, and each present sub-message visited in full at its own field position. For a message whose FD fields are all top-level, the order and wire format SHALL be the same as before this change.

#### Scenario: Mixed top-level and nested fields
- **WHEN** a message declares a top-level FD field, then a repeated sub-message holding FD fields, then another top-level FD field
- **THEN** every descriptor arrives in the field and element it was sent from

#### Scenario: Existing protocol unchanged
- **WHEN** an unmodified peer exchanges a `getSharedMemory` reply or a server-manager message carrying a session socket FD
- **THEN** the exchange behaves exactly as before this change

### Requirement: FD count mismatch fails the message
A receiver SHALL reject a message whose number of attached descriptors differs from the number of present `field_is_fd` fields, and SHALL close every descriptor received with it.

#### Scenario: Too few descriptors
- **WHEN** a message has three present FD fields but arrives with two descriptors
- **THEN** the message is rejected, both received descriptors are closed, and the error is logged

#### Scenario: Too many descriptors
- **WHEN** a message has one present FD field but arrives with two descriptors
- **THEN** the message is rejected, both received descriptors are closed, and the error is logged

### Requirement: Per-message FD limit
A sender SHALL refuse to send a message with more descriptors than the receiving side accepts: 16 per message from client to server, and 32 from server to client. A refused request SHALL fail the call. A refused reply SHALL be replaced by an error reply, and a refused event SHALL report failure to the caller.

#### Scenario: Reply over the limit
- **WHEN** a service replies with 33 present FD fields
- **THEN** the client receives an error reply instead of the response, and no descriptors

#### Scenario: Request over the limit
- **WHEN** a client calls a method whose request has 17 present FD fields
- **THEN** the call fails locally with an error, and nothing is sent

#### Scenario: Event over the limit
- **WHEN** the server sends an event with 33 present FD fields
- **THEN** sending the event reports failure, and nothing is sent

### Requirement: Received FDs are owned by the receiver
Each descriptor placed into a received message SHALL be a new descriptor owned by the receiving code, independent of the sender's copy. Closing either copy SHALL NOT affect the other.

#### Scenario: Sender closes after sending
- **WHEN** the server closes its descriptor after a reply carrying it is sent
- **THEN** the client's descriptor still refers to the open file

### Requirement: Owned reply FDs are closed by the server
A service SHALL be able to mark a reply's descriptors as owned. The server SHALL then close every descriptor in that reply exactly once after the send attempt, whatever its outcome: sent, failed, error reply instead, or refused over the limit. Unmarked replies SHALL leave their descriptors open, as before.

#### Scenario: Owned reply sent
- **WHEN** a service marks its reply owned and the reply, carrying two descriptors, is sent successfully
- **THEN** both server-side descriptors are closed and the server's open descriptor count returns to its earlier value

#### Scenario: Owned reply fails to send
- **WHEN** a service marks its reply owned and the client has disconnected before the send
- **THEN** every descriptor in the reply is still closed exactly once

#### Scenario: Owned reply replaced by an error
- **WHEN** a service marks its reply owned, fills FD fields, and then marks the call failed
- **THEN** the client receives the error reply and every descriptor in the response is closed

#### Scenario: Unowned reply keeps its descriptor
- **WHEN** the shared-memory reply is sent without marking it owned
- **THEN** the server's shared-memory descriptor stays open

### Requirement: Reply delivery is reported to the service
A service SHALL be able to register a delivery callback before completing a call. The server SHALL invoke it exactly once, after the send attempt and after closing owned descriptors. It reports delivered only when the whole success reply, with all its descriptors, was written to the client's socket; otherwise not delivered.

#### Scenario: Successful delivery
- **WHEN** a service registers a delivery callback and its reply is written in full
- **THEN** the callback is invoked once with delivered

#### Scenario: Client gone
- **WHEN** a service registers a delivery callback and the client disconnects before the reply is sent
- **THEN** the callback is invoked once with not delivered

#### Scenario: Send fails
- **WHEN** a service registers a delivery callback and writing the reply to the socket fails or is short
- **THEN** the callback is invoked once with not delivered, and the failure is logged

#### Scenario: Error reply
- **WHEN** a service registers a delivery callback and marks the call failed
- **THEN** the callback is invoked once with not delivered

#### Scenario: Delivered but client dies afterwards
- **WHEN** the reply is written in full and the client process exits before reading it
- **THEN** the callback reports delivered, and cleanup is left to the disconnect handling

#### Scenario: No callback registered
- **WHEN** a service completes a call without registering a delivery callback
- **THEN** the reply is sent exactly as before this change
