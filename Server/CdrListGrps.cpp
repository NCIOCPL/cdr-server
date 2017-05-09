/*
 * Retrieves list of the existing CDR group names.
 */

#include "CdrCommand.h"
#include "CdrDbStatement.h"
#include "CdrDbResultSet.h"

cdr::String cdr::listGrps(cdr::Session& session,
                          const cdr::dom::Node& commandNode,
                          cdr::db::Connection& conn)
{
    // Make sure our user is authorized to list groups.
    if (!session.canDo(conn, L"LIST GROUPS", L""))
        throw cdr::Exception(
                L"LIST GROUPS action not authorized for this user");

    // Submit the query to the database
    cdr::db::Statement s = conn.createStatement();
    cdr::db::ResultSet r = s.executeQuery("SELECT name FROM grp");

    // Pull in the names from the result set.
    cdr::String response;
    while (r.next()) {
        if (response.size() == 0)
            response = L"  <CdrListGrpsResp>\n";
        response += L"   <GrpName>" + r.getString(1) + L"</GrpName>\n";
    }
    if (response.size() == 0)
        return L"  <CdrListGrpsResp/>\n";
    else
        return response + L"  </CdrListGrpsResp>\n";
}
