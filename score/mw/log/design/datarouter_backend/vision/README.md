# Safe Datarouter Backend: Vision

This document describes how we envision the future development of the
implementation of the Datarouter backend for logging. The actual design to be
implemented is described in [here](safe_datarouter_backend.md).

- [Safe Datarouter Backend: Vision](#safe-datarouter-backend-vision)
  - [Introduction](#introduction)
  - [Comparison to the current design](#comparison-to-the-current-design)
  - [High Level Design](#high-level-design)
    - [Types of Logs](#types-of-logs)
    - [Data structures](#data-structures)
  - [Data flow](#data-flow)
    - [Initialization of Logging](#initialization-of-logging)
    - [Log message processing](#log-message-processing)
      - [Verbose log messages](#verbose-log-messages)
      - [Non-Verbose log messages](#non-verbose-log-messages)
    - [Disconnect of the Logging client](#disconnect-of-the-logging-client)
  - [Handling of non-verbose record identifiers](#handling-of-non-verbose-record-identifiers)
  - [Appendix](#appendix)
    - [Design notes `VerboseMessageWriter`](#design-notes-verbosemessagewriter)
    - [Sketch of how to implement the `VerboseMessageWriter`](#sketch-of-how-to-implement-the-verbosemessagewriter)
    - [Sketch of how to implement the `NonVerboseRecordWriter`:](#sketch-of-how-to-implement-the-nonverboserecordwriter)

## Introduction

The `mw::log` implementation development goal is to reach ASIL qualification and
to ensure freedom of interference of logging with customer functions. The
logging mode `kRemote` in `mw::log` transports the logs from the client process
to Ethernet via the `Datarouter` process. The design goals for the communication
are as follows:

1. Zero-copy: Log messages shall be serialized once and saved in the destination
   location without further copies on the client side.

2. Lock-free: Logging calls from the client context shall not block. We prefer
   lock free implementation when feasible. If we have to use a mutex, the critical
   section shall be upper bounded and priority inheritance shall be used on the mutex.

3. Deterministic memory management: Dynamic memory allocations are only allowed
   during the initialization phase. The implementation must preallocate all
   necessary memory upon the first invocation of the logging API.

4. Protection of safety-qualified logging client processes from non-qualified
   `Datarouter` process.

## Comparison to the current design

With the lightweight approach, we will reach the safety requirement of removing
unix domain sockets. In comparison, this approach has the following advantages
at the cost of much higher effort:

1. Performance: Because we cannot change the shared memory data structures,
   verbose logging will still be copied twice and serialized. Both was avoided
   in the original design. With the new approach performance situation will not
   improve, memory consumption will not be decreased. We also change the tooling
   minimizing the load a client has for initialization of the logging library.

2. Not Lock-free for the user context: Verbose logging will not become lock-free,
   multiple concurrent verbose logs need will remain sequential in a critical
   section using a mutex.

3. Coupling of ASIL-B and QM use cases: Subscriber and Logging use cases are
   coupled within the MWSR classes. Changing these data structures always risks
   breaking either logging or subscribers. This design solves this by clearly
   separating ASIL code from QM code, and moving logging library code in a
   self-contained package in `mw::log`. With the lightweight design, we only
   focus on replacing unix domain and the shared memory structures (mwsr writer)
   will remain mostly unchanged and kept in the logging library.

## High Level Design

The communication between the logging client (datarouter backend) and
`Datarouter` shall take place via shared memory and the [message passing
library](../../../com/message_passing/design/README.md). This library, however,
only supports unidirectional message transport. Hence, the design allows to
achieve the logging requirements without the need for a back-channel. On the
highest level of abstraction we see the ASIL-B qualified client process on the
one side and the datarouter process on the other ![Component
view](./score/mw/log/design/datarouter_backend/vision/overview.puml).
The logs are written by the client into shared memory and read-out by
datarouter. Freedom of interference is ensured since the datarouter process has
read-only access to the shared-memory. In order to reuse the logging buffers in
shared-memory the client may overwrite them after datarouter has acknowledged it
has successfully read them. This acknowledgement is sent via the
safety-qualified message passing library.

### Types of Logs

We distinguish between two fundamentally different types of logs a client may
want to trace.

- *Verbose messages*: These are 'short' human-readable messages which are
  (de-)serialization and possibly converted into the DLT-format. The creation,
  i.e. the composition, of these messages may take a long time.
- *Non-verbose records*: These are 'long' messages containing arbitrary
  datatype. They are optimized to be processed by machines. As also indicated by
  their name a record contains additional metadata 'header' and the 'payload'
  data.

The objective from a logging perspective is the same for both types. Yet, the
mentioned differences required distinct handling of each type. For example, the
datatype of a verbose message is known at compile-time, compared to the payload
datatype in a non-verbose record. Hence, to correctly de-serialize the latter
both the logging client and the datarouter process need to agree on a mapping
from datatype to serializer. This is achieved via identifiers and parsing of
common `.json` files as [described
below](#handling-of-non-verbose-record-identifiers)

### Data structures

Naturally, the communication to `datarouter` is encapsulated within the
`mw::log` package of the logging client. This minimizes coupling with the
frontend. Similarly, the backend is uncoupled on datarouter side as illustrated
in the component view ![Component
view](./score/mw/log/design/datarouter_backend/vision/component_diagram.puml).
The `DatarouterBackend` and `Datarouter` are the linking points of this design
into the [the larger logging package](../README.md). Note, possibly small
(internal) API changes must be introduce to obtain a seamless link. The
dedicated mechanisms for the [types of logs](#types-of-logs) become apparent by
the respectively named readers and writers. For the three building-block of the
communication (shared-memory, side-channel, and `.json` file reading)
special-purpose libraries free the design from bare-metal concerns such as
file-handling or establishing the communication link. The libraries respective
assumptions of use were checked and suffice for this high level design. A more
thorough analysis is done in the detailed design.

Looking into the components in more detail we see the different types of logs
also reflect in two data structures allocated in shared-memory. Verbose messages
are stored in `VerboseMessageContainer`. It contains a circular buffer with a
predefined number of slots with predefined size is used. The slots allow for
writing messages in parallel by multiple threads. Since composing the message
may take an arbitrary amount of time a FIFO-queue is used to maintain the order
of messages while logging. The slot-buffer and the FIFO-queue are (exclusively)
altered by the `CircularAllocator` and `Fifo` classes, respectively. Both these
classes may be implemented lock-free by using the atomic indices provided by the
container.

In contrast to verbose messages which are always of type `LogEntry`, non-verbose
records may contain arbitrary types. Hence, they comprise of metadata 'header's
and 'payload' data. The records are stored into a circular buffer by
`NonVerboseRecordWriter`. Due to the unknown length of each record parallel
writes into the buffer are prohibited. Therefore, the
`SynchronizedNonVerboseRecordContainer` guards write accesses to the underlying
container. Nevertheless, read-access and erasure of elements is possible without
locking via atomic indices provided by the container.

On datarouter side the situation is mirrored. For both verbose and non-verbose
logs a respective reader class is provided. Since datarouter has read-only
access to the shared-memory it cannot erase read-elements from the buffers
directly. Rather, the erasure is done by the logging client. The client receives
the message of how much data is has been read and may be freed via a
`score::mw::com::message_passing` channel. Accordingly, the
`SafeDataRouterBackend` and the `LogClientReader` hold an
`score::mw::com::message_passing::IReceiver` and
`score::mw::com::message_passing::ISender` instance, respectively. Details on the
communication are given in the [next section](#data-flow). Datarouter contains
one `LogClientReader` for each connected client. ![High Level
design](./score/mw/log/design/datarouter_backend/vision/high_level_design.puml).

## Data flow

In the following sequence diagrams for the initialization and initial
connection, the logging of verbose messages and non-verbose records as well as
the case of a disconnecting client are shown. Additionally, consequences of
failures are evaluated.

### Initialization of Logging

Initialization starts for both `Datarouter` and the `SafeDatarouterBackend` side
by reading out the respective `.json` files as [described
below](#handling-of-non-verbose-record-identifiers). In case of failure to
read-in a data type from file, logs of this file type will be dropped.
Datarouter is then already ready and starts polling for new clients to connect
to. It does so by monitoring a predefined directory within shared-memory. If a
new file is found the respective `LogClientReader` is initialized. It then
creates the actual readers and starts the message passing sender which tries to
connect to the respective receiver. If any of these initializations fail
datarouter will retry a certain number of times before giving up on this
particular client. Note, due to the timestamp in the shared memory file name a
restarted client is detectable and the reader only connects to the newest client
instance.

After reading the data types from file and instantiating the respective look-up
map, the client proceeds to allocate the containers. Therein, it checks and
handles orphaned shared-memory files associated to this client (identified by
the app name). Then, the respective writer instances and finally the receiver
for the message passing channel are initialized. If any of these operations fail
they can be retried or aborted. In the latter case the logging client should
shutdown or restart altogether.

![Initialization sequence diagram](./score/mw/log/design/datarouter_backend/vision/initialization_sequence_diagram.puml)

### Log message processing

Although, the processing sequence of verbose and non-verbose logs is similar,
they are discussed separately to de-clutter the diagrams and highlight the
differences. Note, the dedicated 'logging thread' in both cases is indeed the
same thread, i.e. only one logging thread exists per logging client process.
This logging thread prohibits the message passing library from blocking the user
thread of the logging client as well as datarouter as per the requirements of
the message passing library.

After successful initialization datarouter frequently polls for new verbose and
non-verbose logs alike. Therefore `FetchNewMessages` checks the read and write
indices of both container against each other and against local copies. The
latter helps detect erroneous buffer entries, unexpected restarts, dropped
messages and ensures any entry is only logged once. If new logs are available,
they are pulled out of the shared-memory by reference (respectively pointer).
The logs are grabbed one at a time `GetNextMessage` and processed according to
their type by the LogParser. Datarouter is intended to only shutdown at power
off. Nevertheless, in case of an unexpected restart it can reconnect and
re-fetch the logs. This, however, might lead to a few number of logs being
traced twice. For the logging client a restart of datarouter goes unnoticed. The
message passing receiver is indifferent to disconnecting receivers and is always
open for 'new' connections. Since datarouter has read-only access to the
shared-memory a restart also has no effect in this regard, only the buffers will
overflow at a certain point in time and logs will be dropped directly on the
client side.

#### Verbose log messages

When writing verbose messages the corresponding slot to write into is directly
exposed to the user. As precaution the user only get the slot after a successful
call to `AcquireSlot()` and after finishing his message has to release the slot
with the respective function. Writing into the slot is also guarded, i.e.
out-of-bounds writes ore re-allocations are prohibited. After the slot is
released, the FIFO-queue is updated accordingly. As soon as the valid indication
is set datarouter may read-out this FIFO-element and the corresponding slot on
its next periodic polling of newly available logs.

As mentioned above, the message passing receiver runs in the dedicated logging
thread and awaits incoming messages from datarouter. The incoming messages tell
the logging client by how much the read index may be moved. Thereby these logs
are 'erased' as they are free to be overridden. Lets consider the case where
datarouter goes berserk and sends meaningless messages. The `Erase` method
handles unplausible inputs gracefully. I.e. in the worst-case it may interpret a
faulty input such that it erases all elements. Hence, the damage is restricted
to these incorrectly deleted logs.

![Verbose message sequence diagram](./score/mw/log/design/datarouter_backend/vision/verbose_sequence_diagram.puml)

#### Non-Verbose log messages

In the non-verbose case the logging is started via the `TRACE` macro. This macro
maps to the `Trace` function of `SafeDataRouterBackend`. This function then
provided the correct serializer callback for the data type of the passed object,
given the type was registered for logging during
[initialization](#initialization-of-logging). If there is still space in the
buffer of `NonVerboseRecordContainer` (checked lock-free), then the buffer is
locked via the `SynchronizedNonVerboseRecordReader` and the respective buffer
location is exposed to the serializer (callback). Naturally, the serializer's
implementation must honors the bounds of the provided buffer location (span).
Design of the non-verbose public API must be aligned with the safety
qualification of the serializer, cf.
[Appendix](#sketch-of-how-to-implement-the-nonverboserecordwriter)

After the serializer completes, the record's header is adjusted to reflect the
actual size of the payload (may be zero if serializer errors-out). Afterwards,
the remaining size in the buffer is checked and the `NonVerboseRecordStatus` is
flipped to `kValid` or `kValidLast`, accordingly. Finally, the write index is
updated and the lock is freed. Thus the new records is able to be picked-up and
processed by datarouter.

On datarouter side, the record handling is analogous to the verbose case. In
predefined intervals, datarouter checks the read and write indices against local
copies in its `FetchNewMessages` function. If unexpected inputs occur, e.g. the
read index is not on-par with the local copy a respective error message of
possibly lost logs is logged and the local indices are set according to the
atomic ones in shared-memory. In the valid state, the read index is smaller or
equal the write index. For equality there is no message to be queried as it
indicates no new records have been written since the last query. Otherwise, the
records are read out one at a time, first checking the `NonVerboseRecordStatus`
for validity. In the invalid case the record's payload stays unread and the read
index is not advanced. In the valid case the payload is read, de-serialized and
forwarded/traced according to the logging settings. Once this is done for all
newly available records the number of records read is sent via the message
passing library. This back-communication is equivalent (potentially even
combined into one message) to the verbose case. The `Erase` method in the client
again handles unplausible inputs gracefully; risking at worst a loss of logs.

![Non-Verbose message sequence diagram](./score/mw/log/design/datarouter_backend/vision/non_verbose_sequence_diagram.puml)

### Disconnect of the Logging client

In order to understand the case where a client is disconnecting first a quick
recap of the initialization is needed. The `SafeDatarouterBackend` is owned by
the `mw::log::Runtime` singleton which starts the (left-hand side of the)
initialization cascade [described above](#initialization-of-logging). Note, this
shall be the only singleton in the logging client. At the end of this cascade
the dedicated 'logging thread' is launched and used inside the message passing
Receiver, while the client thread(s) start working in their respective `main`s.

Before exiting `main()` the client shall call the `mw::log::Runtime::Shutdown()`
as a final step before returning from `main()`. The method is not thread-safe,
i.e., there all prior calls to logging APIs in all threads must join first
before calling this method. After the shutdown no more calls to logging APIs are
supported. The reason for this limitation is that libraries such as score::os,
message passing and shared memory are using internal singletons with static
lifetime. Unless those libraries are refactored to allow usage without singleton
the need to shutdown the logging library explicitly before the end of `main()`
is necessary.

In the destructor of the client `Runtime`'s destructor is called. This in turn
triggers a stop source command which closes the message passing receiver and
joins the logging thread with the client thread(s). After the join, the detached
state flag in the shared-memory resource is set. Afterwards the is unlinked by
the client, if datarouter has already discovered the file. The first message
received from datarouter indicates the discovery. If know message from
datarouter has been received yet the file is left untouched for datarouter to
pick it up (and trace the remaining logs) post-mortem. Datarouter tries to
clean-up (unlink) the shared resource if it only receives (a dedicated) error
code from the `mw::com::message_passing::Receiver`. After a possible restart of
the client the old resources is tried to be unlinked once more. Then the new
resource with the updated timestamp is created. By just unlinking shared-memory
resources known to datarouter it can finish tracing all messages still stored
within it. In case of a restarting client, datarouter will discovers the new
shared-memory resource and by the app name and the timestamp switch to the new
resource (after tracing the old one to completion). After this switch the
resources will be freed by the operating system, as it is not linked to by any
process.

![Client disconnect sequence diagram](./score/mw/log/design/datarouter_backend/vision/client_disconnect_sequence_diagram.puml)

## Handling of non-verbose record identifiers

For each non-verbose message type we send an unique 4 Byte identifier in the DLT
protocol. This allows the receiver to decode and deserialize the message.
Therefore, the client and `mw::log` needs to know during runtime which ID shall
be used for a message type based on JSON configuration files. Also, Datarouter
will use that ID to forward the message to subscribers. Previously the message
identifiers were stored in a single file for the whole system and each
application had to load that JSON file with more than 15k lines on startup. Now
we change that and extend the tooling to generate additional files, one for each
APPID. In addition, we will have one file for common identifiers, such as the
file transfer message.

The client then loads the following files on startup:

1. `/opt/datarouter/etc/common-class-ids.json`
2. `/opt/datarouter/etc/class-ids-<app-id>.json`

Datarouter continues to load the global `/opt/datarouter/etc/class-ids.json` as
it needs to know all the identifiers system wide. Each file consists of a single
root dictionary with the C++ type identifier as key and the message
specification as value:

```json
{
   "score::gw4a::Gw4aVersioninfoLogdata": {
            "id": 8388610,
            "ctxid": "gww",
            "appid": "Fasi",
            "loglevel": 4
   },
}
```

If the application specific file cannot be found, only the common config file
will be read. A configuration flag in `logging.json` shall be available to use
the global `class-id.json` instead, which could be used for testing e.g. in
SCTF.

The common `common-class-ids.json` shall at least contain the `score::logging::FileTransferEntry`:

```json
{
   "score::logging::FileTransferEntry": {
            "id": 1234,
            "ctxid": "DFLT",
            "appid": "DR",
            "loglevel": 4
   },
}
```

The generator of the configuration files shall ensure that there is no
duplication of message identifiers, as it was not the case so far.

## Appendix

The following notes are only drafts that sketch the basic ideas for the low
level design and need to be finalized in the detailed design.

### Design notes `VerboseMessageWriter`

`read_index_` and `write_index_` are continuously
incremented and may overflow.
To calculate an index we take
the counter modulo the buffer size.
Therefore, the buffer size must be power of two to
handle the overflow correctly.

The queue is full iff

```C++
write_index+1 mod size == read_index
```

Hence, we can only store `buffer size - 1` elements.

`VerboseMessageWriter::Push()`: use weak_compare_exchange() to
try to increment the end counter. If it succeeds
 the element can be inserted at the old write index.
Then set the valid flag. The valid flag is necessary because when
we update the write_index concurrent reads to the buffer could
occur before the element is written to the buffer.

`VerboseMessageWriter::Erase()`: First clear the valid flag then increment the read_index.
Shall handle the case gracefully if more elements are given
than currently available.

### Sketch of how to implement the `VerboseMessageWriter`

```C++
+ VerboseMessageWriter::TryWriteMessage( message ) {
   if( TryWriteIntoSlot( message ) ) {
      if( !TryPushToFifo() ) {
         return fail;
      }
   }
   return sucess;
}

VerboseMessageWriter::Erase(num_elements: size_t): void {
   FirstRemoveFifo
   ThenRemoveSlot
}

VerboseMessageWriter::TryWriteIntoSlot( message ) : score::cpp::optional<size_t> {
   slot = AcquireSlot(); --> return if empty
   GetUnderlyingSlot( slot ) = message;
   ReleaseSlot()
}
- TryPushFifo() :
```

### Sketch of how to implement the `NonVerboseRecordWriter`:

```C++
 TryPush( callback : score::cpp::function<size_t, (uint8*, size_t>) ) : PushResult {
 // class invariant: write_index fulfills alignment requirement of NonVerboseRecordHeader
 lock( mutex )
 if( read_index <= write_index && write_index + max_message_size < buffer.size() || //before_wrapping
     read_index >   write_index && write_index + max_message_size < read_index ) { // after_wrapping
     // e.g. extract if-condition into IsWriteSpaceAvailable function

   header_start_ptr = write_index;
   payload_start_ptr = header_start_ptr + sizeof( NonVerboseRecordHeader );
   payload_length = callback( payload_start_ptr, max_message_size );

   future_write_counter = AlignTo<NonVerboseRecordHeader>( write_counter + header_size + payload_length ).
   if( future_write_index >= buffer.size() ) { future_write_index = 0; }

   NonVerboseRecordHeader( ... ) -> save into header_start_ptr

   write_counter = future_write_index;
}
unlock(mutex)

}
```

Sketch of how to implement the `NonVerboseRecordReader`:

```C++
GetNextMessage() {
  valid_status = reinterpret_cast<Enum>( read_index,  read_index + sizeof( Enum ) )
if( invalid ) --> return
else {
  header = reinterpret_cast<header>( read_index + sizeof( Enum ),  + read_index + sizeof( Enum ) + sizeof( header ) )
 payload = reinterpret_cast<>( read_index + sizeof( Enum ) + sizeof( header, read_index + sizeof( Enum ) + sizeof( header ) + header_.message_size_ ) )

}


   msg_header = reinterpret_cast<>
  if( a.size() == 0 ) { return {} }
  else {
   return NonVerboseRecord{ a }
  }
}

FetchNewMessages() {
 local_write_index_ = data_.write_index_.load()
 local_read_ = data_.read_index_.load()
if( temp != local_read_ ) { print( ERROR ) proceed either way }
else { local_read_ = temp }

}
while( m = GetNextMessage != empty) {
 type =  m.Header().TypeInfo()
 parser.parse( m )
}

SendOnSideChannel( num_messages_read )
```

Sketch how to rewire TRACE-Macro:

```C++
template<typename UserType>
void Trace(const UserType& x){
    // Is UserType enabled by LogLevel.
    // 1. Get the log level and context assigned for UserType from class-id.json.
    // 2. Compare the log level for UserType with the configured log level for the context assigned to UserType.
    // 3. If the log level for UserType is greater than the configured context log level early return and drop the message.

    IRecorder recorder = mw::log::Runtime::instance().GetRecorder();
    if( recorder.IsTypeEnabled(user_type_pretty_name) ) {
      recorder.Trace(
         [&x]
         (span<uint8_t> reserved_space){
            return ::score::common::visitor::logging_serializer::serialize(x, reserved_space);
         }
      );
   else {
      //discard log
   }
}

// v This function is user responsibility to get ASIL test/coverage etc.
void Trace( MyType const& x ) {
   Trace<MyType>( x );
}

// v This function is user responsibility to get ASIL test/coverage etc.
void Trace( MyOtherType const& x ) {
   Trace<MyOtherType>( x );
}
```
