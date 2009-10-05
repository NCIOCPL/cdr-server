/************************************************************
* catchexp.h                                                *
*                                                           *
*   Header file for subroutines for                         *
*   intercepting crashes and preventing Windows             *
*   from writing a message to the console.                  *
*                                                           *
************************************************************/

#ifndef CATCHEXP_H
#define CATCHEXP_H
#include <windows.h>
#include <process.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Replaces Windows default exception catcher with our own
 * This can still be used, but is superseded by one that
 *  throws a C++ exception.
 */

void set_exception_catcher (char *logfilename, int abortOnError);

/* Interpret info in a Windows structured exception block */
char *analyzeException (struct _EXCEPTION_POINTERS *excp);

#ifdef __cplusplus
}
#endif

#endif /* CATCHEXP_H */
