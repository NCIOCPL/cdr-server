/************************************************************************
*   analyzeException()                                                  *
*                                                                       *
*   PURPOSE                                                             *
*       Convert the information in a Windows _EXCEPTION_POINTERS        *
*       structure to human readable form - as much as we can anyway.    *
*                                                                       *
*       Interpretation is written to a static buffer (static so that    *
*       an out of memory error won't occur during execution).           *
*                                                                       *
*   PASS                                                                *
*       Pointer to MS Windows structured exception information.         *
*                                                                       *
*   RETURN                                                              *
*       Pointer to null terminated string in the internal static        *
*         buffer, with interpretation information.                      *
*                                                                       *
*                                               Author: Alan Meyer      *
************************************************************************/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <process.h>
#include "catchexp.h"

#define MSGSIZE     256         /* Max size msg component   */
#define MSGBUFSIZE 1024         /* Max of all messages      */

/* Create message here in thread specific memory */
__declspec (thread) static char S_buf[MSGBUFSIZE];

/* Flag to prevent recursion - crash within crash */
__declspec (thread) static int S_inCrash=0;

static void prBuf (char **, char *, ...);

char *analyzeException (struct _EXCEPTION_POINTERS *excp)
{
    PEXCEPTION_RECORD ep;       /* Details of the crash     */
    time_t tm;                  /* For date/time            */
    char   *typep;              /* Exception type message   */
    char   *bufp;               /* Ptr place for next print */



    if (!S_inCrash) {
        S_inCrash = 1;


        /* Create ptr to ptr to next place to write in S_buf */
        bufp = &S_buf[0];

        /* Date/time/pid */
        time (&tm);
        prBuf (&bufp, "----------- Exception Report\n");
        prBuf (&bufp, "Date:           %s", ctime (&tm));
        prBuf (&bufp, "ProcessID:      %d\n", _getpid ());

        /* Exception information */
        ep = excp->ExceptionRecord;

        /* Print structure */
        prBuf (&bufp, "ExceptionCode:  %0x\n", ep->ExceptionCode);
        prBuf (&bufp, "ExceptionFlags: %0x\n", ep->ExceptionFlags);
        prBuf (&bufp, "ExceptionAddr:  %p\n",  ep->ExceptionAddress);
        prBuf (&bufp, "NumberParams:   %0x\n", ep->NumberParameters);

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
        prBuf (&bufp, "Exception:      %s\n", typep);

        /* Following doesn't seem to work in Win95, just NT */
        if (ep->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
            prBuf (&bufp, "Access violation:\n");
            prBuf (&bufp, "Memory address %p could not be ",
                     ep->ExceptionInformation[1]);
            prBuf (&bufp, (ep->ExceptionInformation[0]) ?
                     "written\n" : "read\n");
        }

        prBuf (&bufp, "----------- Exception end\n");

        /* Allow routine to be called again */
        S_inCrash = 1;
    }

    return S_buf;
}

/************************************************************************
*   prBuf()                                                             *
*                                                                       *
*   PURPOSE                                                             *
*       Print info to the buffer pointed to, and advance the pointer.   *
*       Subroutine of analyzeException.                                 *
*                                                                       *
*   PASS                                                                *
*       Pointer to pointer to buffer.                                   *
*       sprintf format string.                                          *
*       Optional args for string.                                       *
*                                                                       *
*   RETURN                                                              *
*       Void.                                                           *
************************************************************************/

static void prBuf (char **bufpp, char *fmt, ...)
{
    va_list args;       /* Ptr to first variable arg.*/
    char msg[MSGSIZE];  /* Assemble one message here */


    /* Initialize for variable arguments */
    va_start (args, fmt);

    /* Create message string */
    vsprintf (msg, fmt, args);
    va_end (args);

    /* Append it to the passed buffer */
    strcpy (*bufpp, msg);

    /* Advance, ready for next */
    *bufpp += strlen (msg);
}
