/*
 * $Id: CdrSession.h,v 1.2 2000-04-15 12:15:37 bkline Exp $
 *
 * Information about the current login.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2000/04/14 16:01:08  bkline
 * Initial revision
 *
 */

#ifndef CDR_SESSION_
#define CDR_SESSION_

#include "CdrString.h"

namespace cdr {
    struct Session {
        cdr::String id;
        cdr::String uName;
        int uid;
    };
}

#endif
