# Remote Logging implementation details

Remote Logging uses the **message passing** library (`MessagePassingServer` / `MessagePassingClient`) as the session channel between `mw::log` producers and DataRouter. It is used for log session registration (`kConnect`), shared memory buffer acquire requests (`kAcquireRequest` / `kAcquireResponse`), and disconnect handling. Shared memory is the data channel.

For communications between a producer and the DataRouter, the producer is the writer and the DataRouter is the reader.

Shared memory is allocated using ```open()``` syscall by the writer process, and a link is created as ```/tmp/logging.<app_id>.<uid>.shmem``` (static mode) or ```/tmp/logging-<random>.shmem``` (dynamic mode, when no app ID is available). At the moment, the reader can (and does) modify the shared memory; in the case of communications between ASIL D writers and ASIL QM reader it can be avoided by requesting such modifications through the control channel; in this case, the fd handle returned by ```open()``` could be reopened as a read-only fd handle (for the filename /proc/self/fd/<fd>) and this reopened handle passed to the reader process, thus guaranteeing that the ASIL QM process cannot modify the memory shared with ASIL D processes.

The consistency of write and read pointers in the shared buffer is protected by a POSIX mutex. A writer takes the mutex for the time of writing a single message record. A reader takes the mutex to read the read and write pointer, and also takes the mutex to advance a reader pointer toward the previous value of the write pointer after the reader processes the existing records. During reading, the reader does not hold the lock. The writer is able to detect if reading is happening at the moment. If the buffer is full, the writer either advances the reading pointer if there is no reading happening, or discards the writing attempt if reading is happening. The writer never waits for the reader to finish reading.

At the moment, the mutex is kept in the shared memory. If we need to make the memory read-only for the writer, the reader needs to request access to the read pointer value through the control channel. The the writer process will fully own the mutex both physically and logically (the mutex is still briefly locked before and after reading, but is not held during reading).

When the producer starts it creates the shared memory buffer (for example, as a [Meyer's singleton](http://laristra.github.io/flecsi/src/developer-guide/patterns/meyers_singleton.html)) and can start writing into it immediately from multiple threads, if desired. The logic of starting a control channel for this buffer and connecting it to the DataRouter is separate and asynchronous. The message passing client for the control channel runs in a separate dedicated thread. The control channel detects disconnects of its peer and reconnects automatically if DataRouter restarts. If the producer exits, DataRouter only closes (and thus frees) the shared memory region when its content fully fed to DataRouter's filters.

## APIs in use

System/library calls used in particular (as implemented at the moment):

### At a Producer

Producers obtain the fd of the shared memory with ```open()``` with flag ```O_RDWR``` (read-write, as the producer both creates and writes the shared memory region).

Then a Producer would ```ftruncate()``` the obtained descriptor to the required size.

Then ```mmap()``` the whole content of the memory region with flags ```(PROT_READ | PROT_WRITE, MAP_SHARED)```, initialize it.

In a separate thread starting later, a Producer
- creates a message passing client.
- sends a connect message with identifier ```DatarouterMessageIdentifier::kConnect``` and app id and uid.
- keeps listening the message passing channel for acquire requests.

If the message passing connection is lost, the message passing library handles reconnection transparently.

### At the DataRouter

DataRouter will setup a message passing server. At each connection, DataRouter does the following:
- receive the shared memory file by constructing from the message as ```/tmp/logging.<app_id>.<uid>.shmem``` (or ```/tmp/logging-<random>.shmem``` in dynamic mode),
- calculate the size of the shared memory region using ```fstat()```,
- map the whole region using mmap() with the flags ```(PROT_READ, MAP_SHARED)```.

DataRouter keeps the message passing channel monitored via a worker thread; once each message received (or once each 100 ms timeout) it processes the tick queue and reads the ring buffer content.

If the control channel disconnects, DataRouter reads the content of the ring buffer for the last time and then close its file descriptor. Then DataRouter wait for the reconnect from the client.
