/*
 * $Id: CdrSession.cpp,v 1.4 2000-04-22 09:30:22 bkline Exp $
 *
 * Session control information.
 *
 * $Log: not supported by cvs2svn $
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
#include "CdrDbResultSet.h"
#include "CdrException.h"

void cdr::Session::lookupSession(const cdr::String& sessionId,
                                 cdr::db::Connection& conn)
{
    cdr::db::Statement select(conn);
    select.setString(1, sessionId);
    cdr::db::ResultSet rs = select.executeQuery("SELECT s.id,"
                                                "       s.usr,"
                                                "       u.name"
                                                "  FROM session s"
                                                "  JOIN usr     u"
                                                "    ON s.usr   = u.id"
                                                " WHERE s.name  = ?"
                                                "   AND s.ended IS NULL");
    if (!rs.next())
        throw cdr::Exception(L"Invalid or expired session", sessionId);
    name  = sessionId;
    id    = rs.getInt(1);
    uid   = rs.getInt(2);
    uName = rs.getString(3);
}

void cdr::Session::setLastActivity(db::Connection& conn) const
{
    cdr::db::Statement update(conn);
    update.setInt(1, id);
    update.executeQuery("UPDATE session"
                        "   SET last_act = GETDATE()"
                        " WHERE id = ?");
}

bool cdr::Session::canDo(db::Connection& conn, 
                         const cdr::String& action,
                         const cdr::String& docType) const
{
    cdr::db::Statement query(conn);
    query.setInt(1, uid);
    query.setString(2, action);
    query.setString(3, docType);
    cdr::db::ResultSet rs = query.executeQuery("SELECT COUNT(*)"
                                               "  FROM grp_usr    gu,"
                                               "       grp_action ga,"
                                               "       action     a,"
                                               "       doc_type   t"
                                               " WHERE ga.action   = a.id"
                                               "   AND ga.doc_type = t.id"
                                               "   AND ga.grp      = gu.grp"
                                               "   AND gu.usr      = ?"
                                               "   AND a.name      = ?"
                                               "   AND t.name      = ?");
    if (!rs.next())
        // Shouldn't ever reach here.
        throw cdr::Exception(L"Database failure extracting result count");
    int count = rs.getInt(1);
    return count > 0;
}
