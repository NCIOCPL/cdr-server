/*
 * $Id: CdrSession.cpp,v 1.1 2000-04-16 21:38:32 bkline Exp $
 *
 * Session control information.
 *
 * $Log: not supported by cvs2svn $
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
                                                "  FROM session s,"
                                                "       usr     u"
                                                " WHERE s.name  = ?"
                                                "   AND s.usr   = u.id"
                                                "   AND s.ended IS NULL");
    if (!rs.next())
        throw cdr::Exception("LInvalid or expired session", sessionId);
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
