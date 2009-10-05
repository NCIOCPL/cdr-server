/*
 * $Id: CdrTiming.h,v 1.3 2004-04-30 01:32:21 ameyer Exp $
 *
 * Simple tool for timing processing.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2002/07/25 10:05:17  bkline
 * Added ccdoc comment markers for cdr namespace.
 *
 * Revision 1.1  2002/07/24 14:16:22  bkline
 * Simple tool for timing instrumentation.
 *
 */
#ifndef CDR_TIMING_H
#define CDR_TIMING_H
#include <stdio.h>
#include <windows.h>
#include <string>
#include <map>

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

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


/**
 * Fancy timer gets count, min, max, average and standard deviation
 * for multiple calls involving a single timer.
 */
class FancyTimer {
    public:
        /**
         * Create and initialize a timer.
         */
        FancyTimer();

        /**
         * (Re-)initialize a timer.
         * Can start it over at 0.
         */
        void init();

        /**
         * Start timing an activity with this timer.  Can stop and start again
         * later.
         *
         * @throw cdr::Exception if started twice without intervening stop.
         */
        void start();

        /**
         * Stop timing an activity.  Can restart later.
         * Accumulates timing info since previous start.
         *
         * @throw cdr::Exception if not previously started.
         */
        void stop();

        /**
         * Return info from the timing in human readable form.
         *
         * @param buf       Timing info copied here.  MUST be long enough.
         *                  100 chars is more than enough.
         *
         * @throw cdr::Exception if started and not stopped.
         */
        void show (char *buf);


    private:
        int     count;          // Num times started since init
        boolean started;        // True=started, if false and stopped, error
        DWORD   startTime;      // Millisecond clock tick value at start
        DWORD   max;            // Max ticks in any start/stop interval
        DWORD   min;            // Min ticks in interval
        DWORD   sum;            // Sum ticks in all intervals since init
        DWORD   sumSquares;     // Sum squares, rounded down
};

} // Namespace cdr

#ifdef CDR_TIMINGS
#define SHOW_ELAPSED(d, t) t.showElapsed(d)
#define MAKE_TIMER(t) cdr::ProfTimer t
#else
#define SHOW_ELAPSED(d, t)
#define MAKE_TIMER(t)
#endif
#endif
