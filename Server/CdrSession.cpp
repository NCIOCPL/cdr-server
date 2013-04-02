/*
 * $Id$
 *
 * Session control information.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.10  2002/08/23 02:03:43  ameyer
 * Now normalizing action string whitespace in canDo().
 *
 * Revision 1.9  2002/06/18 22:16:03  ameyer
 * Bug fix in new getCanDo().
 *
 * Revision 1.8  2002/06/18 20:44:46  ameyer
 * Oops, return from CanDo should be "<CdrCanDoResp>...".
 *
 * Revision 1.7  2002/06/18 20:34:08  ameyer
 * Added CdrCanDo command.
 *
 * Revision 1.6  2000/10/30 17:41:00  mruben
 * added canDo for document number
 *
 * Revision 1.5  2000/05/03 15:25:41  bkline
 * Fixed database statement creation.
 *
 * Revision 1.4  2000/04/22 09:30:22  bkline
 * Fixed a couple of typo bugs.
 *
 * Revision 1.3  2000/04/21 13:59:00  bkline
 * Added canDo() method.
 *
 * Revision 1.2  2000/04/17 03:10:21  bkline
 * Fixed typo in wide string constant.
 *
 * Revision 1.1  2000/04/16 21:38:32  bkline
 * Initial revision
 *
 */

#include <ctime>
#include "CdrSession.h"
#include "CdrCommand.h"
#include "CdrDom.h"
#include "CdrString.h"
#include "CdrDbConnection.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"
#include "CdrException.h"

// #include <iostream> // DEBUG

// The password hashing algorithm to try
static std::string s_hashAlgo = "SHA1";

// This determines if our SQL Server DBMS can use it
static bool dbHasHash(cdr::db::Connection&, const std::string&);

// See CdrSession.h
cdr::String cdr::createSessionRecord(
    const int           userId,
    const cdr::String&  userName,
    cdr::db::Connection conn,
    const cdr::String&  comment
) {
    // Double check user ID - though it should be done elsewhere
    static const char selectQuery[] =
        "SELECT id, name"
        " FROM usr "
        " WHERE id = ? AND expired IS NULL";
    cdr::db::PreparedStatement select = conn.prepareStatement(selectQuery);
    select.setInt(1, userId);
    cdr::db::ResultSet rs = select.executeQuery();

    // Check user exists and not expired
    if (!rs.next()) {
        cdr::log::pThreadLog->Write(
                L"Failed logon attempt (invalid or expired user name) ",
                L"name: " + userName);
        throw cdr::Exception(L"Invalid logon credentials");
    }

    // Confirm exact name match
    int id             = rs.getInt(1);
    cdr::String dbName = rs.getString(2);
    if (userName != dbName) {
        cdr::log::pThreadLog->Write(
                L"Failed logon attempt (user name case mismatch)",
                L"user typed: " + userName + L"; name in database: " + dbName);
        throw cdr::Exception(L"Invalid logon credentials");
    }
    select.close();

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
        "INSERT INTO session(name, usr, comment, initiated, last_act)"
        "     VALUES (?, ?, ?, GETDATE(), GETDATE())";
    cdr::db::PreparedStatement insert = conn.prepareStatement(insertQuery);
    insert.setString(1, sessionId);
    insert.setInt(2, id);
    insert.setString(3, comment);
    insert.executeQuery();
    insert.close();

    return sessionId;
}

// See CdrSession.h
cdr::String cdr::duplicateSessionRecord(
    const cdr::String&   oldSession,
    cdr::db::Connection& conn,
    const cdr::String&   comment
) {
    int         usrId;
    cdr::String usrName,
                newSessionId;

    // Check that existing session exists and is active
    std::string query =
        "SELECT s.usr, u.name"
        "  FROM session s"
        "  JOIN usr u ON s.usr = u.id"
        " WHERE s.name = ? AND ended IS NULL";
    cdr::db::PreparedStatement select = conn.prepareStatement(query);
    select.setString(1, oldSession);
    cdr::db::ResultSet rs = select.executeQuery();
    if (!rs.next())
        throw cdr::Exception(L"Can't duplicate invalid or expired session",
                               oldSession);
    usrId   = rs.getInt(1);
    usrName = rs.getString(2);
    select.close();

    // Create and return new session record
    return createSessionRecord(usrId, usrName, conn, comment);
}


// See CdrSession.h
void cdr::Session::lookupSession(
    const cdr::String&   sessionId,
    cdr::db::Connection& conn
) {
    std::string query = "SELECT s.id,"
                        "       s.usr,"
                        "       u.name"
                        "  FROM session s"
                        "  JOIN usr     u"
                        "    ON s.usr   = u.id"
                        " WHERE s.name  = ?"
                        "   AND s.ended IS NULL";
    cdr::db::PreparedStatement select = conn.prepareStatement(query);
    select.setString(1, sessionId);
    cdr::db::ResultSet rs = select.executeQuery();
    if (!rs.next())
      throw cdr::Exception(L"Invalid or expired session", L"'"+sessionId+L"'");
    name  = sessionId;
    id    = rs.getInt(1);
    uid   = rs.getInt(2);
    uName = rs.getString(3);
}

void cdr::Session::setLastActivity(db::Connection& conn,
                                   const String& ip) const
{
    std::string query = "UPDATE session "
                        "   SET last_act = GETDATE(),"
                        "       ip_address = ?"
                        " WHERE id = ?";
    cdr::db::PreparedStatement update = conn.prepareStatement(query);
    update.setString(1, ip);
    update.setInt(2, id);
    update.executeQuery();
}

bool cdr::Session::canDo(db::Connection& conn,
                         const cdr::String& action,
                         const cdr::String& docType) const
{
    std::string query = "SELECT COUNT(*)"
                        "  FROM grp_usr    gu,"
                        "       grp_action ga,"
                        "       action     a,"
                        "       doc_type   t"
                        " WHERE ga.action   = a.id"
                        "   AND ga.doc_type = t.id"
                        "   AND ga.grp      = gu.grp"
                        "   AND gu.usr      = ?"
                        "   AND a.name      = ?"
                        "   AND t.name      = ?";
    cdr::db::PreparedStatement ps = conn.prepareStatement(query);
    ps.setInt(1, uid);
    ps.setString(2, action);
    ps.setString(3, docType);
    cdr::db::ResultSet rs = ps.executeQuery();
    if (!rs.next())
        // Shouldn't ever reach here.
        throw cdr::Exception(L"Database failure extracting result count");
    int count = rs.getInt(1);
    return count > 0;
}


// See CdrSession.h
int cdr::chkIdPassword(
    cdr::String& userName,
    cdr::String& password,
    cdr::db::Connection& conn
) {
    // Look in the plain text or hashed password
    bool useHash = dbHasHash(conn, s_hashAlgo);

    // Lookup user info
    cdr::String chkdPassword(password);
    std::string query;
    if (useHash) {
        // Implementation note:
        // Hashing single byte characters and double byte characters produces
        //  different results.  We're doing everything as single byte,
        //  which obviates use of qmark processing with setString().
        std::string pw = password.toUtf8();
        query =
            "SELECT id, name, hashedpw"
            "  FROM usr"
            " WHERE name = ?"
            "   AND HASHBYTES('" + s_hashAlgo + "', '" + pw + "') = hashedpw";
    }
    else {
        query =
            "SELECT id, name, password "
            "  FROM usr "
            " WHERE name = ?";
            // " WHERE name = ? AND password = ?";
    }
    cdr::db::PreparedStatement select = conn.prepareStatement(query);

    select.setString(1, userName);
    // select.setString(2, password);
    cdr::db::ResultSet rs = select.executeQuery();
    if (!rs.next()) {
        // No match
        return -1;
    }

    // Name matching is case insensitive, make further check here
    int usrId              = rs.getInt(1);
    cdr::String dbName     = rs.getString(2);
    cdr::String dbPassword = rs.getString(3);
    select.close();

    if (userName != dbName) {
        cdr::log::pThreadLog->Write(
                L"Failed logon attempt (user name case mismatch)",
                L"user typed: " + userName + L"; name in database: " + dbName);
        return -1;
    }

    // Check passwords
    if (!useHash && (chkdPassword != dbPassword)) {
        // Mismatch on chkdPassword / dbPassword, has to be case difference
        cdr::log::pThreadLog->Write(L"Failed logon attempt (invalid password)",
                L"name: " + userName);
        return -1;
    }

    // Found it
    return usrId;
}

void cdr::testPasswordString(
    const cdr::String& password
) {
    const wchar_t *p;   // Characters in the password
    int c;              // Single char from the password
    int hasUpper = 0;   // Characteristics we have found
    int hasLower = 0;
    int hasDigit = 0;
    int hasPunct = 0;

    // Minimum length
    if (password.length() < 8)
        throw cdr::Exception("Passwords must be at least 8 characters");
    if (password.length() > 32)
        // This is only necessary while we allow plain text pw's
        throw cdr::Exception(
                "Sorry, the maximum length of passwords is 32 characters");

    p = password.c_str();
    while (*p) {
        c = (int) *p;
        // Test for legal char types
        if (iswupper(c))
            hasUpper = 1;
        else if (iswlower(c))
            hasLower = 1;
        else if (iswdigit(c))
            hasDigit = 1;
        else if (iswpunct(c))
            hasPunct = 1;
        else
            throw cdr::Exception("Invalid character in password.  "
                    "Please only use letters, numbers, and punctuation.");

        // Test for some illegal characters
        if (c == '\'' || c == '"' || c == ' ')
            throw cdr::Exception(
                    "Sorry, quote marks may not be used in CDR passwords");
        if (c == ' ')
            throw cdr::Exception(
                    "Sorry, spaces may not be used in CDR passwords");
        if (c > 127)
            throw cdr::Exception(
                    "Sorry, Unicode chars may not be used in CDR passwords");
        if (c < 32)
            // Check above should prevent this but - belt and suspenders
            throw cdr::Exception("Control chars may not be used in passwords");

        ++p;
    }

    int sumTypes = hasUpper + hasLower + hasDigit + hasPunct;
    if (sumTypes < 3)
        throw cdr::Exception("Passwords must have 3 of the 4 categories of "
                "upper case, lower case, numbers, or punctuation.");

    return;
}

void cdr::Session::setUserPw (
    cdr::db::Connection& conn,
    const cdr::String&   userName,
    const cdr::String&   password,
    const bool           newUser
) {
    std::string qry;            // Update query

    // If the session's user isn't the owner of the pw, is he
    //  allowed to change the owner's password?
    if (uName != userName) {
        if ((newUser && !canDo(conn, L"CREATE USER", L"")) ||
                        !canDo(conn, L"MODIFY USER", L""))
            throw cdr::Exception(
                L"User is not authorized to change other user's password");
    }

    // Check that the password meets security requirements, exception if not
    // Passwords that have passed checks are SQL injection safe
    testPasswordString(password);

    // Can we hash the password?
    if (dbHasHash(conn, s_hashAlgo)) {
        // Use the std::string rather than wide chars to get straightforward
        //  hashing - same going in, going out, and if run in stored proc
        std::ostringstream os;
        os << "UPDATE usr"
           << " SET hashedpw = HASHBYTES('"
           << s_hashAlgo
           << "', '"
           << password.toUtf8()
           << "')"
           << " WHERE name = ?";
        qry = os.str();

        cdr::db::PreparedStatement stmt = conn.prepareStatement(qry);
        stmt.setString(1, userName);
        stmt.executeQuery();
        stmt.close();
    }
    // XXX Do this unconditionally until testing is complete
    // else
    {
        qry =
            "UPDATE usr "
            "   SET password = ?"
            " WHERE name = ?";
        cdr::db::PreparedStatement stmt = conn.prepareStatement(qry);
        stmt.setString(1, password);
        stmt.setString(2, userName);
        stmt.executeQuery();
        stmt.close();
    }
}


/**
 * Does the DBMS support password hashing of a specific type.
 *
 * Subroutine of chkIdPassword().
 *
 *  @param algo     Check this algorithm, e.g., 'SHA1'
 *  @param conn     Database connection
 *
 *  @return         True = Yes it does.
 */
static bool dbHasHash(
    cdr::db::Connection& conn,
    const std::string&   algo
) {
    static bool tested = false;
    static bool testresult;

    // Short circuit
    if (tested)
        return testresult;

    cdr::String hashed;

    // Query to select hashbytes from passed algorithm name
    // Can "foo" be hashed by SQL Server using the passed algorithm?
    std::string qry = "SELECT HASHBYTES('" + algo + "', 'foo')";

    // Assume it works until proven otherwise
    testresult = true;

    // Test
    try {
        cdr::db::PreparedStatement ps = conn.prepareStatement(qry);
        cdr::db::ResultSet rs = ps.executeQuery();
        hashed = rs.getString(1);
        ps.close();
        if (hashed.isNull() || hashed == L"")
            // Function is supported but algorithm is not
            testresult = false;
    }
    catch (...) {
        // HASHBYTES function is unsupported
        testresult = false;
    }

    // Don't have to try again
    tested = true;

    return testresult;
}


int cdr::getLoginFailedCount(
    cdr::db::Connection& conn,
    const cdr::String&   userName
) {
    int count;

    // Get the count from the user record
    cdr::db::PreparedStatement ps = conn.prepareStatement(
            "SELECT login_failed FROM usr WHERE name = ?");
    ps.setString(1, userName);
    cdr::db::ResultSet rs = ps.executeQuery();
    if (rs.next())
        count = rs.getInt(1);
    else
        count = -1;
    ps.close();

    return count;
}


int cdr::setLoginFailedCount(
    cdr::db::Connection& conn,
    const cdr::String&   userName,
    int                  counter
) {
    if (counter != 1 && counter != 0)
        // This could only be a bug
        throw cdr::Exception(
            L"Invalid counter=" + cdr::String::toString(counter) +
            L" passed to setLoginFailedCount - please inform support staff.");

    // Get current count
    int oldCount = getLoginFailedCount(conn, userName);
    if (oldCount == -1)
        // User not found
        return oldCount;

    // Generate the new count
    int newCount = 0;
    if (counter != 0)
        newCount = oldCount + 1;

    // Store it
    cdr::db::PreparedStatement ps = conn.prepareStatement(
        "UPDATE usr SET login_failed = ? WHERE name = ?");
    ps.setInt(1, newCount);
    ps.setString(2, userName);
    ps.executeQuery();
    ps.close();

    return newCount;
}


bool cdr::Session::canDo(db::Connection& conn,
                         const cdr::String& action,
                         int docId) const
{
    // In case the action string includes newlines, etc.
    // This can happen if it was stored in an XML doc via XMetal
    cdr::String normAction = cdr::normalizeWhiteSpace (action);

    std::string query = "SELECT COUNT(*)"
                        "  FROM grp_usr    gu,"
                        "       grp_action ga,"
                        "       action     a,"
                        "       document   d"
                        " WHERE ga.action   = a.id"
                        "   AND ga.doc_type = d.doc_type"
                        "   AND ga.grp      = gu.grp"
                        "   AND gu.usr      = ?"
                        "   AND a.name      = ?"
                        "   AND d.id        = ?";
    cdr::db::PreparedStatement ps = conn.prepareStatement(query);
    ps.setInt(1, uid);
    ps.setString(2, normAction);
    ps.setInt(3, docId);
    cdr::db::ResultSet rs = ps.executeQuery();
    if (!rs.next())
        // Shouldn't ever reach here.
        throw cdr::Exception(L"Database failure extracting result count");
    int count = rs.getInt(1);
    return count > 0;
}


/**
 * Processes CdrCanDo command, reporting whether the current session/user
 * is authorized to perform an action.
 *
 * Expected command format is:
 *   <CdrCanDo>
 *     <Action>Name_of_action</Action>
 *     <DocType>Name_of_document_type</DocType>
 *   </CdrCanDo>
 *
 * DocType is optional.  If not supplied, the default is L"", which should
 * work for actions that are not doc type specific.
 */

cdr::String cdr::getCanDo (cdr::Session& session,
                           const cdr::dom::Node& node,
                           cdr::db::Connection& conn)
{
    cdr::dom::Node child;       // Child node in command
    cdr::String cmdAction=L"",  // Action to be checked from command element
                cmdDocType=L"", // DocType to be checked
                result;         // Y=Action is allowed, N=not

    // Parse command
    child = node.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();

            if (name == L"Action")
                cmdAction = cdr::dom::getTextContent (child);
            else if (name == L"DocType")
                cmdDocType = cdr::dom::getTextContent (child);
        }
        child = child.getNextSibling();
    }

    // Doc type is optional, but there has to be an action to check
    if (cmdAction == L"")
        throw cdr::Exception (L"getCanDo: No action specified to check");

    // Check
    result = (session.canDo (conn, cmdAction, cmdDocType)) ? L"Y" : L"N";

    // Compose reply
    return cdr::String(L"   <CdrCanDoResp>" + result + L"</CdrCanDoResp>");
}
