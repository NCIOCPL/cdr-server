/*
 * $Id: CdrLogoff.cpp,v 1.2 2000-05-03 15:25:41 bkline Exp $
 *
 * Closes a CDR session.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2000/04/16 21:41:18  bkline
 * Initial revision
 *
 */

#include "CdrCommand.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"

cdr::String cdr::logoff(cdr::Session& session, 
                        const cdr::dom::Node&,
                        cdr::db::Connection& conn)
{
    // Pop the logoff date/time into the session table.
    std::string query = "UPDATE session SET ended = GETDATE() WHERE id = ?";
    cdr::db::PreparedStatement update = conn.prepareStatement(query);
    update.setInt(1, session.id);
    update.executeQuery();

    
    // Clear out the session object's state.
    session.id    = 0;
    session.uid   = 0;
    session.name  = L"";
    session.uName = L"";

    // Send back the command response.
    return L"  <CdrLogoffResp/>\n";
}
