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
*       Abort flag.                                                     *
*           True = Abort if exception caught.                           *
*           False= Keep processing.                                     *
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

static char *S_logfile;

static int S_fatal = 0;

void set_exception_catcher (
    char *logfilename,
    int  abortOnError
) {
    /* Save logfile name, if it exists */
    if (logfilename)
        S_logfile = strdup (logfilename);

    /* Save error handling flag */
    S_fatal = abortOnError;

    SetUnhandledExceptionFilter (catchCrash);
}

static LONG WINAPI catchCrash (struct _EXCEPTION_POINTERS *excp)
{
    FILE   *fp;             /* Ptr to logfile or stdout */
    char   *msgp;           /* Ptr to explanation string*/

    static int s_incrash=0; /* Prevent recursion        */

    if (!s_incrash) {
        s_incrash = 1;

        /* Open log file */
        fp = stderr;
        if (S_logfile) {
            if ((fp = fopen (S_logfile, "a")) == NULL)
                fp = stderr;
        }

        /* Interpret the data in the exception pointers */
        msgp = analyzeException (excp);

        /* Copy it all to our crash dump file */
        fprintf (fp, msgp);

        /* Report program disposition */
        if (S_fatal)
            fprintf (fp, "Aborting processing\n");
        else
            fprintf (fp, "Attempting to continue ...\n");

        /* Close logfile */
        fclose (fp);

        s_incrash = 0;
    }

    /* Only return if we allow it */
    if (S_fatal)
        exit (1);
    return (EXCEPTION_EXECUTE_HANDLER);
}
