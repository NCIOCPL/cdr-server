/*
 * $Id: CdrLogoff.cpp,v 1.5 2002-08-10 19:28:21 bkline Exp $
 *
 * Closes a CDR session.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.4  2000/05/09 21:08:19  bkline
 * Replaced attempt to use private Session field directly with accessor
 * method call.
 *
 * Revision 1.3  2000/05/09 20:14:56  bkline
 * Replaced direct manipulation of Session fields with clear() method.
 *
 * Revision 1.2  2000/05/03 15:25:41  bkline
 * Fixed database statement creation.
 *
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
    // Don't log the special guest account off.
    if (session.getName() != L"guest") {

        // Pop the logoff date/time into the session table.
        std::string query = 
            "UPDATE session SET ended = GETDATE() WHERE id = ?";
        cdr::db::PreparedStatement update = conn.prepareStatement(query);
        update.setInt(1, session.getId());
        update.executeQuery();
    }

    
    // Clear out the session object's state.
    session.clear();

    // Send back the command response.
    return L"  <CdrLogoffResp/>\n";
}
