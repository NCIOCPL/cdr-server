/*
 * $Id: CdrCommand.h,v 1.5 2000-04-22 18:57:38 bkline Exp $
 *
 * Interface for CDR command handlers.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.4  2000/04/22 15:34:56  bkline
 * Filled out documentation comments.
 *
 * Revision 1.3  2000/04/16 19:11:36  bkline
 * Added const qualifier to Node argument in Command signature.
 *
 * Revision 1.2  2000/04/15 12:05:34  bkline
 * Changed DbConnection* to DbConnection&.  Removed redundant namespace
 * qualifiers.
 *
 * Revision 1.1  2000/04/14 15:58:04  bkline
 * Initial revision
 */

#ifndef CDR_COMMAND_
#define CDR_COMMAND_

#include "CdrSession.h"
#include "CdrDom.h"
#include "CdrString.h"
#include "CdrDbConnection.h"

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

    /**
     * All external commands have this signature.
     */
    typedef String (*Command)(
            Session&,           // Contains information about current user.
            const dom::Node&,   // Contains XML for the command.
            db::Connection&     // Connection to the CDR database.
    );

    /**
     * Finds the command which matches the specified name.
     */
    extern Command lookupCommand(const String&);

    /**
     * Declarations of the external commands.  All commands are loaded
     * in a map collection.
     */
    extern String logon      (Session&, const dom::Node&, db::Connection&);
    extern String logoff     (Session&, const dom::Node&, db::Connection&);
    extern String checkAuth  (Session&, const dom::Node&, db::Connection&);
    extern String addGrp     (Session&, const dom::Node&, db::Connection&);
    extern String getGrp     (Session&, const dom::Node&, db::Connection&);
    extern String modGrp     (Session&, const dom::Node&, db::Connection&);
    extern String delGrp     (Session&, const dom::Node&, db::Connection&);
    extern String listGrps   (Session&, const dom::Node&, db::Connection&);
    extern String addUsr     (Session&, const dom::Node&, db::Connection&);
    extern String getUsr     (Session&, const dom::Node&, db::Connection&);
    extern String modUsr     (Session&, const dom::Node&, db::Connection&);
    extern String delUsr     (Session&, const dom::Node&, db::Connection&);
    extern String listUsrs   (Session&, const dom::Node&, db::Connection&);
    extern String addDoc     (Session&, const dom::Node&, db::Connection&);
    extern String repDoc     (Session&, const dom::Node&, db::Connection&);
    extern String delDoc     (Session&, const dom::Node&, db::Connection&);
    extern String validateDoc(Session&, const dom::Node&, db::Connection&);
    extern String search     (Session&, const dom::Node&, db::Connection&);
    extern String getDoc     (Session&, const dom::Node&, db::Connection&);
    extern String checkOut   (Session&, const dom::Node&, db::Connection&);
    extern String checkIn    (Session&, const dom::Node&, db::Connection&);
    extern String report     (Session&, const dom::Node&, db::Connection&);
    extern String filter     (Session&, const dom::Node&, db::Connection&);
    extern String getLinks   (Session&, const dom::Node&, db::Connection&);

}

#endif
