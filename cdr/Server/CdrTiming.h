/*
 * $Id: CdrTiming.h,v 1.1 2002-07-24 14:16:22 bkline Exp $
 *
 * Simple tool for timing processing.
 *
 * $Log: not supported by cvs2svn $
 */
#ifndef CDR_TIMING_H
#define CDR_TIMING_H
#include <stdio.h>
#include <windows.h>

namespace cdr {

class ProfTimer {
    DWORD started;
public:
    ProfTimer() : started(GetTickCount()) {}
    void showElapsed(const char* what) {
        DWORD now = GetTickCount();
        DWORD delta = now - started;
        started = now;
        fprintf(stderr, "%35s: %3lu.%03lu\n", what, delta / 1000L, 
                                                    delta % 1000L);
    }
};
}

#ifdef CDR_TIMINGS
#define SHOW_ELAPSED(d, t) t.showElapsed(d)
#define MAKE_TIMER(t) cdr::ProfTimer t
#else
#define SHOW_ELAPSED(d, t)
#define MAKE_TIMER(t)
#endif
#endif
