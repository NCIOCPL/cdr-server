/************************************************************************
*   set_exception_catcher ()                                            *
*                                                                       *
*   PURPOSE                                                             *
*       Intercept exceptions, overriding the default exception          *
*       handler.  Prevents Windows from locking the process and         *
*       writing a message to the console (very inconvenient if          *
*       operating remotely), waiting for a human operator to            *
*       press a key.                                                    *
*                                                                       *
*   PASS                                                                *
*       Log file name.                                                  *
*           If a crash occurs, relevant information is written to       *
*           the logfile.                                                *
*           If NULL, info is written to stderr.                         *
*                                                                       *
*   RETURN                                                              *
*       Void.                                                           *
*                                                                       *
*                                               Author: Alan Meyer      *
************************************************************************/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <process.h>
#include "catchexp.h"

static LONG WINAPI catchCrash (struct _EXCEPTION_POINTERS *excp);

static char *s_logfile;

void set_exception_catcher (
    char *logfilename
) {
    /* Save logfile name, if it exists */
    if (logfilename)
        s_logfile = strdup (logfilename);

    SetUnhandledExceptionFilter (catchCrash);
}

static LONG WINAPI catchCrash (struct _EXCEPTION_POINTERS *excp)
{
    PEXCEPTION_RECORD ep;   /* Details of the crash     */
    FILE   *fp;             /* Ptr to logfile or stdout */
    time_t tm;              /* For date/time            */
    char   *typep;          /* Exception type message   */

    static int s_incrash=0; /* Prevent recursion        */


    if (!s_incrash) {
        s_incrash = 1;

        /* Open log file */
        fp = stderr;
        if (s_logfile) {
            if ((fp = fopen (s_logfile, "a")) == NULL)
                fp = stderr;
        }

        /* Date/time/pid */
        time (&tm);
        fprintf (fp, "----------- Exception Report\n");
        fprintf (fp, "Date:           %s", ctime (&tm));
        fprintf (fp, "ProcessID:      %d\n", _getpid ());

        /* Exception information */
        ep = excp->ExceptionRecord;

        /* Print structure */
        fprintf (fp, "ExceptionCode:  %0x\n", ep->ExceptionCode);
        fprintf (fp, "ExceptionFlags: %0x\n", ep->ExceptionFlags);
        fprintf (fp, "ExceptionAddr:  %p\n",  ep->ExceptionAddress);
        fprintf (fp, "NumberParams:   %0x\n", ep->NumberParameters);

        /* Print type, from excpt.h */
        switch (ep->ExceptionCode) {

            case EXCEPTION_ACCESS_VIOLATION:
                typep = "EXCEPTION_ACCESS_VIOLATION";
                break;
            case EXCEPTION_DATATYPE_MISALIGNMENT:
                typep = "EXCEPTION_DATATYPE_MISALIGNMENT";
                break;
            case EXCEPTION_BREAKPOINT:
                typep = "EXCEPTION_BREAKPOINT";
                break;
            case EXCEPTION_SINGLE_STEP:
                typep = "EXCEPTION_SINGLE_STEP";
                break;
            case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
                typep = "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
                break;
            case EXCEPTION_FLT_DENORMAL_OPERAND:
                typep = "EXCEPTION_FLT_DENORMAL_OPERAND";
                break;
            case EXCEPTION_FLT_DIVIDE_BY_ZERO:
                typep = "EXCEPTION_FLT_DIVIDE_BY_ZERO";
                break;
            case EXCEPTION_FLT_INEXACT_RESULT:
                typep = "EXCEPTION_FLT_INEXACT_RESULT";
                break;
            case EXCEPTION_FLT_INVALID_OPERATION:
                typep = "EXCEPTION_FLT_INVALID_OPERATION";
                break;
            case EXCEPTION_FLT_OVERFLOW:
                typep = "EXCEPTION_FLT_OVERFLOW";
                break;
            case EXCEPTION_FLT_STACK_CHECK:
                typep = "EXCEPTION_FLT_STACK_CHECK";
                break;
            case EXCEPTION_FLT_UNDERFLOW:
                typep = "EXCEPTION_FLT_UNDERFLOW";
                break;
            case EXCEPTION_INT_DIVIDE_BY_ZERO:
                typep = "EXCEPTION_INT_DIVIDE_BY_ZERO";
                break;
            case EXCEPTION_INT_OVERFLOW:
                typep = "EXCEPTION_INT_OVERFLOW";
                break;
            case EXCEPTION_PRIV_INSTRUCTION:
                typep = "EXCEPTION_PRIV_INSTRUCTION";
                break;
            case EXCEPTION_IN_PAGE_ERROR:
                typep = "EXCEPTION_IN_PAGE_ERROR";
                break;
            case EXCEPTION_ILLEGAL_INSTRUCTION:
                typep = "EXCEPTION_ILLEGAL_INSTRUCTION";
                break;
            case EXCEPTION_NONCONTINUABLE_EXCEPTION:
                typep = "EXCEPTION_NONCONTINUABLE_EXCEPTION";
                break;
            case EXCEPTION_STACK_OVERFLOW:
                typep = "EXCEPTION_STACK_OVERFLOW";
                break;
            case EXCEPTION_INVALID_DISPOSITION:
                typep = "EXCEPTION_INVALID_DISPOSITION";
                break;
            case EXCEPTION_GUARD_PAGE:
                typep = "EXCEPTION_GUARD_PAGE";
                break;
            case EXCEPTION_INVALID_HANDLE:
                typep = "EXCEPTION_INVALID_HANDLE";
                break;
            default:
                typep = "Unknown";
        }
        fprintf (fp, "Exception:      %s\n", typep);

        /* Following doesn't seem to work in Win95, just NT */
        if (ep->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
            fprintf (fp, "Access violation:\n");
            fprintf (fp, "Memory address %p could not be ",
                     ep->ExceptionInformation[1]);
            fprintf (fp, (ep->ExceptionInformation[0]) ?
                     "written\n" : "read\n");
        }
        
        fprintf (fp, "----------- Exception end\n");

        /* Close logfile */
        fclose (fp);
    }

    return (EXCEPTION_EXECUTE_HANDLER);
}
