#ifndef _MSTR_H_
#define _MSTR_H_

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "nalloc.h"

#define MSTR_POINTER    0x00
#define MSTR_ALLOCATED  0x01

struct mstr
{
    size_t  len;
    nptr    data;
};

static inline int mstr_cmp(const struct mstr *l, const struct mstr *r)
{
    if(l->len != r->len)
        return (l->len > r->len) ? 1 : -1;

    return strncmp(l->data, r->data, l->len);
}

static inline struct mstr* _mstr_init(struct mstr *mstr, char *data, size_t len)
{
    mstr->len = len;
    mstr->data = (nptr) data;
    return mstr;
}

static inline struct mstr* mstr_init(struct mstr *mstr, char *data, size_t len)
{
    return _mstr_init(mstr, data, len);
}

static inline struct mstr* mstr_init_dup(struct mstr *mstr, const char *data, size_t len)
{
    char *d = nalloc(sizeof(char)*(len+1));
    d[len] = 0x00;
    memcpy(d, data, len);
    return _mstr_init(mstr, d, len);
}

static inline struct mstr* mstr_alloc(char *data, size_t len)
{
    struct mstr *str = ncalloc(sizeof(struct mstr));
    mstr_init(str, data, len);
    return str;
}

static inline struct mstr* mstr_alloc_dup(const char *data, size_t len)
{
    struct mstr *str = (struct mstr *) ncalloc(sizeof(struct mstr));
    mstr_init_dup(str, data, len);
    return str;
}

static inline struct mstr* mstr_flush_dup(struct mstr *mstr)
{
    nalloc_free(mstr->data);
    mstr->len = 0;

    return mstr;
}

static inline void mstr_free(struct mstr *mstr)
{
    nalloc_free(mstr);
}

static inline void mstr_free_dup(struct mstr *mstr)
{
    nalloc_free(mstr->data);
    nalloc_free(mstr);
}

static inline struct mstr* mstr_concat(int count, ...)
{
    va_list argptr;
    size_t len = 0;
    int i;
    char *args[count], *data, *dp;
    int alen[count];

    va_start(argptr, count);
    for(i = 0; i < count; i++)
    {
        args[i] = va_arg(argptr, char *);
        len += alen[i] = strlen(args[i]);
    }
    va_end(argptr);

    dp = data = nalloc(len+1);
    data[len] = '\0';
    for(i = 0; i < count; i++)
    {
        memcpy(dp, args[i], alen[i]);
        dp += alen[i];
    }

    struct mstr *str = mstr_alloc(data, len);
    return str;
}

#endif // _MSTR_H_
