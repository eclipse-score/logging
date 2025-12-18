# Lock-free logging with Datarouter backend

- [Lock-free logging with Datarouter backend](#lock-free-logging-with-datarouter-backend)
  - [Problem statement](#problem-statement)
  - [Mutually exclusive ring buffer implementation](#mutually-exclusive-ring-buffer-implementation)
  - [Towards a lock-free implementation](#towards-a-lock-free-implementation)
    - [Solution A: Predictive Serializer](#solution-a-predictive-serializer)
    - [Solution B: Ring buffer array: One ring buffer for each critical thread](#solution-b-ring-buffer-array-one-ring-buffer-for-each-critical-thread)
    - [Comparison: implementation complexity VS configuration complexity](#comparison-implementation-complexity-vs-configuration-complexity)
  - [Conclusion](#conclusion)


## Problem statement

The logging public API is thread safe. It supports that users call logging from
multiple threads in parallel without external synchronization. Consider the
following example:

```C++
void Thread1(){
    LogInfo() << "Hello";
}

void Thread2(){
    LogInfo() << "Foobar";
}
```

In addition, the user expects that the execution of the logging call does not
block on other threads. This is formulated by the robust design measures
([SCR-861518][1]) requirements to ensure freedom from interference (FFI).

1. Cross-locking: Cross-application and cross-thread dependencies shall be
   avoided. A thread logging in a loop shall not block other thread's execution.
   ([SCR-861578][2])

2. Avoid locks: ara::log shall be free of time-unbound locks and shall implement
   strategies to limit wait time. Time limit shall be defined based on
   requirements from functions on individual ECU. Mutual exclusion mechanisms
   shall include priority inversion protection. ([SCR-1016724][3])

In the current implementation these requirements are violated because a mutex is
used to exclusively access a ring buffer. Thus it can be seen in applications
with many concurrent threads that invoke logging that there is mutex contention
and blocking:

![Trace from Ticket-47483](trace_blocking.png)

The trace from [Ticket-47483][4] illustrates this problem clearly: Both the
sensor_cleaning and condition_evalu threads are logging concurrently and
execution is blocking each other. Thus the runtime behavior is such as if the
threads are running on a single CPU core due to the mutex.

The implementation of the logging library needs to change to avoid such blocking
behavior and fullfil `SCR-861578` and `SCR-1016724`.

## Mutually exclusive ring buffer implementation

To understand how the logging library was implemented initially with a central
mutex we need to dive into the details. For a verbose message the information
flow looks like this.

1. Creation of LogStream instance

On the creation of LogStream instance a slot is reserved on in the private
address space of the app.

```C++
auto log_stream = LogInfo(...);
```

The slot memory is reserved by mw::log on
initialization. For the reservation of a slot a lock-free algorithm is used. The
algorithm in essence iterates over all slots and picks the first available slot.
If no slot is available, the log message is dropped. This algorithm is
implemented using atomics and therefore lock free and has a fixed upper runtime
bound.

```C++
log_stream << "my message";
```

The user then fills this slot with data that comprise the verbose
message.

2. Flush of the LogStream

```C++
log_stream.Flush();
```

When the user determines that the log message is complete the LogStream is
flushed. Then the content of the slot is copied into a ring buffer using
`MwsrWriter`. The ring buffer is allocated in a shared memory space and is read
by Datarouter. Datarouter is another process that has read only access to the
ring buffer via shared memory. The mechanism for flushing a log stream is same
as a sending a non-verbose message. In both cases the TRACE macro is used to
submit the message to Datarouter.

3. Inside the TRACE macro

```C++
TRACE(log_entry);
```

Up until this point the logic is without mutex and lock free. Inside the TRACE
macro we now enter the critical section:

```C++
logger::instancePtr->wr_.allocAndWrite([this, &t](auto data, auto size) {
        using s = ::score::common::visitor::logging_serializer;
        auto idsize = s::serialize(id_, data, size);
        return idsize + s::serialize(t, data + idsize, size - idsize);
    });
```

`allocAndWrite()` is a member of the `MwsrWriter` class that handles the access
to shared memory. It accepts a callback that provides a span to the reserved
 memory in the ring buffer. In pseudo code, `allocAndWrite()` works like this:

```C++
void allocAndWrite(timestamp_t timestamp, Callback callback) {
    lock();
    addr, available_space = ReserveMaximumRingBufferSpace();
    auto used_bytes = callback(addr, available_space);
    ShrinkRingBufferToActuallyNeedSpace(used_bytes);
    unlock();
}
```

Thus the `allocAndWrite()` function defines a critical section that is mutually
exclusive for all calls to the logging API. The implementation was done in this
way because the serializer cannot tell in advance how many bytes the output will
have. Thus a maximum message length is at first reserved in the buffer, then the
serializer runs and returns the actual length of the output. Finally the ring
buffer shrinks again to the actual used size. Obviously such an implementation
cannot work concurrently. As each message is directly placed after the
previous message in the ring buffer it needs to know where the previous message
ends. Thus it has to wait until the completion of the previous message.

## Towards a lock-free implementation

Changing the implementation to avoid mutex contention we have at least two options:

(A) Implement a predictive serializer that can output the needed capacity for a
message in advance before serialization.

(B) Introduce one individual ring buffer for each 'critical' thread.

### Solution A: Predictive Serializer

Since the serializer could predict how big a serialized message is going to be,
we can attempt to reserve that memory without blocking concurrent writers. Once
the memory is reserved concurrent writers can already reserve chunks directly
after the last reserved chunk. After a chunk is completely filled with the
serialized data, then we mark it as ready for reading by setting an atomic flag.
Using another atomic flag, the reader will mark a chunk as read. Then the
writing threads know that it is safe to reuse that memory for future entries
into the ring buffer.

![Single concurrent ring buffer](single_concurrent_ring_buffer.svg)

With this approach we would need to reach the following milestones

1. Use the serializer library to output a size prediction.
2. Refactor MwsrWriter and MwsrReader to replace the mutex with lock free algorithms.

Note that we could consider this approach without the predictive serializer. We
could allocate the maximum possible message length (around 64K) for each
message. The serializer could use the available space and the unused parts would
be left as padding. However, most messages only contain a few hundred bytes and
therefore reserving 64 K for each message would be a massive waste of RAM. As we
are strongly memory constrained on target, this simple but wasteful idea is not
feasible in practice.

### Solution B: Ring buffer array: One ring buffer for each critical thread

Alternatively we could consider introducing a ring buffer array for each
critical thread. By critical thread we mean threads that are defined by the user
that need the mutex-free behavior of the logging library. We forsee that not
each thread on the system will need such guarantees and thus we can save RAM by
minimizing the number of ring buffers we create.

![Ring buffer array](ring_buffer_array.svg)

With this approach we have the following milestones:

1. Refactor MwrsWriter and MwsrReader to support multiple ring buffers
   internally.
2. Adjust the side channel communication including Datarouter to support
   multiple ring buffers.
3. Extend the configuration to allow for multiple ring buffers with different
   sizes.
4. Apply the correct configuration for each user.

Some practical concerns come to mind with this approach. For example how to
identify a critical thread on runtime. This could be done through the thread
name, but the thread name could change dynamically. Also some threads are
implemented by third parties (e.g. AA stack). For those it is hard to identify
which threads should be considered critical, and thread names might change over
time, or new threads are added.

### Comparison: implementation complexity VS configuration complexity

The main advantage of solution A is that it would be memory efficient. With
multiple ring buffer some memory would be wasted, as each ring buffer has to be
sized for the worst case load in order to avoid drops.

One downside of solution A would be that the size prediction of the serializer
would cost some runtime performance. The serializer would probably run twice,
once in a 'visitor only mode' without copying any data just to determine the
size, and a second time doing the actual serialization. However, the visitation
step could be optimized further to reduce the runtime impact.

Regarding solution B the implementation effort would be smaller as we don't have
to change the serializer. However, there is a higher burden on the user to
correctly specify all the critical threads in the logging configuration. Thus we
expect a longer and more complex integration phase with this approach.

## Conclusion

Due to the advantages of Solution A in terms of memory consumption and ease of
use we move forward with solution A. The main challenges thereby lie in the
refactoring of MwsrWriter to introduce a lock-free operation based on that.

[1]: broken_link_c/issue/861518
[2]: broken_link_c/issue/861578
[3]: broken_link_c/issue/1016724
[4]: broken_link_j/Ticket-47483
