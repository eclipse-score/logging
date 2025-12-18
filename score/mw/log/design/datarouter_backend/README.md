# Safe Datarouter Backend

- [Safe Datarouter Backend](#safe-datarouter-backend)
  - [Introduction](#introduction)
  - [Inter-process communication](#inter-process-communication)
  - [Class diagram](#class-diagram)
  - [Activity diagrams](#activity-diagrams)
  - [Lock-free Shared Memory Design](#lock-free-shared-memory-design)
  - [Lazy logging client](#lazy-logging-client)
  - [Establishing a logging session](#establishing-a-logging-session)
  - [Avoiding shared memory leaks](#avoiding-shared-memory-leaks)
  - [Post-mortem logging](#post-mortem-logging)
  - [Limited impact of incoming messages in the logging client](#limited-impact-of-incoming-messages-in-the-logging-client)
  - [Message exchange between Logging Clients and Datarouter](#message-exchange-between-logging-clients-and-datarouter)
    - [Establish a new logging session](#establish-a-new-logging-session)
      - [Request from the client to Datarouter](#request-from-the-client-to-datarouter)
    - [Acquire data to read on the ring buffer](#acquire-data-to-read-on-the-ring-buffer)
      - [Request from Datarouter to acquire](#request-from-datarouter-to-acquire)
      - [Response from the Client to confirm acquire](#response-from-the-client-to-confirm-acquire)

## Introduction

The `mw::log` implementation development goal is to reach ASIL qualification and
to ensure freedom of interference of logging with customer functions. The
logging mode `kRemote` in `mw::log` transports the logs from the client process
to Ethernet via the Datarouter process. The design goals for the communication
are as follows:

1. Lock-free and wait-free: Logging calls from the client context shall not
   block. The client context shall not wait on other log calls from client
   context, neither on the internal logging thread.

2. Deterministic memory management: Dynamic memory allocations are only allowed
   during the initialization phase. The implementation shall preallocate all
   necessary memory upon the first invocation of the logging API.

3. Protection of safety-qualified logging client processes from non-qualified
   Datarouter process. The logging library shall validate and treat inputs from
   Datarouter as untrusted.

## Inter-process communication

In the Datarouter backend the goal is to safely and efficiently transfer the log
messages from the user process to the Datarouter process.

The inter-process communication takes place via shared memory and the [message
passing library](../../../com/message_passing/design/README.md). This library
only supports unidirectional message transport. As we need bidirectional
communication over the side channel, we establish two independent unidirectional
channels. On the highest level of abstraction we see the ASIL-B qualified client
process on the one side and the datarouter process on the other

![Inter-process communication](https://www.plantuml.com/plantuml/proxy?src=https://raw.githubusercontent.com/swh/ddad_score/mw/log/design/datarouter_backend/inter_process_communication.uxf?ref=1f63d6572c73339987778857d3e2f35b831d877b).

The logs are written by the client into shared memory and read-out by
datarouter. Freedom of interference is ensured since the datarouter process has
read-only access to the shared-memory. In order to reuse the logging buffers in
shared-memory the client may overwrite them after datarouter has acknowledged it
has successfully read them. This acknowledgement is sent via the
safety-qualified message passing library.

## Class diagram

![Class diagram](https://www.plantuml.com/plantuml/proxy?src=https://raw.githubusercontent.com/swh/ddad_score/mw/log/design/datarouter_backend/class_diagram.uxf?ref=a294e8aeb6453344591cfb1780d719f4a5ea1daa)

The class diagram above shows the relevant classes client-side and in
Datarouter. Client-side the `DatarouterBackend` contains a circular allocator
for the preallocation of the verbose messages. When a new verbose message is
created a slot is reserved first for the user in the circular allocator. The
memory inside the slot is preallocated and lives in the process space of the
client. When the user flushes the verbose message, the content of the slot is
copied into the ring buffer that lives in shared memory through the `TRACE()`
macro. The shared memory buffer is encapsulated by the `SharedMemoryWriter`
class. It is owned by the `platform::Logger` as a Singleton. For non-verbose
logging, or structured logging `mw::log` is not involved, but `TRACE()` is used
directly. The `TRACE()` macro internally calls
`SharedMemoryWriter::AllocateAndWrite()`, which locks a client-side only mutex
and writes the input structure in serialized form into the ring-buffer. When
Datarouter is reading from the ring buffer the data shall not be overwritten.
The responsibility of the `DatarouterMessageClient` class is to synchronize the
access to the ring buffer between Datarouter and the client.

## Activity diagrams

![DataRouterBackend::DataRouterBackend Activity diagram](https://www.plantuml.com/plantuml/proxy?src=https://raw.githubusercontent.com/swh/ddad_score/mw/log/design/datarouter_backend_datarouterbackend.uxf?ref=5e05cd648f508d14acff7fc405f7cba93c4c5dff)

## Lock-free Shared Memory Design

The log messages from apps are transferred to Datarouter through shared memory.
For lock- and wait-free data exchange we use the `WaitFreeAlternatingWriter` as
documented [here](../../detail/wait_free_producer_queue/README.md) as part of
the `SharedMemoryWriter` class. The data in shared memory is layed out in
consecutive and contiguous segments:

1. Control structure `mw::log::detail::SharedData` at offset = 0.
2. Linear buffer one.
3. Linear buffer two.

Datarouter shall be able to open this file read-only. The control structure
contains fundamental information about the buffer sizes, and the atomic
variables.

The `SharedMemoryWriter` has two main responsibilities. First is to register
data types using `TryRegisterType()` and secondly to push instances of those
types using `AllocAndWrite()`. Both aspects use the `WaitFreeAlternatingWriter`
to ensure lock-free writing of data.

Before a data type can be used it needs to be successfully registered. In the
registration, the `SharedMemoryWriter` send the fully qualified type name and a
corresponding numeric type identifier.

This identifier is then used by `AllocAndWrite()`. For each call, the method
tries to acquire one contiguous area of memory on the current linear buffer.
This area then contains two sub-segments:

1. The header `mw::log::BufferEntryHeader` which contains a timestamp and type
   identification.
2. Followed by the payload as written by the user.

## Lazy logging client

The logging client shall remain in the passive role in the message exchange with
Datarouter, i.e., it should react and respond to messages from Datarouter. The
reason for that is that we restrict the client to only a single thread for
logging to save resources. In addition, there shall be no messages sent from a
user context in the client. By user context we mean an public API call to
logging of any user thread that is not the logging thread. Sending a message
with the `mw::com` `Sender` results in a context switch to the receiver (i.e.
Datarouter). This operation could block for an undefined period of time. This
would violate our requirements and design goals of not blocking the user of
logging. Hence we only allow message sending in the logging thread in the client
process. The only exception is during the initialization phase. Here, the client
may send a connect message to Datarouter.

## Establishing a logging session

Before the information exchange between Datarouter and the Logging client can
begin, a session needs to be established. By that we mean the two side-channels
for message passing need to be connected. The client needs to instantiate a
`Sender` that connects with the `Receiver` of Datarouter. Datarouter needs to
instantiate a `Sender` that connects to the `Receiver` of the client. In
addition, Datarouter has to open the shared memory file from the logging
client.

We need to support logging with all possible startup orders of the processes:

1. A logging client starts after Datarouter started.
2. A logging client started before Datarouter starts.

When a client starts, it creates a ring buffer in a shared-memory page and maps
it to a file. From that point on, the user is able to write logs in the
shared-memory ring buffer even in early startup phase. In the initialization
phase of the logger library, a connect message is attempted to be sent to
Datarouter. This is only possible if Datarouter is already running and ready to
receive. If a client exits before Datarouter has connected the resource must be
cleaned up and the logs are lost.

For each client, there shall be two files:

1. The shared memory file of the client `/tmp/logging.<appid>.<uid>.shmem`.
2. The receiver file of the client `/mw_com/logging.<appid>.<uid>`.

The shared memory file shall be writable only for the UID of the client, and
read-only for everything else.

On startup, Datarouter creates its own receiver and attaches it to a file:

- `/mw_com/logging.datarouter_recv`

After Datarouter creates its receiver the logging clients are unblocked and
establish the connection by sending the connect message.

## Avoiding shared memory leaks

The logging library should ensure the shared memory resources are freed when no
longer needed to avoid leaks. Logging clients may exit and restart multiple
times. The shared memory file that contains the ring buffer read by datarouter
can only be cleaned by the client and not by Datarouter due the restrictive
permissions. Hence, the logging client shall unlink the shared memory file when
it receives the first message from Datarouter. At this point it is ensured that
Datarouter has already opened the file and is able to consume it. This way the
resource is cleaned up as soon as all processes have closed the file.

In addition, files created by the logging client shall be unlinked on exit. Due
to this design decision leaks should be avoided, but if an app exits before
Datarouter can establish the connection the logs are lost. If, however, the
client crashes before being able to unlink the file, the resources cannot be
freed. After a crash the app may be restarted. Thus, on startup, the client
shall check for shared memory instances of previous runs and unlink them as well
to fix the leak.

## Post-mortem logging

Datarouter should completely retrieve the all the logs of an application even
after that app exited. The only condition, as explained in the previous section,
is that Datarouter already established a connection to the app. Datarouter
should detect that an app is terminated if sending a message to that app fails.
At this point, Datarouter should switch the `SharedMemoryReader` to detached
mode. In detached mode, the reader assumes all data remains unchanged and thus
readable without synchronization. Only after reading the remaining data from the
ring buffer, should Datarouter unmap the shared memory page. Then the OS should
free up the used resources.

![SharedMemoryReader Class diagram](https://www.plantuml.com/plantuml/proxy?src=https://raw.githubusercontent.com/swh/ddad_score/mw/log/design/mw_log_shared_memory_reader.uxf?ref=a294e8aeb6453344591cfb1780d719f4a5ea1daa)

## Limited impact of incoming messages in the logging client

Consider a fault in Datarouter that leads to permanently repeated message
sending to one of the logging clients in a hot loop. First of all, this would
not directly impact the user threads of the clients but the logger thread in the
client process. The logger thread is spawned by the thread pool (containing a
single thread) in the `DatarouterMessagePassingClient` in `DatarouterBackend`.

The implementation shall limit the impact of such a "denial-of-service"-like
situation on the client threads. First, the lock-free implementation of the
shared memory buffers ensures that the logger cannot block the running operation
of a client writing to the shared memory. It is always the reader i.e. the
logger thread and Datarouter that on the writers to finish, and not vice-versa.

Secondly, we also mitigate CPU starvation of the clients by limiting the maximum
number of requests a logger thread may execute in a fixed time interval. This
rate limitation is implemented with a fixed sleep period in the logger thread
after each incoming request.

## Message exchange between Logging Clients and Datarouter

The side channel in logging is needed to synchronize the access to the data in
shared memory between the client and Datarouter. Previously, the channel was
implemented using unix domain sockets. Here, we replace unix domain sockets by
[message passing](../../../com/message_passing/design/README.md).

Datarouter periodically scans for new clients and initiates the message
exchange. In the message exchange between Datarouter and the Clients,
Datarouter always takes the active role. Datarouter sends a request and the
Client replies with a response to the request.

There is only a single receiver in Datarouter for all clients. Each message
from `mw::com::MessagePassing` includes a message id and a PID of the sender
besides the Payload. We use the `ShortMessage` and `MediumMessage` type for all
messages, which can currently hold a payload length of 8 Bytes and 16 Bytes,
respectively. The PID is used as an identifier for the concrete logging client
in Datarouter.

### Establish a new logging session

When a logging client starts, it sends a connect message to Datarouter. This is
only possible if Datarouter is already running. If Datarouter is not yet running
sending the message will block until Datarouter becomes ready to receive
messages. If the application shuts down before receiving the connect message,
the logs of that application will be lost.

#### Request from the client to Datarouter

Message ID: `kConnectToClient`

Payload: `struct ConnectMessage`

This is the only message that shall be initiated by the client itself. The
message shall contain the necessary information for Datarouter to find the
shared memory file location of the client and establish the corresponding
message passing channel.

There are two modes that a client could use:

1. Deterministic identifier mode that is is used in production.
2. Dynamic identifier mode that is used only for testing purposes.

In deterministic mode the client is identified using its DLT app id and user id.
By system design, each process shall have an unique uid so that there should be
no collisions.

Dynamic mode is provided for testing purposes where we cannot ensure a unique
app id and user id. Instead it uses a random unique 6 character long identifier
that is obtained using `mkstemp()`. Enabling this mode in production is
prevented by secpol enforcement as the message passing channel path is fixed by
secpol policy.

### Acquire data to read on the ring buffer

In a dynamic periodic interval, Datarouter request to acquire the current data in the
ring buffer from a client. This allows to synchronize reading the ring buffer in
shared memory and prevents that the client modifies that data during reading.
Each acquire request corresponds to toggling the linear buffer for reading and
writing. The interval depends on the condition of non-zero number of bytes written
into the buffer. Datarouter sends request to Acquire the buffer only if the write
buffer is not empty or every 10th constant interval check finished with the
decision not to request the switching. Every 10th request is executed in a
keepalive manner to check the communication link.

#### Request from Datarouter to acquire

Message ID: `kAcquireRequest`

Payload: None

#### Response from the Client to confirm acquire

When the data is acquired on client side by a call to
`AlternatingReader::Switch()`, the client confirms that by responding. The
response contains the range in the ring buffer that is ready to be read as well
as the updated type index. When a user traces a data type for the C++ fully
qualified type name is put on the type stack in the `MwsrWriter`. This allows
Datarouter to identify the payload in the ring buffer by index to the type
stack.

Message ID: `kAcquireResponse`

Message type: `MediumMessage`

Payload: `struct ReadAcquireResult`

