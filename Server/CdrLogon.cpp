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

static const int MAX_FAILED_LOGONS = 10;

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

    int failCount = cdr::getLoginFailedCount(conn, userName);
    if (failCount == -1)
        // Record for this username doesn't exist
        throw cdr::Exception(L"Invalid logon credentials");

    // If user has tried too many times, don't go any further
    if (failCount > MAX_FAILED_LOGONS) {
        // Keep track of count to help find problematic user IDs
        cdr::setLoginFailedCount(conn, userName, 1);

        // But block login
        throw cdr::Exception(L"Too many consecutive login failures.  "
            L"Please ask administrative staff to reset your password.");
    }

    // Look up the user in the database, checking name & pw to get ID
    int userId = cdr::chkIdPassword(userName, password, conn);
    if (userId == -1) {
        // Increment the login failure count
        cdr::setLoginFailedCount(conn, userName, 1);

        // Block the user
        throw cdr::Exception(L"Invalid logon credentials");
    }

    // Create a new session in the database
    cdr::String sessionId = createSessionRecord(userId, userName, conn);

    // Populate the session object.
    session.lookupSession(sessionId, conn);

    // Clear the record of failed logins if needed
    if (failCount > 0)
        cdr::setLoginFailedCount(conn, userName, 0);

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
