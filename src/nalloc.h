#ifndef _NALLOC_H_
#define _NALLOC_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#ifdef NALLOC_64
#define NALLOC_SIZE  uint64_t
#define NALLOC_MAX   (UINT64_MAX-sizeof(uint64_t))
#else // NALLOC_64
#define NALLOC_SIZE  uint32_t
#define NALLOC_MAX   (UINT32_MAX-sizeof(uint32_t))
#endif // NALLOC_64

struct nalloc
{
    NALLOC_SIZE     size;
    unsigned char   data[];
};

typedef void*    nptr;

#define HAVE_ATOMIC

#ifdef HAVE_ATOMIC
#define _NALLOC_MEM_ADD(x)  __sync_add_and_fetch(&nalloc_memory, x)
#define _NALLOC_MEM_SUB(x)  __sync_sub_and_fetch(&nalloc_memory, x)
#else // HAVE_ATOMIC
#define _NALLOC_MEM_ADD(x)   do{ \
    pthread_mutex_lock(&nalloc_lock); \
    nalloc_memory += x; \
    pthread_mutex_unlock(&nalloc_lock); \
    } while(0)

#define _NALLOC_MEM_SUB(x)   do{ \
    pthread_mutex_lock(&nalloc_lock); \
    nalloc_memory -= x; \
    pthread_mutex_unlock(&nalloc_lock); \
    } while(0)
#endif // HAVE_ATOMIC

#define NALLOC_MEM_ADD(_x_)   do{ \
        if(_nalloc_thread_safe) _NALLOC_MEM_ADD(_x_); \
        else nalloc_memory += _x_; \
    }while(0)
#define NALLOC_MEM_SUB(_x_)   do{ \
        if(_nalloc_thread_safe) _NALLOC_MEM_SUB(_x_); \
        else nalloc_memory -= _x_; \
    }while(0)

static size_t nalloc_memory = 0;
static int _nalloc_thread_safe = 0;

#ifndef HAVE_ATOMIC
static pthread_mutex_t nalloc_lock = PTHREAD_MUTEX_INITIALIZER;
#endif // HAVE_ATOMIC

static inline void nalloc_default_oos(size_t size)
{
    fprintf(stderr, "Nalloc, out of memroy, size %zu\n", size);
    fflush(stderr);
    abort();
    return;
}

static inline void nalloc_default_oom(size_t size)
{
    fprintf(stderr, "Nalloc, out of memory size %zu, allocated %zu\n", size, nalloc_memory);
    fflush(stderr);
    abort();
    return;
}

static void (*nalloc_default_oos_handler)(size_t) = nalloc_default_oos;
static void (*nalloc_default_oom_handler)(size_t) = nalloc_default_oom;

static inline nptr nalloc_ptr(nptr ptr)
{
    if(ptr == NULL) return NULL;

    return (void*)ptr-(sizeof(NALLOC_SIZE));
}

static inline size_t nalloc_ptr_size(nptr ptr)
{
    if(ptr == NULL) return 0;

    struct nalloc *na = nalloc_ptr(ptr);
    return (size_t)na->size;
}

static inline void nalloc_free(nptr ptr)
{
    if(ptr == NULL) return;

    size_t size;
    struct nalloc *na = nalloc_ptr(ptr);
    size = (size_t)na->size;
    free((void*) na);
    NALLOC_MEM_SUB(size);
#ifdef DEBUG
    fprintf(stdout, "Nalloc, freed %zu, Allocated %zu\n", size, nalloc_memory);
    fflush(stderr);
#endif // DEBUG
    return;
}

static inline nptr nalloc(size_t size)
{
    if(size == 0) return NULL;
    if(size > NALLOC_MAX)
    {
        nalloc_default_oos_handler(size);
        return NULL;
    }

    struct nalloc *na;
    na = malloc(size+sizeof(NALLOC_SIZE));
    if(na == NULL) nalloc_default_oom_handler(size);

    na->size = (NALLOC_SIZE)size;

    NALLOC_MEM_ADD((size_t)na->size);

#ifdef DEBUG
    fprintf(stdout, "Nalloc, nalloc %zu, Allocated %zu\n", size, nalloc_memory);
    fflush(stderr);
#endif // DEBUG
    return (void*)na->data;
}

static inline nptr ncalloc(size_t size)
{
    if(size == 0) return NULL;
    if(size > NALLOC_MAX)
    {
        nalloc_default_oos_handler(size);
        return NULL;
    }

    struct nalloc *na;
    na = calloc(size+sizeof(NALLOC_SIZE), 1);
    if(na == NULL) nalloc_default_oom_handler(size);
    na->size = (NALLOC_SIZE)size;
    NALLOC_MEM_ADD((size_t)na->size);
#ifdef DEBUG
    fprintf(stdout, "Ncalloc, ncalloc %zu, Allocated %zu\n", size, nalloc_memory);
    fflush(stderr);
#endif // DEBUG
    return (void*)na->data;
}

static inline nptr nrealloc(nptr ptr, size_t size)
{
    if(size > NALLOC_MAX)
    {
        nalloc_default_oos_handler(size);
        return NULL;
    }

    void *n = nalloc(size);

    if(ptr == NULL)
        return n;

    size_t old_size = nalloc_ptr_size(ptr);

    if(old_size > size)
    {
        memcpy(n, ptr, size);
    }else
    {
        memcpy(n, ptr, old_size);
    }

    nalloc_free(ptr);
#ifdef DEBUG
    fprintf(stdout, "Nalloc, nrealloc %zu, Allocated %zu\n", size, nalloc_memory);
    fflush(stderr);
#endif // DEBUG
    return n;
}

static inline size_t nalloc_memroy()
{
    return nalloc_memory;
}

static inline void nalloc_thread_safe(int flag)
{
    _nalloc_thread_safe = flag;
}


static inline nptr nalloc_dup2(void *data, size_t len)
{
    void *ndata;

    if(data == NULL)
        return NULL;

    ndata = nalloc(len);
    memcpy(ndata, data, len);
    return ndata;
}


static inline nptr nalloc_dup(nptr data)
{
    return nalloc_dup2(data, nalloc_ptr_size(data));
}

static inline nptr nalloc_duplen(void *data, size_t len)
{
    void *ndata;

    if(data == NULL)
        return NULL;

    if(len == 0 || len > NALLOC_MAX)
        return NULL;

    ndata = nalloc(len);
    memcpy(ndata, data, len);

    return ndata;
}

#endif // _NALLOC_H_
