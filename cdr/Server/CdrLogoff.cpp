/*
 * $Id: CdrLogoff.cpp,v 1.1 2000-04-16 21:41:18 bkline Exp $
 *
 * Closes a CDR session.
 *
 * $Log: not supported by cvs2svn $
 */

#include "CdrCommand.h"
#include "CdrDbResultSet.h"

cdr::String cdr::logoff(cdr::Session& session, 
                        const cdr::dom::Node&,
                        cdr::db::Connection& conn)
{
    // Pop the logoff date/time into the session table.
    cdr::db::Statement update(conn);
    update.setInt(1, session.id);
    update.executeQuery("UPDATE session"
                        "   SET ended = GETDATE()"
                        " WHERE id    = ?");

    
    // Clear out the session object's state.
    session.id    = 0;
    session.uid   = 0;
    session.name  = L"";
    session.uName = L"";

    // Send back the command response.
    return L"  <CdrLogoffResp/>\n";
}
