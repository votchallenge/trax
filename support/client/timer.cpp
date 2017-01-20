
#include <stdio.h>
#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
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
#ifdef WIN32
    timer_state status;
    QueryPerformanceCounter ((LARGE_INTEGER *) &status);
    return status;
#else
    struct timeval tv;

    gettimeofday (&tv, NULL);

    return ((timer_state) tv.tv_sec * TIMER_USEC_PER_SEC) + tv.tv_usec;
#endif
}

double timer_elapsed(timer_state start) {
#ifdef WIN32
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
