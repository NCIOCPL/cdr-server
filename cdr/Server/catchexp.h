/************************************************************
* catchexp.h                                                *
*                                                           *
*   Header file for catchexp.c, subroutine for              *
*   intercepting crashes and preventing Windows             *
*   from writing a message to the console.                  *
*                                                           *
************************************************************/

#ifndef CATCHEXP_H
#define CATCHEXP_H

#ifdef __cplusplus
extern "C" {
#endif

void set_exception_catcher (char *logfilename);

#ifdef __cplusplus
}
#endif

#endif /* CATCHEXP_H */
