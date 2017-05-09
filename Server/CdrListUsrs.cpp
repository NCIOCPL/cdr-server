/*
 * Retrieves list of the existing CDR users.
 */

#include "CdrCommand.h"
#include "CdrDbStatement.h"
#include "CdrDbResultSet.h"

/**
 * Selects all rows in the usr table and displays the values from the name
 * column.
 */
cdr::String cdr::listUsrs(cdr::Session& session,
                          const cdr::dom::Node& commandNode,
                          cdr::db::Connection& dbConnection)
{
    // Make sure our user is authorized to list groups.
    if (!session.canDo(dbConnection, L"LIST USERS", L""))
        throw cdr::Exception(
                L"LIST USERS action not authorized for this user");

    // Submit the query to the database
    cdr::db::Statement s = dbConnection.createStatement();
    cdr::db::ResultSet r = s.executeQuery(" SELECT name                "
                                          "   FROM usr                 "
                                          "  WHERE expired IS NULL     "
                                          "     OR expired > GETDATE() ");

    // Pull in the names from the result set.
    cdr::String response;
    while (r.next()) {
        if (response.size() == 0)
            response = L"  <CdrListUsrsResp>\n";
        response += L"   <UserName>" + cdr::entConvert(r.getString(1))
                 +  L"</UserName>\n";
    }
    if (response.size() == 0)
        return L"  <CdrListUsrsResp/>\n";
    else
        return response + L"  </CdrListUsrsResp>\n";
}
