/*
 * $Id: CdrSession.h,v 1.3 2000-04-16 21:37:25 bkline Exp $
 *
 * Information about the current login.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2000/04/15 12:15:37  bkline
 * Added uName and uid members.
 *
 * Revision 1.1  2000/04/14 16:01:08  bkline
 * Initial revision
 *
 */

#ifndef CDR_SESSION_
#define CDR_SESSION_

#include "CdrString.h"
#include "CdrDbConnection.h"

namespace cdr {
    struct Session {
        int id;
        String name;
        int uid;
        String uName;
        String lastStatus;
        void lookupSession(const String&, db::Connection&);
        void setLastActivity(db::Connection&) const;
    };
}

#endif
