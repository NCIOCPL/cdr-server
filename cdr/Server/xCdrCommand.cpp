/*
 * $Id: xCdrCommand.cpp,v 1.5 2000-04-21 13:57:11 bkline Exp $
 *
 * Lookup facility for CDR commands.  Also contains stubs right now.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.4  2000/04/17 21:25:28  bkline
 * Replaced stub for cdr::checkAuth() function.
 *
 * Revision 1.3  2000/04/16 21:42:25  bkline
 * Replaced stub for cdr::logoff().
 *
 * Revision 1.2  2000/04/15 14:08:44  bkline
 * Changed cdr::DbConnection* to cdr::db::Connection&.  Replaced stub
 * for cdr::logon with real implementation.
 *
 * Revision 1.1  2000/04/13 17:08:10  bkline
 * Initial revision
 */

#include "CdrCommand.h"

/**
 * Finds the function which implements the specified command.
 */
cdr::Command cdr::lookupCommand(const cdr::String& name)
{
    struct CommandMap { 
        CommandMap(const cdr::String& n, cdr::Command c) : name(n), cmd(c) {}
        cdr::String name; 
        cdr::Command cmd; 
    };
    static CommandMap map[] = {
        CommandMap(L"CdrLogon",         cdr::logon),
        CommandMap(L"CdrLogoff",        cdr::logoff),
        CommandMap(L"CdrCheckAuth",     cdr::checkAuth),
        CommandMap(L"CdrAddGrp",        cdr::addGrp),
        CommandMap(L"CdrGetGrp",        cdr::getGrp),
        CommandMap(L"CdrModGrp",        cdr::modGrp),
        CommandMap(L"CdrDelGrp",        cdr::delGrp),
        CommandMap(L"CdrListGrps",      cdr::listGrps),
        CommandMap(L"CdrAddUsr",        cdr::addUsr),
        CommandMap(L"CdrGetUsr",        cdr::getUsr),
        CommandMap(L"CdrModUsr",        cdr::modUsr),
        CommandMap(L"CdrDelUsr",        cdr::delUsr),
        CommandMap(L"CdrListUsrs",      cdr::listUsrs),
        CommandMap(L"CdrAddDoc",        cdr::addDoc),
        CommandMap(L"CdrRepDoc",        cdr::repDoc),
        CommandMap(L"CdrDelDoc",        cdr::delDoc),
        CommandMap(L"CdrValidateDoc",   cdr::validateDoc),
        CommandMap(L"CdrSearch",        cdr::search),
        CommandMap(L"CdrGetDoc",        cdr::getDoc),
        CommandMap(L"CdrCheckOut",      cdr::checkOut),
        CommandMap(L"CdrCheckIn",       cdr::checkIn),
        CommandMap(L"CdrReport",        cdr::report),
        CommandMap(L"CdrFilter",        cdr::filter),
        CommandMap(L"CdrGetLinks",      cdr::getLinks)
    };
    for (size_t i = 0; i < sizeof map / sizeof *map; ++i) {
        if (name == map[i].name)
            return map[i].cmd;
    }
    return 0;
}

/**
 * Temporary function to pump out a stub response element for a 
 * CDR command which has not yet been implemented.
 */
static cdr::String stub(const cdr::String name)
{
    return L"  <" + name + L"Resp>\n   <Stub>Got your Cdr" + name +
           L" command ... can't process it yet.</Stub>\n  </" 
           + name + L"Resp>\n";
}

// Stubs.  Replace these as they are implemented.
#if 0
cdr::String cdr::logon(cdr::Session& session, 
                       const cdr::dom::Node& commandNode,
                       cdr::db::Connection& dbConnection) {
    return stub(L"Logon");
}

cdr::String cdr::logoff(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection) {
    return stub(L"Logoff");
}

cdr::String cdr::checkAuth(cdr::Session& session, 
                           const cdr::dom::Node& commandNode,
                           cdr::db::Connection& dbConnection) {
    return stub(L"CheckAuth");
}
#endif

cdr::String cdr::addGrp(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection) {
    return stub(L"AddGrp");
}

cdr::String cdr::getGrp(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection) {
    return stub(L"GetGrp");
}

cdr::String cdr::modGrp(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection) {
    return stub(L"ModGrp");
}

cdr::String cdr::delGrp(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection) {
    return stub(L"DelGrp");
}

cdr::String cdr::listGrps(cdr::Session& session, 
                          const cdr::dom::Node& commandNode,
                          cdr::db::Connection& dbConnection) {
    return stub(L"ListGrps");
}

cdr::String cdr::addUsr(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection) {
    return stub(L"AddUsr");
}

cdr::String cdr::getUsr(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection) {
    return stub(L"GetUsr");
}

cdr::String cdr::modUsr(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection) {
    return stub(L"ModUsr");
}

cdr::String cdr::delUsr(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection) {
    return stub(L"DelUsr");
}

cdr::String cdr::listUsrs(cdr::Session& session, 
                          const cdr::dom::Node& commandNode,
                          cdr::db::Connection& dbConnection) {
    return stub(L"ListUsrs");
}

cdr::String cdr::addDoc(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection) {
    return stub(L"AddDoc");
}

cdr::String cdr::repDoc(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection) {
    return stub(L"RepDoc");
}

cdr::String cdr::delDoc(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection) {
    return stub(L"DelDoc");
}

cdr::String cdr::validateDoc(cdr::Session& session, 
                             const cdr::dom::Node& commandNode,
                             cdr::db::Connection& dbConnection) {
    return stub(L"ValidateDoc");
}

#if 0
cdr::String cdr::search(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection) {
    return stub(L"Search");
}
#endif

cdr::String cdr::getDoc(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection) {
    return stub(L"GetDoc");
}

cdr::String cdr::checkOut(cdr::Session& session, 
                          const cdr::dom::Node& commandNode,
                          cdr::db::Connection& dbConnection) {
    return stub(L"CheckOut");
}

cdr::String cdr::checkIn(cdr::Session& session, 
                         const cdr::dom::Node& commandNode,
                         cdr::db::Connection& dbConnection) {
    return stub(L"CheckIn");
}

cdr::String cdr::report(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection) {
    return stub(L"Report");
}

cdr::String cdr::filter(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection) {
    return stub(L"Filter");
}

cdr::String cdr::getLinks(cdr::Session& session, 
                          const cdr::dom::Node& commandNode,
                          cdr::db::Connection& dbConnection) {
    return stub(L"GetLinks");
}
