
#include <stdio.h>
#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)
#define WINDOWS
#include <windows.h>
#else
#ifdef _MAC_
#define OSX
#include <mach/mach.h>
#include <mach/mach_time.h>
static const uint64_t NANOS_PER_USEC = 1000ULL;
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

#define TIMER_USEC_PER_SEC 1000000

namespace trax {

timer_state timer_clock() {
#if defined(WINDOWS)
    timer_state status;
    QueryPerformanceCounter ((LARGE_INTEGER *) &status);
    return status;
#elif defined(OSX)
    mach_timebase_info_data_t timebase_info;
    mach_timebase_info(&timebase_info);
    return (timer_state)((timebase_info.denom * NANOS_PER_USEC) / timebase_info.numer);
#else
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return ((timer_state) tv.tv_sec * TIMER_USEC_PER_SEC) + tv.tv_usec;
#endif
}

double timer_elapsed(timer_state start) {
#if defined(WINDOWS)
    static timer_state freq = 0;
    timer_state delta, stop;

    if (freq == 0)
        QueryPerformanceFrequency ((LARGE_INTEGER *) &freq);

#else
#define freq TIMER_USEC_PER_SEC
    timer_state delta, stop;
#endif

    stop = timer_clock();
    delta = stop - start;
    return (double) delta / (double) freq;
}

}
