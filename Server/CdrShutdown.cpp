/*
 * $Id: CdrShutdown.cpp,v 1.1 2001-11-28 19:45:16 bkline Exp $
 *
 * Shuts down the CDR system.
 *
 * $Log: not supported by cvs2svn $
 */

#include "CdrCommand.h"
#include "CdrException.h"
/**
 * Makes sure the current user is authorized to shut the system down.
 * The command dispatcher will do the rest if we don't throw an exception.
 */
cdr::String cdr::shutdown(cdr::Session& session,
                          const cdr::dom::Node& commandNode,
                          cdr::db::Connection& dbConnection) {
    if (!session.canDo(dbConnection, L"SHUTDOWN", L""))
        throw cdr::Exception(L"SHUTDOWN action not authorized for this user");
    return L"  <CdrShutdownResp/>\n";
}
