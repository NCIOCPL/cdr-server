#include <stdlib.h>
#include <math.h>
#include <map>
#include <string>
#include "CdrTiming.h"
#include "CdrDbConnection.h"
#include "CdrDbStatement.h"
#include "CdrDbResultSet.h"
#include "CdrException.h"

namespace cdr {


//-----------------------------------------------------------
//  FancyTimer
//-----------------------------------------------------------
    // Display resolution divider, for seconds.milliseconds
    static const DWORD RES = (DWORD) 1000;

    // Largest integer
    static const DWORD MAXTIC = (DWORD) 0xFFFFFFFFFFFFFFFF;

    cdr::FancyTimer::FancyTimer() {
        init();
    }

    void cdr::FancyTimer::init() {
        count   = 0;
        started = false;
        startTime = max = sum = sumSquares = (DWORD) 0;
        min = MAXTIC;
    }

    void cdr::FancyTimer::start() {
        if (started)
            throw cdr::Exception (L"FancyTimer start called twice in a row");
        startTime = GetTickCount();

        started = true;
    }

    void cdr::FancyTimer::stop() {

        if (!started)
            throw cdr::Exception (L"FancyTimer stopped without start");

        // Get amount of time passed, in ticks
        DWORD amount;
        DWORD last = GetTickCount();
        if (last < startTime)
            // Adjust for clock tick wraparound every 47 days.
            amount = (MAXTIC - startTime) + last;
        else
            amount = (last - startTime);

        // Update info
        count += 1;
        if (min > amount)
            min = amount;
        if (max < amount)
            max = amount;
        sum += amount;

        // Sum of squares can overflow
        // Round off 4 binary digits to avoid that
        // Round off accounts for squaring (this is a FancyTimer!)
        // We put them back later
        amount = (amount >> 4) + ((amount & (DWORD) 0x0C) > 11) ? 1 : 0;
        sumSquares = amount * amount;

        started = false;
    }

    void cdr::FancyTimer::show(char *buf) {

        if (started)
            throw cdr::Exception (L"FancyTimer show without stop");

        // Get standard deviation
        DWORD avg = sum / count;
        double dblDev = sqrt (((double) sumSquares) / count);

        // Restore radix lost in roundoff
        DWORD stdDev = ((DWORD) dblDev) << 4;

        // Format results for caller
        sprintf (buf, " Count=%8ld  Min=%3ld.%03ld  Max=%3ld.%03ld"
                      "  Avg=%3ld.%03ld stdDev=%3ld.%03ld",
                 count, min/RES, min%RES, max/RES,max%RES,
                        avg/RES, avg%RES, stdDev/RES, stdDev%RES);
    }
} // Namespace cdr
