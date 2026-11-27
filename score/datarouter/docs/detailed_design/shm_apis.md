<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->

# Remote Logging implementation details

Remote Logging uses the **message passing** library (`MessagePassingServer` / `MessagePassingClient`) as the session channel between `mw::log` producers and DataRouter. It is used for log session registration (`kConnect`), shared memory buffer acquire requests (`kAcquireRequest` / `kAcquireResponse`), and disconnect handling. Shared memory is the data channel.

For communications between a producer and the DataRouter, the producer is the writer and the DataRouter is the reader.

The writer process creates the shared memory file, sizes it and allocates the memory.

The reader accesses the shared memory in read-only mode using ```mmap()``` syscall with the file name reconstructed based on data passed via connect message.

The writer uses lock-free atomics in the linear control block to avoid race condition between its threads when accessing the buffer.
The writer switches the double buffers assigned for writing and for reading following request sent from the reader via message.
The reader requests the switch of the buffer either when it has finished reading or after a timeout has occurred.
The writer drops log messages if the active write-buffer is full.
Under no circumstances, the writer will be blocked by the reader.

When the producer starts it creates the shared memory buffer (for example, as a [Meyer's singleton](http://laristra.github.io/flecsi/src/developer-guide/patterns/meyers_singleton.html)) and can start writing into it immediately from multiple threads, if desired. The logic of starting a control channel for this buffer and connecting it to the DataRouter is separate and asynchronous. The message passing client for the control channel runs in a separate dedicated thread.

The control channel detects disconnects of its peer and reconnects automatically if DataRouter restarts unless the connection attempt fails repeatedly. If the producer exits, DataRouter only closes (and thus frees) the shared memory region when its content fully fed to DataRouter's filters.

## APIs in use

System/library calls used in particular (as implemented at the moment):

### At a Producer

In static mode, the writer process creates the shared memory file using ```open()``` syscall as ```/tmp/logging.<app_id>.<uid>.shmem```.
In dynamic mode, the writer process creates the shared memory file using ```mkstemps``` as ```/tmp/logging-<random>.shmem``` and reopens it using ```open()```.
When opening the file, the writer uses the flags O_RDWR | O_EXCL | O_CLOEXEC (+ O_CREAT in static mode).
In both cases, the writer sizes the file using ```ftruncate()``` and allocates the memory using ```mmap()``` syscall with flags ```(PROT_READ | PROT_WRITE, MAP_SHARED)```.

In a separate thread starting later, a Producer
- creates a message passing client.
- sends a connect message with identifier ```DatarouterMessageIdentifier::kConnect``` and app id and uid.
- keeps listening the message passing channel for acquire requests.

### At the DataRouter

DataRouter will setup a message passing server. At each connection, DataRouter does the following:
- reconstruct the shared memory file name from the connect message fields as /tmp/logging.<app_id>.<uid>.shmem (or /tmp/logging-<random>.shmem in dynamic mode) and open it with ```O_RDONLY```,
- calculate the size of the shared memory region using ```fstat()```,
- map the whole region using mmap() with the flags ```(PROT_READ, MAP_SHARED)```.

DataRouter keeps the message passing channel monitored via a worker thread; once each message received (or once each 100 ms timeout) it processes the tick queue and reads the ring buffer content.

If the control channel disconnects, DataRouter reads the content of the ring buffer for the last time, then unmaps the shared memory and destroys the session. The server continues listening for new connections, so a reconnecting client establishes a new session.
