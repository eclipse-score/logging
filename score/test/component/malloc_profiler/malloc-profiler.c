/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

/*
 * Trace memory consumption under Linux and QNX.
 *
 * This program can easily be compiled and linked using e.g.:
 *
 *    gcc src/malloc-profiler.c -shared -fPIC -o libmalloc-profiler.so
 *
 * Use
 *
 *    see README.md
 *
 * The tracer is implemented as a shared library, which must be preloaded before
 * starting the binary under test (LD_PRELOAD). It hooks malloc(), calloc(),
 * realloc(), valloc(), pvalloc(), posix_memalign(), aligned_alloc() and free().
 * Traced are:
 * - the called function
 * - the time of the call
 * - the parameters and the return value
 * - the name and the id of the thread that's calling
 *
 * Compile with the preprocessor macro __DEBUG__ defined to enable debug traces
 *
 */

#ifndef QNXNTO
// assuming Linux
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __QNX__
#include <sys/neutrino.h>
#include <sys/syspage.h>

#ifdef KEV_TRACE
#include <process.h>
#include <sys/trace.h>
#endif

typedef void* (*malloc_f)(size_t size);
typedef void* (*calloc_f)(size_t n, size_t size);
typedef void* (*realloc_f)(void* old_blk, size_t size);
typedef void (*free_f)(void* ptr);
typedef pid_t (*fork_f)(void);
typedef void* (*valloc_f)(size_t size);
typedef void* (*pvalloc_f)(size_t size);
typedef void* (*memalign_f)(size_t alignment, size_t size);
typedef void* (*_mmap2_f)(void* __addr,
                          size_t __len,
                          int __prot,
                          int __flags,
                          int __fd,
                          off64_t __off,
                          unsigned __align,
                          size_t __preload,
                          void** __base,
                          size_t* __size);
typedef int (*munmap_f)(void* __addr, size_t __len);

#define PROFILER_MAX_THREADNAMES 256
#define SHM_PATH "/dev/shmem"
#define OS "QNX"

#ifdef KEV_TRACE
#define TRACEF_OFFSET 1000
#endif

#else
#include <sys/prctl.h>
#include <syscall.h>

extern void* __libc_malloc(size_t size);
extern void* __libc_calloc(size_t n, size_t size);
extern void* __libc_realloc(void* old_blk, size_t size);
extern void __libc_free(void* ptr);
extern pid_t __libc_fork(void);
extern void* __libc_valloc(size_t size);
extern void* __libc_pvalloc(size_t size);
extern void* __libc_memalign(size_t alignment, size_t size);

#define SHM_PATH "/dev/shm"
#define OS "Linux"
#endif

typedef int (*posix_memalign_f)(void** memptr, size_t alignment, size_t size);
typedef void* (*aligned_alloc_f)(size_t alignment, size_t size);

#define TASK_COMM_LEN 16
#define PROFILER_CHECK_INTERVALL_NS 1000000000

#define strong_alias(name, aliasname) _strong_alias(name, aliasname)
#define _strong_alias(name, aliasname) extern __typeof(name) aliasname __attribute__((alias(#name)));
#define strong_alias_malloc_nothrow(name, aliasname) _strong_alias_malloc_nothrow(name, aliasname)
#define _strong_alias_malloc_nothrow(name, aliasname) \
    extern __typeof(name) aliasname __attribute__((alias(#name))) __attribute__((malloc)) __attribute__((nothrow));

#define FUNC_MALLOC 1
#define FUNC_FREE 2
#define FUNC_MMAP 3
// definition for mmap call originating from outside malloc-family functions
#define FUNC_MMAP_EXT 4
#define FUNC_MUNMAP 5
// definition for munmap call originating from outside malloc-family functions
#define FUNC_MUNMAP_EXT 6
#define TRACE_ITEM_FLAG_REALLOC 1
#define TRACE_ITEM_FLAG_CALLOC 2
#define TRACE_ITEM_FLAG_VALLOC 3
#define TRACE_ITEM_FLAG_PVALLOC 4
#define TRACE_ITEM_FLAG_POSIX_MEMALIGN 5
#define TRACE_ITEM_FLAG_MEMALIGN 6
#define TRACE_ITEM_FLAG_ALIGNED_ALLOC 7

/************************/
/* profiler     	    	*/
/************************/
/**
 * @brief One and only global variable. Holds all necessary data
 */
struct
{
    unsigned flags;
    int file_fd;                      /**< shared memory file descriptor */
    unsigned long max_tracefile_size; /**< maximum trace file size */
    uint64_t total_length;            /**< the current amount of data the trace file */
    uint64_t prelude_length;          /**< length of the heading lines (parameters, column info) */
    void* v;                          /**< virtual address of the trace file */
    unsigned verbosity;
    struct timespec creation_time; /**< point in time when the file was created */
#ifdef __QNX__
    char threadnames[PROFILER_MAX_THREADNAMES][TASK_COMM_LEN];
    uint8_t backtrace[PROFILER_MAX_THREADNAMES];
#endif
} profiler = {};

#define PROFILER_FLAG_ENABLED 1
#define PROFILER_FLAG_DISABLED 2
#define PROFILER_FLAG_INITIALIZED 4

/************************/
/* profiler_gettid      */
/************************/
static pid_t profiler_gettid(void)
{
#ifdef __QNX__
    return pthread_self();
#else
    return (pid_t)syscall(SYS_gettid);
#endif
}

/************************/
/* profiler_printf		*/
/************************/
static int profiler_printf(const char* format, ...)
{
    va_list va;
    char buf[256];
    int result;
    int unused_var;

    va_start(va, format);
    result = vsnprintf(buf, sizeof(buf), format, va);
    unused_var = write(1, buf, result);
    (void)unused_var;
    va_end(va);

    return result;
}

#define profiler_vprintf(level, ...) \
    if (profiler.verbosity >= level) \
    profiler_printf(__VA_ARGS__)

#ifdef __DEBUG__
#define dbg_print(...) profiler_printf(__VA_ARGS__)
#else
#define dbg_print(...)
#endif

/************************/
/* profiler_shm_open  	*/
/************************/
static int profiler_shm_open(const char* name, int oflag, mode_t mode)
{
    int res;
    char shmname[256];

    snprintf(shmname, sizeof(shmname) - 1, SHM_PATH "%s", name);

    res = open(shmname, oflag | O_NOFOLLOW | O_CLOEXEC, mode);

    return res;
}

/************************/
/* profiler_threadname	*/
/************************/
static int profiler_threadname(char* procname, int blen)
{
    int res = 0;
#ifdef __QNX__
    res = pthread_getname_np(pthread_self(), procname, blen);

    if (res != EOK)
    {
        profiler_vprintf(1, "%s: pthread_getname_np() failed, reason is %s\n", __func__, strerror(res));
    }
#else
    if (prctl(PR_GET_NAME, procname) == -1)
    {
        res = errno;
        profiler_vprintf(1, "%s: prctl() failed, reason is %s\n", __func__, strerror(errno));
    }
#endif

    return res;
}

/**********************************/
/* profiler_set_modification_time */
/**********************************/
static void profiler_set_modification_time(struct timespec* ptimespec)
{
    struct stat st;

    if (fstat(profiler.file_fd, &st) == 0)
        memcpy(ptimespec, &st.st_mtim, sizeof(profiler.creation_time));
    else
        profiler_vprintf(1, "%s: fstat() failed, reason is %s\n", __func__, strerror(errno));
}

/**********************************/
/* profiler_get_current_time */
/**********************************/
static inline uint64_t profiler_get_current_time(void)
{
#ifdef __QNX__
    uint64_t tstamp = ClockCycles();
#else
    struct timespec t;

    clock_gettime(CLOCK_MONOTONIC, &t);
    uint64_t tstamp = 1000000000 * t.tv_sec + t.tv_nsec;
#endif
    return tstamp;
}

/************************/
/* init					*/
/************************/
#ifdef __QNX__
/* QNX provides the command line arguments as extern symbols */
extern char** _argv;
extern int _argc;

__attribute__((constructor)) static void init()
#else
__attribute__((constructor)) static void init(int _argc, char** _argv, char** envp)
#endif
{
#ifdef CSV_TRACE
    int res = 0;

    char* verb = getenv("PROFILER_VERBOSE");

    if (verb)
    {
        profiler.verbosity = strtoul(verb, NULL, 0);
    }
    else
        profiler.verbosity = 1;

    /* maximum trace file size */
    char* max_size = getenv("PROFILER_MAX_FILE_SIZE");

    if (max_size)
    {
        profiler.max_tracefile_size = strtoul(max_size, NULL, 0);
    }

    /* default value */
    if (profiler.max_tracefile_size == 0)
        profiler.max_tracefile_size = 4 * 1024 * 1024;

    /* open the trace file */
    char trace_file_name[128];
    char procname[TASK_COMM_LEN + 1] = {};

    profiler_threadname(procname, sizeof(procname));

    if (procname[0])
    {
        procname[TASK_COMM_LEN] = 0;
        snprintf(trace_file_name, sizeof(trace_file_name) - 1, "/profiler.%s.%d.csv", procname, getpid());
    }
    else
    {
        /* try command line */
        if (_argc > 0 && _argv && strlen(_argv[0]) > 0)
        {
            /* determine base name */
            char* p = _argv[0] + strlen(_argv[0]);

            while (p > _argv[0])
            {
                if (*p == '/')
                {
                    p++;
                    break;
                }

                p--;
            }

            snprintf(trace_file_name, sizeof(trace_file_name) - 1, "/profiler.%s.%d.csv", p, getpid());
            snprintf(procname, sizeof(procname) - 1, "%s", p);
        }
        else
            snprintf(trace_file_name, sizeof(trace_file_name) - 1, "/profiler.%d.csv", getpid());
    }

    profiler.file_fd =
        profiler_shm_open(trace_file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IROTH);

    if (profiler.file_fd != -1)
    {
        if (ftruncate(profiler.file_fd, profiler.max_tracefile_size) == -1)
        {
            res = errno;
            profiler_vprintf(1,
                             "%s: ftruncate(%u) failed, reason is %s\n",
                             "malloc-profiler",
                             profiler.max_tracefile_size,
                             strerror(errno));
            close(profiler.file_fd);
        }
        else
        {
            profiler.v =
                mmap64(NULL, profiler.max_tracefile_size, PROT_READ | PROT_WRITE, MAP_SHARED, profiler.file_fd, 0);

            if (profiler.v != MAP_FAILED)
            {
                memset(profiler.v, 0, profiler.max_tracefile_size);  // necessary?

#ifdef __QNX__
                uint64_t cps = SYSPAGE_ENTRY(qtime)->cycles_per_sec;
                uint32_t arena_size = 0, arena_cache_size = 0, arena_cache_blocks = 0;

                /*
                    determine arena size and cache size
                    if MALLOC_ARENA_CACHE_MAXSZ and MALLOC_ARENA_CACHE_MAXBLK are set to contradictory values,
                    the strictest value applies. Assuming the arena size is 32kB, MALLOC_ARENA_CACHE_MAXSZ is
                    64kB and MALLOC_ARENA_CACHE_MAXBLK is 3, then 2 arenas of 32kB are kept in the cache.
                    If MALLOC_ARENA_CACHE_MAXSZ is changed to 128kB, other parameters stay the same, then 3
                    arenas are kept. Again, this applies only for allocations < arena size!
                */

                char* envp = getenv("MALLOC_ARENA_SIZE");

                if (envp)
                    arena_size = strtoul(envp, NULL, 0);

                if (arena_size == 0)
                    arena_size = 32 * 1024;  // default value

                envp = getenv("MALLOC_ARENA_CACHE_MAXBLK");

                if (envp)
                    arena_cache_blocks = strtoul(envp, NULL, 0);

                if (arena_cache_blocks == 0)
                    arena_cache_blocks = 12;  // default value

                envp = getenv("MALLOC_ARENA_CACHE_MAXSZ");

                if (envp)
                    arena_cache_size = strtoul(envp, NULL, 0);

                if (arena_cache_size != 0)
                {
                    if (arena_cache_size > arena_cache_blocks * arena_size)
                        arena_cache_size = arena_cache_blocks * arena_size;
                }
                else
                    arena_cache_size = arena_cache_blocks * arena_size;

                profiler.total_length =
                    sprintf(profiler.v,
                            "Parameters,OS=%s,application=%s,cycles_per_second=%lu,arena_size=%u,arena_cache_size=%u\n",
                            OS,
                            procname,
                            cps,
                            arena_size,
                            arena_cache_size);
#else
                profiler.total_length = sprintf(
                    profiler.v, "Parameters,OS=%s,application=%s,cycles_per_second=%u\n", OS, procname, 1000000000);
#endif
                profiler.total_length += sprintf(profiler.v + profiler.total_length,
                                                 "timestamp,func,flag,in_ptr,ptr,size,duration,pid,processname\n");
                profiler.prelude_length = profiler.total_length;
            }
            else
            {
                res = errno;
                profiler_vprintf(1, "%s: mmap() failed, reason is %s\n", "malloc-profiler", strerror(errno));
            }

            /* memorize the creation time for later profiler enabling */
            profiler_set_modification_time(&profiler.creation_time);
        }
    }
    else
    {
        res = errno;
        profiler_vprintf(
            1, "%s: profiler_shm_open(%s) failed, reason is %s\n", "malloc-profiler", trace_file_name, strerror(errno));
    }

    if (res == 0)
    {
        profiler_vprintf(
            2, "\e[1;32mmalloc-profiler initialization completed, trace file = " SHM_PATH "%s\n\e[0m", trace_file_name);

        uint32_t flags = PROFILER_FLAG_INITIALIZED;

        char* ena = getenv("PROFILER_ENABLED");

        if (ena && strtoul(ena, NULL, 0) > 0)
        {
            flags |= PROFILER_FLAG_ENABLED;

            profiler_vprintf(2, "Profiler starts recording\n");
        }

        __atomic_or_fetch(&profiler.flags, flags, __ATOMIC_RELEASE);
    }
    else
        profiler_vprintf(1, "\e[1;31mmalloc-profiler initialization failed\n\e[0m");
#endif
}

/***************************/
/* disable_profiler        */
/***************************/
void disable_profiler()
{
    /* disable logging */
    __atomic_and_fetch(&profiler.flags, ~PROFILER_FLAG_ENABLED, __ATOMIC_RELAXED);
    /* do not react on any user modifications during recording */
    profiler_set_modification_time(&profiler.creation_time);
    /* flag that logging has been enabled before */
    __atomic_or_fetch(&profiler.flags, PROFILER_FLAG_DISABLED, __ATOMIC_RELEASE);
}

/************************/
/* fini					*/
/************************/
__attribute__((destructor)) static void fini(void)
{
#ifdef CSV_TRACE

    /*
        Application exit. to resize the trace file to avoid
        MBs of trailing zeros, following approach was done:
        int unused_var;

        disable_profiler();
        __atomic_store_n(&profiler.max_tracefile_size, profiler.total_length, __ATOMIC_RELEASE);
        unused_var = ftruncate64(profiler.file_fd, profiler.total_length);
        (void)unused_var;

        It turned out that the destructor is called while there are still calls to trace,
        especially free()-calls. In addition free()-calls after disabling the profiler led
        to restarting the profiler including erasing everything recorded so far.


        To avoid complicating the code further I concluded to accept the MBs of trainling
        zeros as the minor issue.
    */
    close(profiler.file_fd);
#endif
}

/***************************/
/* strfunc                 */
/***************************/
/**
 * @brief Turn numeric func-constant into string
 */
char* strfunc(unsigned func)
{
    /*
        The strings "map_e" and "unm_e" are used to identify calls to mmap and munmap functions
        originating from outside the malloc-family functions.
    */
    static char strfunc_table[][16] = {"", "ma", "fr", "map", "map_e", "unm", "unm_e", "out_of_bounds"};

    if (func >= 1 && func <= 6)
        return strfunc_table[func];
    else
        return strfunc_table[7];
}

/***************************/
/* strflag                 */
/***************************/
/**
 * @brief Turn numeric flag-constant into string
 */
char* strflag(unsigned flag)
{
    static char strflag_table[][16] = {
        "", "realloc", "calloc", "valloc", "pvalloc", "posix_memalign", "memalign", "aligned_alloc", "invalid"};

#define STRFLAG_TABLE_SIZE (sizeof(strflag_table) / 16)

    if (flag >= 0 && flag <= STRFLAG_TABLE_SIZE - 2)
        return strflag_table[flag];
    else
        return strflag_table[STRFLAG_TABLE_SIZE - 1];
}

#ifndef __QNXNTO__
/***************************/
/* thread-local variable   */
/***************************/
__thread char procname[TASK_COMM_LEN] = {};

#endif

/***************************/
/* profiler_trace          */
/***************************/
/**
 * @brief Store the parameters of a function call in memory

 Parameters:

 func : integer value used to map the basic functions like "ma", "fr", "map", "unm", "out_of_bounds".
 flags : integer value used to map the specific memory allocation functions like calloc, realloc.
 in_ptr : is used to hold the old pointer when realloc is called and new memory with new pointer is allocated. In case
 of other functions this is NULL.
 ptr : is used to hold the pointer value on calling memory allocation functions like malloc, calloc.
 size : is used to hold the size of the memory allocated with each memory allocation functions used.
 tstamp_duration : is used to hold the call duration in nanoseconds of the memory allocation functions used.

*/

static void profiler_trace(uint16_t func,
                           uint16_t flags,
                           void* in_ptr,
                           void* ptr,
                           size_t size,
                           uint64_t tstamp_duration)
{

#if defined KEV_TRACE && __QNXNTO__

    trace_logf(_NTO_TRACE_USERFIRST + TRACEF_OFFSET,
               "MEM pid:%u tid:%u fn:%s flg:%s in:%p out:%p size:%zu dur:%zu",
               getpid(),
               gettid(),
               strfunc(func),
               strflag(flags),
               in_ptr,
               ptr,
               size,
               tstamp_duration);
#endif

#ifdef CSV_TRACE
    char line[256];
    int len;

    /* actually it's not a loop */
    while ((profiler.flags & PROFILER_FLAG_ENABLED) == 0)
    {
        static uint64_t last_check = 0;
        struct timespec t;

        /* not yet initialized */
        if ((profiler.flags & PROFILER_FLAG_INITIALIZED) != 0)
        {

            /* check if the shared mem file was touched,
                if yes enable the profiler */
            clock_gettime(CLOCK_MONOTONIC, &t);
            uint64_t now = 1000000000 * t.tv_sec + t.tv_nsec;

            if (now - last_check > PROFILER_CHECK_INTERVALL_NS)
            {
                last_check = now;

                struct timespec ctime;
                profiler_set_modification_time(&ctime);

                if (memcmp(&profiler.creation_time, &ctime, sizeof(profiler.creation_time)) != 0)
                {
                    profiler_vprintf(2, "Profiler starts recording\n");

                    /*
                        if the profiler was disabled before, reset the ringbuffer
                        in theory that's dangerous since there could be a thread
                        still in the critical section (means it has reserved space
                        in the rb, but not written the data). In this case the zeored mem
                        would be overwritten again.
                        In practise there will be a lot of time between buffer disabling
                        and re-eanbling, since the re-enambling will be done manually
                        If the user touches the tracefile while tracing is enabled, we
                        might run into this situation.

                        As possible fix is the use of atomic counters: when a thread starts
                        tracing, it increases a counter and if it finishes tracing, it decrements it.
                        When tracing gets reenabled, the counter is checked. If it is zero, the actual
                        re-enabling can be done without danger.
                    */
                    if ((profiler.flags & PROFILER_FLAG_DISABLED))
                    {
                        memset(profiler.v + profiler.prelude_length,
                               0,
                               profiler.max_tracefile_size - profiler.prelude_length);
                        profiler.total_length = profiler.prelude_length;
                        __atomic_and_fetch(&profiler.flags, ~PROFILER_FLAG_DISABLED, __ATOMIC_RELAXED);
                    }

                    memcpy(&profiler.creation_time, &ctime, sizeof(profiler.creation_time));
                    __atomic_or_fetch(&profiler.flags, PROFILER_FLAG_ENABLED, __ATOMIC_RELEASE);
                    break;
                }
            }
        }

        return;
    }

#ifdef __QNX__
    char* procname = "max threads exceeded";
    pid_t tid = profiler_gettid();

    if (tid < PROFILER_MAX_THREADNAMES)
    {
        procname = profiler.threadnames[tid];

        if (profiler.threadnames[tid][0] == 0)
        {
            profiler_threadname(procname, TASK_COMM_LEN);

            /* fallback tid as thread name */
            if (procname[0] == 0)
            {
                snprintf(procname, TASK_COMM_LEN - 1, "%d", tid);
            }
        }
    }
#else
    if (procname[0] == 0)
    {
        profiler_threadname(procname, sizeof(procname));
    }
#endif

    dbg_print("thread %d trace func %s flag %s\n", profiler_gettid(), strfunc(func), strflag(flags));

    /*
        determine the timestamp
        in the resulting trace the timestamps
        are not necessarily sorted in ascending order
        due to thread concurency
    */
    uint64_t tstamp = profiler_get_current_time();

    /* compute the length of the trace */
    len = snprintf(line,
                   sizeof(line) - 1,
                   "%lu,%s,%s,%p,%p,%lu,%lu,%d,%s\n",
                   tstamp,
                   strfunc(func),
                   strflag(flags),
                   in_ptr,
                   ptr,
                   size,
                   tstamp_duration,
                   profiler_gettid(),
                   procname);

    /* atomically reserve bytes in the trace file */
    uint64_t old_len = __atomic_fetch_add(&profiler.total_length, (uint64_t)len, __ATOMIC_RELAXED);

    /* check the size */
    if (profiler.total_length <= profiler.max_tracefile_size)
    {
        /* copy the trace data into the reserved area */
        memcpy(profiler.v + old_len, line, len);
    }
    else
    {
        /* buffer full, disable tracing */
        disable_profiler();
        profiler_vprintf(2, "Recording completed\n");
    }
#endif
}

/************************/
/* set_backtrace   	    */
/************************/
/*
    The function sets a per-thread value to the value
    of the passed parameter.
    The malloc-family of functions sets the value to 1
    before calling the original function and to 0 afterwards.
    mmap and munmap check the value. If it is != 0
    they will trace the call, otherwise not. This way only
    mmaps and munmap originating from an alloc-function
    are traced.
*/
static inline void set_backtrace(unsigned val)
{
#ifdef CSV_TRACE
#ifdef __QNX__
    pid_t tid = profiler_gettid();

    if (tid < PROFILER_MAX_THREADNAMES)
    {
        profiler.backtrace[tid] = val;
    }
#else
#endif
#endif
}

/************************/
/* malloc       	    */
/************************/
void* malloc(size_t size)
{
#if defined(__QNXNTO__)
    static malloc_f pf = NULL;

    if (pf == NULL)
    {
        pf = dlsym(RTLD_NEXT, __func__);
    }

    if (pf)
    {
        set_backtrace(1);
        const uint64_t tstamp_before = profiler_get_current_time();
        void* res = pf(size);
        const uint64_t tstamp_after = profiler_get_current_time();
        set_backtrace(0);
        profiler_trace(FUNC_MALLOC, 0, 0, res, size, tstamp_after - tstamp_before);
        return res;
    }
    else
        return NULL;
#else
    const uint64_t tstamp_before = profiler_get_current_time();
    void* res = __libc_malloc(size);
    const uint64_t tstamp_after = profiler_get_current_time();
    profiler_trace(FUNC_MALLOC, 0, 0, res, size, tstamp_after - tstamp_before);
    return res;
#endif
}

/************************/
/* calloc       		    */
/************************/
void* calloc(size_t n, size_t size)
{
#ifdef __QNX__
    static calloc_f pf = NULL;

    if (pf == NULL)
    {
        pf = dlsym(RTLD_NEXT, __func__);
    }

    if (pf)
    {
        set_backtrace(1);
        const uint64_t tstamp_before = profiler_get_current_time();
        void* res = pf(n, size);
        const uint64_t tstamp_after = profiler_get_current_time();
        set_backtrace(0);
        profiler_trace(FUNC_MALLOC, TRACE_ITEM_FLAG_CALLOC, 0, res, n * size, tstamp_after - tstamp_before);
        return res;
    }
    else
        return NULL;
#else
    const uint64_t tstamp_before = profiler_get_current_time();
    void* res = __libc_calloc(n, size);
    const uint64_t tstamp_after = profiler_get_current_time();
    profiler_trace(FUNC_MALLOC, TRACE_ITEM_FLAG_CALLOC, 0, res, n * size, tstamp_after - tstamp_before);
    return res;
#endif
}

/************************/
/* realloc          		*/
/************************/
void* realloc(void* old_blk, size_t size)
{
#ifdef __QNX__
    static realloc_f pf = NULL;

    if (pf == NULL)
    {
        pf = dlsym(RTLD_NEXT, __func__);
    }

    if (pf)
    {
        set_backtrace(1);
        const uint64_t tstamp_before = profiler_get_current_time();
        void* res = pf(old_blk, size);
        const uint64_t tstamp_after = profiler_get_current_time();
        set_backtrace(0);
        profiler_trace(FUNC_MALLOC, TRACE_ITEM_FLAG_REALLOC, old_blk, res, size, tstamp_after - tstamp_before);
        return res;
    }
    else
        return NULL;
#else
    const uint64_t tstamp_before = profiler_get_current_time();
    void* res = __libc_realloc(old_blk, size);
    const uint64_t tstamp_after = profiler_get_current_time();
    profiler_trace(FUNC_MALLOC, TRACE_ITEM_FLAG_REALLOC, old_blk, res, size, tstamp_after - tstamp_before);
    return res;
#endif
}

/************************/
/* free              		*/
/************************/
void free(void* ptr)
{
#ifdef __QNX__
    static free_f pf = NULL;

    if (pf == NULL)
    {
        pf = dlsym(RTLD_NEXT, __func__);
    }

    if (pf)
    {
        set_backtrace(1);
        const uint64_t tstamp_before = profiler_get_current_time();
        pf(ptr);
        const uint64_t tstamp_after = profiler_get_current_time();
        set_backtrace(0);
        profiler_trace(FUNC_FREE, 0, 0, ptr, 0, tstamp_after - tstamp_before);
    }
#else
    const uint64_t tstamp_before = profiler_get_current_time();
    __libc_free(ptr);
    const uint64_t tstamp_after = profiler_get_current_time();
    profiler_trace(FUNC_FREE, 0, 0, ptr, 0, tstamp_after - tstamp_before);
#endif
}

/************************/
/* valloc       		    */
/************************/
void* valloc(size_t size)
{
#ifdef __QNX__
    static valloc_f pf = NULL;

    if (pf == NULL)
    {
        pf = dlsym(RTLD_NEXT, __func__);
    }

    if (pf)
    {
        set_backtrace(1);
        const uint64_t tstamp_before = profiler_get_current_time();
        void* res = pf(size);
        const uint64_t tstamp_after = profiler_get_current_time();
        set_backtrace(0);
        profiler_trace(FUNC_MALLOC, TRACE_ITEM_FLAG_VALLOC, 0, res, size, tstamp_after - tstamp_before);
        return res;
    }
    else
        return NULL;
#else
    const uint64_t tstamp_before = profiler_get_current_time();
    void* res = __libc_valloc(size);
    const uint64_t tstamp_after = profiler_get_current_time();
    profiler_trace(FUNC_MALLOC, TRACE_ITEM_FLAG_VALLOC, 0, res, size, tstamp_after - tstamp_before);
    return res;
#endif
}

/************************/
/* pvalloc       		    */
/************************/
void* pvalloc(size_t size)
{
#ifdef __QNX__
    static pvalloc_f pf = NULL;

    if (pf == NULL)
    {
        pf = dlsym(RTLD_NEXT, __func__);
    }

    if (pf)
    {
        set_backtrace(1);
        const uint64_t tstamp_before = profiler_get_current_time();
        void* res = pf(size);
        const uint64_t tstamp_after = profiler_get_current_time();
        set_backtrace(0);
        profiler_trace(FUNC_MALLOC, TRACE_ITEM_FLAG_PVALLOC, 0, res, size, tstamp_after - tstamp_before);
        return res;
    }
    else
        return NULL;
#else
    const uint64_t tstamp_before = profiler_get_current_time();
    void* res = __libc_pvalloc(size);
    const uint64_t tstamp_after = profiler_get_current_time();
    profiler_trace(FUNC_MALLOC, TRACE_ITEM_FLAG_PVALLOC, 0, res, size, tstamp_after - tstamp_before);
    return res;
#endif
}

/************************/
/* posix_memalign		    */
/************************/
int posix_memalign(void** memptr, size_t alignment, size_t size)
{
    static posix_memalign_f pf = NULL;

    if (pf == NULL)
    {
        pf = dlsym(RTLD_NEXT, __func__);
    }

    if (pf)
    {
        set_backtrace(1);
        const uint64_t tstamp_before = profiler_get_current_time();
        int res = pf(memptr, alignment, size);
        const uint64_t tstamp_after = profiler_get_current_time();
        set_backtrace(0);
        profiler_trace(FUNC_MALLOC, TRACE_ITEM_FLAG_POSIX_MEMALIGN, 0, *memptr, size, tstamp_after - tstamp_before);
        return res;
    }
    else
        return EINVAL;
}

strong_alias(posix_memalign, profiler_posix_memalign)

    /************************/
    /* memalign     		    */
    /************************/
    void* memalign(size_t alignment, size_t size)
{
#ifdef __QNX__
    static memalign_f pf = NULL;

    if (pf == NULL)
    {
        pf = dlsym(RTLD_NEXT, __func__);
    }

    if (pf)
    {
        set_backtrace(1);
        const uint64_t tstamp_before = profiler_get_current_time();
        void* res = pf(alignment, size);
        const uint64_t tstamp_after = profiler_get_current_time();
        set_backtrace(0);
        profiler_trace(FUNC_MALLOC, TRACE_ITEM_FLAG_MEMALIGN, 0, res, size, tstamp_after - tstamp_before);
        return res;
    }
    else
        return NULL;
#else
    const uint64_t tstamp_before = profiler_get_current_time();
    void* res = __libc_memalign(alignment, size);
    const uint64_t tstamp_after = profiler_get_current_time();
    profiler_trace(FUNC_MALLOC, TRACE_ITEM_FLAG_MEMALIGN, 0, res, size, tstamp_after - tstamp_before);
    return res;
#endif
}

strong_alias(memalign, profiler_memalign)

    /************************/
    /* aligned_alloc 		    */
    /************************/
    void* aligned_alloc(size_t alignment, size_t size)
{
    static aligned_alloc_f pf = NULL;

    if (pf == NULL)
    {
        pf = dlsym(RTLD_NEXT, __func__);
    }

    if (pf)
    {
        set_backtrace(1);
        const uint64_t tstamp_before = profiler_get_current_time();
        void* res = pf(alignment, size);
        const uint64_t tstamp_after = profiler_get_current_time();
        set_backtrace(0);
        profiler_trace(FUNC_MALLOC, TRACE_ITEM_FLAG_ALIGNED_ALLOC, 0, res, size, tstamp_after - tstamp_before);
        return res;
    }
    else
        return NULL;
}

strong_alias_malloc_nothrow(aligned_alloc, profiler_aligned_alloc)

    /************************/
    /* fork         		*/
    /************************/
    pid_t fork(void)
{
    pid_t res;

#ifdef __QNX__
    static fork_f pf = NULL;

    if (pf == NULL)
    {
        pf = dlsym(RTLD_NEXT, __func__);
    }

    if (pf)
    {
        res = pf();
    }
    else
        return -1;
#else
    res = __libc_fork();
#endif

    if (res == 0)
    {
        /* disable tracing for the child process */
        disable_profiler();
        munmap(profiler.v, profiler.max_tracefile_size);
        close(profiler.file_fd);
    }

    return res;
}

#ifdef __QNX__

/************************/
/* _mmap2 				*/
/************************/
void* _mmap2(void* __addr,
             size_t __len,
             int __prot,
             int __flags,
             int __fd,
             off64_t __off,
             unsigned __align,
             size_t __preload,
             void** __base,
             size_t* __size)
{
    static _mmap2_f pf = NULL;

    if (pf == NULL)
    {
        pf = dlsym(RTLD_NEXT, __func__);
    }

    if (pf)
    {
        const uint64_t tstamp_before = profiler_get_current_time();
        void* res = pf(__addr, __len, __prot, __flags, __fd, __off, __align, __preload, __base, __size);
        const uint64_t tstamp_after = profiler_get_current_time();

        if (profiler.backtrace[profiler_gettid()])
            profiler_trace(FUNC_MMAP, 0, 0, res, __len, tstamp_after - tstamp_before);
        else
            /*
                If the call to mmap originated from outside the malloc-family functions,
                trace it with the specific identifier defined for this case.
            */
            profiler_trace(FUNC_MMAP_EXT, 0, 0, res, __len, tstamp_after - tstamp_before);
        return res;
    }
    else
        return NULL;
}

typedef int (*munmap_f)(void* __addr, size_t __len);

/************************/
/* munmap 				*/
/************************/
int munmap(void* __addr, size_t __len)
{
    static munmap_f pf = NULL;

    if (pf == NULL)
    {
        pf = dlsym(RTLD_NEXT, __func__);
    }

    if (pf)
    {
        const uint64_t tstamp_before = profiler_get_current_time();
        int res = pf(__addr, __len);
        const uint64_t tstamp_after = profiler_get_current_time();

        if (profiler.backtrace[profiler_gettid()])
            profiler_trace(FUNC_MUNMAP, 0, 0, __addr, __len, tstamp_after - tstamp_before);
        else
            /*
                If the call to munmap originated from outside the malloc-family functions,
                trace it with the specific identifier defined for this case.
            */
            profiler_trace(FUNC_MUNMAP_EXT, 0, 0, __addr, __len, tstamp_after - tstamp_before);
        return res;
    }

    return -1;
}

#endif
