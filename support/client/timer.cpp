
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

#define TIMER_NSEC_PER_SEC  1000000000ULL
#define TIMER_USEC_PER_SEC  1000000ULL
#define TIMER_NSEC_PER_USEC 1000ULL

namespace trax {


static double freq = 0;

void timer_init() {
    if (freq != 0) {
        // Already initialized
        return;
    }

#if defined(WINDOWS)
    LARGE_INTEGER tmp;
    QueryPerformanceFrequency (&tmp);
    freq = tmp.QuadPart;
#elif defined(OSX)
    // Convert Mach Absolute Time Units to seconds, use double to avoid overflow.
    // See more on https://developer.apple.com/library/content/qa/qa1398/_index.html
    mach_timebase_info_data_t timebase_info;
    mach_timebase_info(&timebase_info);
    freq = TIMER_NSEC_PER_SEC * timebase_info.denom / (double) timebase_info.numer;
#else
    freq = TIMER_USEC_PER_SEC;
#endif
}


timer_state timer_clock() {
#if defined(WINDOWS)
    LARGE_INTEGER tmp;
    QueryPerformanceCounter (&tmp);
    return tmp.QuadPart;
#elif defined(OSX)
    return mach_absolute_time();
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

    return (double) delta / freq;
}

}
