/*
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
 * JIRA::OCECDR-3849 - Integrate CDR login with NIH Active Directory
 */

#include <ctime>
#include <cstdlib>
#include "CdrCommand.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"

// Create session for a user authenticated by Windows.
static cdr::String windowsLogon(cdr::db::Connection& conn,
                                const cdr::String& windowsName,
                                cdr::Session& session) {

    // Find the CDR account for the Windows user.
    int usrId = 0;
    cdr::String usrName;
    cdr::db::PreparedStatement select = conn.prepareStatement(
        "SELECT id, name"
        "  FROM usr"
        " WHERE name = ?"
        "   AND expired IS NULL"
        "   AND password = ''"
        "   AND hashedpw = HASHBYTES('SHA1', '')");
    select.setString(1, windowsName);
    cdr::db::ResultSet rs = select.executeQuery();
    if (rs.next()) {
        usrId = rs.getInt(1);
        usrName = rs.getString(2);
    }
    select.close();
    if (!usrId || usrName.empty()) {
        cdr::log::pThreadLog->Write(L"Windows user not found", windowsName);
        throw cdr::Exception(L"Invalid logon credentials");
    }

    // Create a new session row in the database
    cdr::String sessionId = createSessionRecord(usrId, usrName, conn);

    // Populate the session object.
    session.lookupSession(sessionId, conn);

    // Send back the command response.
    cdr::String response = L"<CdrLogonResp><SessionId>" + sessionId +
        L"</SessionId></CdrLogonResp>";
    return response;
}

// CdrCommand.h
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

    if (password.empty())
        return windowsLogon(conn, userName, session);

    // Look up the user in the database, checking name & pw to get ID
    int userId = cdr::chkIdPassword(userName, password, conn);
    if (userId == -1) {

        // Block the user
        throw cdr::Exception(L"Invalid logon credentials");
    }

    // Create a new session in the database
    cdr::String sessionId = createSessionRecord(userId, userName, conn);

    // Populate the session object.
    session.lookupSession(sessionId, conn);

    // Send back the command response.
    cdr::String response = L"  <CdrLogonResp>\n   <SessionId>";
    response += sessionId + L"</SessionId>\n  </CdrLogonResp>\n";
    return response;
}

// CdrCommand.h
cdr::String cdr::dupSession(
    cdr::Session&   session,
    const cdr::dom::Node& node,
    cdr::db::Connection&  conn
) {
    int         oldSessionId;
    cdr::String oldSessionName,
                newSessionName;

    // Don't need to parse the command, the session identifiers are
    //  already parsed out in the Session object
    oldSessionId   = session.getId();
    oldSessionName = session.getName();

    // Replicate it
    cdr::String comment =
            L"Session duplicated from id=" + String::toString(oldSessionId);
    newSessionName = duplicateSessionRecord(oldSessionName, conn, comment);

    // Send back the command response.
    cdr::String response = L"  <CdrDupSessionResp>\n";
    response += L"   <sessionId>" + oldSessionName + L"</SessionId>\n";
    response += L"   <newSessionId>" + newSessionName + L"</newSessionId>\n";
    response += L"  </CdrDupSessionResp>\n";
    return response;
}
