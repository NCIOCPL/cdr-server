/*
 * $Id: CdrCommand.h,v 1.1 2000-04-14 15:58:04 bkline Exp $
 *
 * Interface for CDR command handlers.
 *
 * $Log: not supported by cvs2svn $
 */

#ifndef CDR_COMMAND_
#define CDR_COMMAND_

#include "CdrSession.h"
#include "CdrDom.h"
#include "CdrString.h"
#include "CdrDbConnection.h"

namespace cdr {
    typedef cdr::String (*Command)(cdr::Session&, 
                                   const cdr::dom::Node&,
                                   cdr::DbConnection*);
    extern Command lookupCommand(const cdr::String&);
    extern cdr::String logon(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String logoff(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String checkAuth(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String addGrp(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String getGrp(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String modGrp(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String delGrp(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String listGrps(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String addUsr(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String getUsr(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String modUsr(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String delUsr(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String listUsrs(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String addDoc(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String repDoc(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String delDoc(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String validateDoc(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String search(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String getDoc(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String checkOut(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String checkIn(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String report(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String filter(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
    extern cdr::String getLinks(cdr::Session&, 
        const cdr::dom::Node&, cdr::DbConnection*);
}

#endif
