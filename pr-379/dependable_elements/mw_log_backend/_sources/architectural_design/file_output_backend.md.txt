<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->

# FileOutputBackend

```{document} mw::log File Output Backend Detailed Design
:id: doc__mw_log_file_output_backend_design
:status: draft
:version: 1
:safety: QM
:security: YES
:realizes: wp__sw_implementation
```

`FileOutputBackend` implements `mw::log::detail::Backend`. It is responsible for
providing buffers by means of `ReserveSlot()` call and writing data to the file
descriptor passed to its constructor when the buffer is returned by the user by
means of calling `FlushSlot()`. It assumes non-blocking calls and implements
measures to store and manage buffers till all the data is pushed out in
subsequent calls. Logs are stored in a circular buffer in the case when the
current `LogRecord` is not completely flushed due to the file descriptor not
accepting all the data. The rest of the data is to be pushed in subsequent calls
to `ReserveSlot()` and `FlushSlot()`, which both start by trying to flush the
pending data, looping over a circular buffer to reduce the queue of unprocessed
logs in the order provided by the caller. Whenever each `LogRecord` is flushed
completely, the buffer is released to the allocator by `ReleaseSlot()`.
In case of depletion of a fixed number of buffers, none is returned to the
caller rather than overwriting old ones.
The zero-copy approach was used inside the backend design. Circular buffers
store objects of `SlotHandle` type which is just a handle to buffers storing
data. It is part of the job of the Recorder to format and copy content into the
buffers.
The zero-copy principle does not hold for the data provided at a level of and
above Recorder.
Configuration of the backend is done through resources passed down in
constructor arguments consisting of the message builder, the file descriptor,
the slot allocator and the OSAL `Fcntl` and `Unistd` objects. The constructor
sets the file descriptor to non-blocking and close-on-exec. The file is opened by
`FileRecorderFactory`, which also creates the `DltMessageBuilder`.

Design decision has been made to make backend independent of loosely coupled
to details of data it processes which is presented by `LogRecord` type. To do
so object providing `IMessageBuilder` interface is passed down by the recorder
factory to allow access data in polymorphic way. This interface makes it possible
to access set of sequential data buffers in order they should be serialized into
a file or onto the console.
`DltMessageBuilder` (file) and `TextMessageBuilder` (console) implement
`IMessageBuilder`. They are responsible for serving data in correct order of
flushing and store some of common parts of the header.

```{uml} _assets/mw_log_file_backend.puml
```

`SlotDrainer` is responsible for storing and disposal of already serialized
data. First data gets inserted into circular buffer. After that step program
flow enters a loop that iterates over available slots in ring buffer and then
by means of `NonBlockingWriter` iterates over all spans of each message.
A flush first continues to write the pending data of the current message, then
releases its slot and loads the next message.

Flush procedure exits whenever all available data is written to the file, the
writing procedure would block i.e. write operation reports that number of bytes
written is less then requested, or the number of slots processed in one cycle
(`limit_slots_in_one_cycle`, default 32) is exceeded.
