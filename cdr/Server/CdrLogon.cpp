/*
 * $Id: CdrLogon.cpp,v 1.3 2000-04-16 21:43:26 bkline Exp $
 *
 * Opens a new CDR session.
 *
 * Example of CdrLogon command:
 *  <CdrLogon>
 *   <UserName>mmr</UserName>
 *   <Password>***REDACTED***</Password>
 *  </CdrLogon>
 *
 * Example response:
 *  <CdrLogonResp>
 *   <SessionId>38F855FF-3-5D3O701PBFCG</SessionId>
 *  </CdrLogonResp>
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2000/04/15 14:05:10  bkline
 * First debugged version.
 *
 * Revision 1.1  2000/04/15 12:23:14  bkline
 * Initial revision
 */

#include <ctime>
#include <cstdlib>
#include "CdrCommand.h"
#include "CdrDbResultSet.h"

cdr::String cdr::logon(cdr::Session& session, 
                       const cdr::dom::Node& node, 
                       cdr::db::Connection& conn)
{
    // Extract the parameters from the command.  Caller catches exceptions.
    cdr::String userName, password;
    cdr::dom::Node child = node.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"UserName")
                userName = cdr::dom::getTextContent(child);
            else if (name == L"Password")
                password = cdr::dom::getTextContent(child);
        }
        child = child.getNextSibling();
    }

    // Look up the user in the database.
    static const char selectQuery[] = "SELECT id, password "
                                        "FROM usr "  
                                       "WHERE name = ?";
    cdr::db::Statement select(conn);
    select.setString(1, userName);
    cdr::db::ResultSet rs = select.executeQuery(selectQuery);
    if (!rs.next())
        throw cdr::Exception(L"Invalid logon credentials");
    int id = rs.getInt(1);
    cdr::String dbPassword = rs.getString(2);
    if (password != dbPassword)
        throw cdr::Exception(L"Invalid logon credentials");
   
    // Create a new row in the session table.
    char idBuf[256];
    unsigned long now = time(0);
    unsigned long ticks = clock();
        static char randomChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static size_t nRandomChars = sizeof randomChars - 1;
    srand(ticks);
    sprintf(idBuf, "%lX-%lX-%03d-%c%c%c%c%c%c%c%c%c%c%c%c",
        now, ticks, id, 
        randomChars[rand() % nRandomChars],
        randomChars[rand() % nRandomChars],
        randomChars[rand() % nRandomChars],
        randomChars[rand() % nRandomChars],
        randomChars[rand() % nRandomChars],
        randomChars[rand() % nRandomChars],
        randomChars[rand() % nRandomChars],
        randomChars[rand() % nRandomChars],
        randomChars[rand() % nRandomChars],
        randomChars[rand() % nRandomChars],
        randomChars[rand() % nRandomChars],
        randomChars[rand() % nRandomChars]);
    cdr::String sessionId = idBuf;
    cdr::db::Statement insert(conn);
    static const char insertQuery[] = 
        "INSERT INTO session(name, usr, initiated, last_act)"
        "     VALUES (?, ?, GETDATE(), GETDATE())";
    insert.setString(1, sessionId);
    insert.setInt(2, id);
    insert.executeQuery(insertQuery);
    
    // Populate the session object.
    session.lookupSession(sessionId, conn);

    // Send back the command response.
    cdr::String response = L"  <CdrLogonResp>\n   <SessionId>";
    response += sessionId + L"</SessionId>\n  </CdrLogonResp>\n";
    return response;
}
