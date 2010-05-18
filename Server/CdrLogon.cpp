/*
 * $Id$
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
 * Revision 1.8  2003/08/04 17:03:26  bkline
 * Fixed breakage caused by upgrade to latest version of Microsoft's
 * C++ compiler.
 *
 * Revision 1.7  2002/06/07 13:54:06  bkline
 * Added case-sensitive check of user name at Lakshmi's request (issue #257).
 *
 * Revision 1.6  2002/04/10 14:32:14  bkline
 * Added logging of failed login attempts.
 *
 * Revision 1.5  2000/06/23 15:28:01  bkline
 * Fixed bug which was causing the size of the session.name column to be
 * exceeded.
 *
 * Revision 1.4  2000/05/03 15:25:41  bkline
 * Fixed database statement creation.
 *
 * Revision 1.3  2000/04/16 21:43:26  bkline
 * Modified composition of sessionId slightly.  Added call to lookupSession.
 *
 * Revision 1.2  2000/04/15 14:05:10  bkline
 * First debugged version.
 *
 * Revision 1.1  2000/04/15 12:23:14  bkline
 * Initial revision
 */

#include <ctime>
#include <cstdlib>
#include "CdrCommand.h"
#include "CdrDbPreparedStatement.h"
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
    static const char selectQuery[] = "SELECT id, name, password "
                                        "FROM usr "  
                                       "WHERE name = ?";
    cdr::db::PreparedStatement select = conn.prepareStatement(selectQuery);
    select.setString(1, userName);
    cdr::db::ResultSet rs = select.executeQuery();
    if (!rs.next()) {
        cdr::log::pThreadLog->Write(L"Failed logon attempt (invalid user name)",
                L"name: " + userName + L"; password: " + password);
        throw cdr::Exception(L"Invalid logon credentials");
    }
    int id                 = rs.getInt(1);
    cdr::String dbName     = rs.getString(2);
    cdr::String dbPassword = rs.getString(3);
    if (userName != dbName) {
        cdr::log::pThreadLog->Write(
                L"Failed logon attempt (user name case mismatch)",
                L"user typed: " + userName + L"; name in database: " + dbName);
        throw cdr::Exception(L"Invalid logon credentials");
    }
    if (password != dbPassword) {
        cdr::log::pThreadLog->Write(L"Failed logon attempt (invalid password)",
                L"name: " + userName + L"; password: " + password);
        throw cdr::Exception(L"Invalid logon credentials");
    }
   
    // Create a new row in the session table.
    char idBuf[256];
    unsigned long now = (unsigned long)time(0);
    unsigned long ticks = clock();
    static char randomChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static size_t nRandomChars = sizeof randomChars - 1;
    srand(ticks);
    sprintf(idBuf, "%lX-%06lX-%03d-%c%c%c%c%c%c%c%c%c%c%c%c",
        now, ticks & 0xFFFFFF, id % 1000, 
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
    static const char insertQuery[] = 
        "INSERT INTO session(name, usr, initiated, last_act)"
        "     VALUES (?, ?, GETDATE(), GETDATE())";
    cdr::db::PreparedStatement insert = conn.prepareStatement(insertQuery);
    insert.setString(1, sessionId);
    insert.setInt(2, id);
    insert.executeQuery();
    
    // Populate the session object.
    session.lookupSession(sessionId, conn);

    // Send back the command response.
    cdr::String response = L"  <CdrLogonResp>\n   <SessionId>";
    response += sessionId + L"</SessionId>\n  </CdrLogonResp>\n";
    return response;
}
