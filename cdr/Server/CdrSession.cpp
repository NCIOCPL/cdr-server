/*
 * $Id: CdrSession.cpp,v 1.5 2000-05-03 15:25:41 bkline Exp $
 *
 * Session control information.
 *
 * $Log: not supported by cvs2svn $
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

#include "CdrSession.h"
#include "CdrDbConnection.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"
#include "CdrException.h"

void cdr::Session::lookupSession(const cdr::String& sessionId,
                                 cdr::db::Connection& conn)
{
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
        throw cdr::Exception(L"Invalid or expired session", sessionId);
    name  = sessionId;
    id    = rs.getInt(1);
    uid   = rs.getInt(2);
    uName = rs.getString(3);
}

void cdr::Session::setLastActivity(db::Connection& conn) const
{
    std::string query = "UPDATE session SET last_act = GETDATE() WHERE id = ?";
    cdr::db::PreparedStatement update = conn.prepareStatement(query);
    update.setInt(1, id);
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
