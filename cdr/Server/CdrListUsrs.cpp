
/*
 * $Id: CdrListUsrs.cpp,v 1.3 2000-05-03 15:25:41 bkline Exp $
 *
 * Retrieves list of the existing CDR users.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2000/04/23 01:19:58  bkline
 * Added function-level comment header.
 *
 * Revision 1.1  2000/04/22 09:25:23  bkline
 * Initial revision
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
    cdr::db::ResultSet r = s.executeQuery("SELECT name FROM usr");
    
    // Pull in the names from the result set.
    cdr::String response;
    while (r.next()) {
        if (response.size() == 0)
            response = L"  <CdrListUsrsResp>\n";
        response += L"   <UserName>" + r.getString(1) + L"</UserName>\n";
    }
    if (response.size() == 0)
        return L"  <CdrListUsrsResp/>\n";
    else
        return response + L"  </CdrListUsrsResp>\n";
}
