#ifndef _UTIME_H_
#define _UTIME_H_

#include <sys/time.h>
#include <stdint.h>
#include <unistd.h>

#define utime_t     uint64_t

static inline const uint64_t utime_time()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return ((uint64_t)(t.tv_sec*1000000) + (uint64_t)t.tv_usec);
}

// @brief Get cpu time
static inline uint64_t utime_cpu()
{
	unsigned int lo,hi;
	__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
	return ((uint64_t)hi << 32) | lo;
}

#endif // _UTIME_H_
