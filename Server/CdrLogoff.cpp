/*
 * Closes a CDR session.
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
