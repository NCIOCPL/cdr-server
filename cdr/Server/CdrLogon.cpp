/*
 * $Id: CdrLogon.cpp,v 1.1 2000-04-15 12:23:14 bkline Exp $
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
 */

#include <ctime>
#include <cstdlib>
#include "CdrCommand.h"
#include "CdrDbResultSet.h"

cdr::String logon(cdr::Session& session, 
                  const cdr::dom::Node& node, 
                  cdr::db::Connection& conn)
{
    // Extract the parameters from the command.  Caller catches exceptions.
    cdr::String uid, pwd;
    cdr::dom::Node child = node.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            cdr::String value = child.getNodeValue();
            if (name == L"UserName")
                uid = value;
            else if (name == L"Password")
                pwd = value;
        }
        child = child.getNextSibling();
    }

    // Look up the user in the database.
    static const char selectQuery[] = "SELECT id, password"
                                        "FROM usr"
                                       "WHERE name = ?";
    cdr::db::Statement select(conn);
    select.setString(1, uid);
    cdr::db::ResultSet rs = select.executeQuery(selectQuery);
    if (!rs.next() || pwd != rs.getString(2))
        throw cdr::Exception(L"Invalid logon credentials");
    int id = rs.getInt(1);
   
    // Create a new row in the session table.
    char idBuf[256];
    unsigned long now = time(0);
        static char randomChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static size_t nRandomChars = sizeof randomChars - 1;
    static bool randomized = false;
    if (!randomized) {
        srand(now);
        randomized = true;
    }
    sprintf(idBuf, "%lX-%d-%c%c%c%c%c%c%c%c%c%c%c%c",
        now, id, 
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
    
    // Send back the command response.
    cdr::String response = L"  <CdrLogonResp>\n   <SessionId>";
    response += sessionId + L"</SessionId>\n  </CdrLogonResp>\n";
    return response;
}
