/*
 * $Id: CdrCommand.h,v 1.2 2000-04-15 12:05:34 bkline Exp $
 *
 * Interface for CDR command handlers.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2000/04/14 15:58:04  bkline
 * Initial revision
 *
 */

#ifndef CDR_COMMAND_
#define CDR_COMMAND_

#include "CdrSession.h"
#include "CdrDom.h"
#include "CdrString.h"
#include "CdrDbConnection.h"

namespace cdr {
    typedef String (*Command)(Session&, dom::Node&, db::Connection&);
    extern Command lookupCommand(const String&);
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
