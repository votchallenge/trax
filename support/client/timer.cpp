
#include <stdio.h>
#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)
#define WINDOWS
#include <windows.h>
#else
#ifdef __APPLE__
#define OSX
#include <mach/mach.h>
#include <mach/mach_time.h>
#else
#define LINUX
#include <sys/time.h>
#endif
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#elif HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "trax/client.hpp"

#define TIMER_USEC_PER_SEC 1000000ULL
#define TIMER_NSEC_PER_USEC 1000ULL

namespace trax {


#if defined(WINDOWS)
static timer_state freq = 0;
#else
static timer_state freq = TIMER_USEC_PER_SEC;
#endif

void timer_init() {
#if defined(WINDOWS)
    if (freq == 0) {
        LARGE_INTEGER tmp;
        QueryPerformanceFrequency (&tmp);
        freq = tmp.QuadPart;
    }
#endif
}


timer_state timer_clock() {
#if defined(WINDOWS)
    LARGE_INTEGER tmp;
    QueryPerformanceCounter (&tmp);
    return tmp.QuadPart;
#elif defined(OSX)
    mach_timebase_info_data_t timebase_info;
    mach_timebase_info(&timebase_info);
    return (timer_state)((timebase_info.denom * TIMER_NSEC_PER_USEC) / timebase_info.numer);
#else
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tv);
    return ((timer_state) tv.tv_sec * TIMER_USEC_PER_SEC) + (tv.tv_nsec / TIMER_NSEC_PER_USEC);
#endif
}

double timer_elapsed(timer_state start) {
    timer_state delta, stop;
    stop = timer_clock();
    delta = stop - start;
    return (double) delta / (double) freq;
}

}
