# Wait-free Multiple-Producer Single Consumer Queue

- [Wait-free Multiple-Producer Single Consumer Queue](#wait-free-multiple-producer-single-consumer-queue)
  - [Introduction](#introduction)
  - [Usage](#usage)
  - [Static Design](#static-design)
  - [Detailed Design: Wait-free Linear Writer](#detailed-design-wait-free-linear-writer)
  - [Detailed Design: Wait-Free Alternating Buffers](#detailed-design-wait-free-alternating-buffers)

## Introduction

This package provides a **wait-free** multiple-producer single consumer queue
implementation that is wait-free for multiple concurrent writer threads. We
support the following requirements tailored for the use case in `mw::log`:

1. Wait-free for multiple producers: Writing for multiple writer threads shall
   be lock-free and wait-free. A write operation shall have a deterministic
   upper-bounded for runtime. The implementation shall not have cross-thread
   mutual exclusion paths.
2. Single-consumer: The data structure supports only a single consumer thread.
3. Blocking consumer: Reading operation shall not be wait-free, and shall not
   block active writers.
4. Data flow: Data shall be transported preserving first-in-first-out (FIFO)
   order of a sequence of contiguous packets.
5. No overwriting: If the buffer is full, new data shall be dropped. Overwriting
   of data shall be not supported.

As algorithms with atomic data structures are easy to get wrong and hard to
verify, our implementation shall be based on a very minimalistic design.
Performance is only a secondary goal after correctness and safety. Thus we set
the following **non-goals** for the current implementation:

1. Sequentially consistent atomic memory order: The implementation shall
   initially use sequential consistency for all atomic operations for
   simplicity. If needed, the memory ordering of atomic operation can be
   relieved for performance improvement as a future step.
2. Cache line optimization and false sharing: The performance of the
   implementation could be improved further by taking into account cache line
   optimization and avoiding false sharing. We believe that the performance will
   still be improved compared to a mutex-based approach and omit that aspect for
   optional future work.

## Usage

Consider the following example for the writer / producer use case:

```C++
// Initialization:
void Initialize(){
    // Create two equal-sized linear buffers for writing.
    std::vector<score::mw::log::detail::Byte> buffer1(kBufferSize);
    std::vector<score::mw::log::detail::Byte> buffer2(kBufferSize);
    score::mw::log::detail::AlternatingControlBlock control_block{};
    control_block.control_block_1.data =
        score::cpp::span<score::mw::log::detail::Byte>(buffer1.data(), static_cast<std::int64_t>(buffer1.size()));
    control_block.control_block_2.data =
        score::cpp::span<score::mw::log::detail::Byte>(buffer2.data(), static_cast<std::int64_t>(buffer2.size()));

    // Initialize the writer given a control block:
    score::mw::log::detail::WaitFreeAlternatingWriter writer{control_block};
    // ...
}

void ProducerThread(score::mw::log::detail::WaitFreeAlternatingWriter& writer){
    // Acquire shall never block, but may return empty result if no capacity.
    acquire_result = writer.Acquire(kAcquireLengthBytes);

    if (acquired_data.size() != kAcquireLength)
    {
        // Error handling: no space in buffer left
        // ...
        return;
    }

    // Use data
    ProduceData(acquire_result.value());

    // Release data for reading. The writer shall always release acquired data.
    writer.Release(acquire_result.value());
}
```

The writer is initialized with a control block structure that references two
equal-sized linear buffers. The producer can rely on the non-blocking and
wait-free behavior of the `Acquire()` method. The producer shall adhere to the
contract to always `Release()` data that is ready for the consumer. In addition,
it has to consider error handling if `Acquire()` cannot allocate more space.

For the consumer, consider the following example code:

```C++
void Consumer(score::cpp::stop_token stop_token) {
    score::mw::log::detail::AlternatingReader reader{control_block};
    while (stop_token.exit_requested() == false)
    {
        // Toggle between the two alternating linear buffers for concurrent reading and writing.
        reader.Switch();

        auto read_result = reader.Read();
        while (read_result.has_value())
        {
            const auto packet = read_result.value();
            ConsumePacket(packet);

            read_result = reader.Read();
        }

        // At this point all data from the current linear buffer has been consumed,
        // and we can toggle to the next buffer for reading.
    }
}
```

The key method for the consumer-side is the `Switch()` method that toggles the
buffers used for reading and writing. The operation of the consumer is blocking,
i.e., it waits until (all) pending writes to the buffer have completed. After
`Switch()` returns, the data from one buffer can be safely read without need for
further synchronization. In parallel, the writer can concurrently write to the
other buffer.

## Static Design

The design is divided into data structures for linear and alternating mode.
The alternating mode is composed by reusing the linear entities:

![Class diagram](https://www.plantuml.com/plantuml/proxy?src=https://raw.githubusercontent.com/swh/ddad_score/mw/log/detail/wait_free_producer_queue/design/class_diagram.uxf?ref=e42bb784c8567256b2c8837cd7a771e6193fc9cb)

For each mode, we extract the data members into POD structs that could be
allocated on shared memory. The data structs should not be accessed directly but
only through the corresponding writer- and reader-classes.

The responsibility of the linear classes is to provide a simple foundation for
lock-free and wait-free writing. The alternating part enables a continuous
operation and decoupling the reader from the writers. This decomposition keeps
both aspects simple and safe to implement.

## Detailed Design: Wait-free Linear Writer

We first explain the linear writer that is the basic building block concurrent
wait-free writing. The buffer created by the writer consists of series of
chunks. Each chunk consists of a length, followed by the payload. The figure
below shows two producer threads writing concurrently and without mutex on the
linear buffer. The synchronization is based on the
[atomic_fetch_add](https://en.cppreference.com/w/c/atomic/atomic_fetch_add)
operation. The linear control block consists of two running atomic indices
`acquire_index` and `written_index`. A producer calls `Acquire()` which
atomically increases the `acquire_index`. The atomic properties guarantee that
the value returned by the atomic operation points to an offset in the linear
array that is exclusively reserved for the caller. For each acquired chunk, the
writer first writes the length of the payload. The length is followed by the
payload as written by the user. When the producer is done they call `Release()`
to finish the data for reading. This operation atomically adds the number of
written bytes to the `written_index`. Thus as soon as all pending writers are
finished we arrive at the equality of `written_index` and `acquire_index`.

![Linear writer sequence](https://www.plantuml.com/plantuml/proxy?src=https://raw.githubusercontent.com/swh/ddad_score/mw/log/detail/wait_free_producer_queue/design/wait_free_linear_buffer.uxf?ref=109d7fea89c8f7f627b62f9e78299f067241a6ec)

The implementation shall prevent the overflow of the running indices. This
achieved by limiting the maximum allowed number of concurrent writers to some
theoretical limit (e.g. 64 threads). Once the limit is exceeded `Acquire()` will
return an empty value. We also limit the maximum size that could be allocated in
one `Acquire()` call. When the acquire exceeds a threshold of
`MaxNumberOfConcurrentWriters * MaxAcquireLength` the writer shall refuse to
write additional data in order to prevent a potential overflow. In addition, the
implementation always has to check if the `acquired_index` fits in the given
linear buffer with the total length requested by the caller.

In addition, the implementation also needs to consider *failed acquisitions*. A
failed acquisition happens when a producer tries to acquire data, but does not
get the full amount of space as we reach the end of the buffer capacity. In such
a case, the writer shall only write the length into the buffer if there is
enough space. Then the reader identifies that a failed acquisition happened at
the end of the buffer by comparing the expected length with the actual size of
the buffer. If an out-of-bounds access would occur, the reader shall assume a
failed acquisition and drop the data.

## Detailed Design: Wait-Free Alternating Buffers

With the wait-free linear writer we have established the basis for concurrent
lock-free writing. However, we also need to enable continuous operation as the
reading of the data should also happen in parallel and without blocking the
producers. Enter the Wait-Free Alternating Buffers Writer concept: The
`AlternatingControlBlock` data structure is built on top of two
`LinearControlBlock`s. There is only a single atomic boolean variable in
addition that controls which linear buffer is currently for reading and which
one is for writing.

The figure below shows the concurrent operation of a producer and consumer thread:

![Alternating writer sequence](https://www.plantuml.com/plantuml/proxy?src=https://raw.githubusercontent.com/swh/ddad_score/mw/log/detail/wait_free_producer_queue/design/wait_free_alternating_buffers.uxf?ref=109d7fea89c8f7f627b62f9e78299f067241a6ec)

In the sequence diagram, the producer thread starts to acquire data on the
linear buffer 1. After that the consumer threads calls `Switch()` to toggle the
buffer that should be written to. Any future writer will use buffer 2 after the
atomic flag `active_for_writing` is updated. Still, buffer 1 is not yet ready
for reading as we first have to wait for pending writers to finish. The
producers increase an atomic counter at the beginning of `Acquire()`. In
`Release()` the counter is atomically decremented. Thus the consumer is able to
tell when all pending writers have finished writing to the current buffer. After
this point, the reader returns a span pointing to buffer 1 that can be safely
accessed then by the consumer. In parallel, the producer is concurrently writing
new data to buffer 2 that will be read by the consumer after the next call to
`Switch()`.
