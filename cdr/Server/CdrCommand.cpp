/*
 * $Id: CdrCommand.cpp,v 1.42 2008-12-19 17:03:46 bkline Exp $
 *
 * Lookup facility for CDR commands.  Also contains stubs right now.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.41  2004/08/20 19:58:55  bkline
 * Added new CdrSetDocStatus command.
 *
 * Revision 1.40  2004/08/11 17:48:15  bkline
 * Adding new command CdrAddExternalMapping.
 *
 * Revision 1.39  2004/07/08 00:32:38  bkline
 * Added CdrGetGlossaryMap command; added cdr.lib to 'make clean' target.
 *
 * Revision 1.38  2004/05/14 02:07:11  ameyer
 * Added CdrCacheing transaction pointing to cdr::cacheInit().
 *
 * Revision 1.37  2003/02/10 14:04:20  bkline
 * Added new command CdrMailerCleanup.
 *
 * Revision 1.36  2003/01/28 23:46:26  ameyer
 * Added 4 XML transactions, all mapping to sysValue() command.
 *
 * Revision 1.35  2002/11/14 13:23:58  bkline
 * Changed CdrFilter command to use filter sets.  Added CdrDelFilterSet
 * command.
 *
 * Revision 1.34  2002/11/12 11:44:37  bkline
 * Added filter set support.
 *
 * Revision 1.33  2002/06/26 02:22:14  ameyer
 * Added CdrLastVersions => lastVersion().
 *
 * Revision 1.32  2002/06/18 20:33:39  ameyer
 * Added CdrCanDo command.
 *
 * Revision 1.31  2002/05/03 20:35:45  bkline
 * New CdrListVersions command added.
 *
 * Revision 1.30  2002/04/04 01:05:21  bkline
 * Added cdr::publish().
 *
 * Revision 1.29  2001/10/17 13:50:42  bkline
 * Added CdrMergeProt command.
 *
 * Revision 1.28  2001/09/19 18:41:37  bkline
 * Added CdrPasteLink.
 *
 * Revision 1.27  2001/06/28 17:37:45  bkline
 * Added command CdrGetCssFiles.
 *
 * Revision 1.26  2001/05/21 20:30:04  bkline
 * Added commands for query term definition support.
 *
 * Revision 1.25  2001/05/16 15:44:10  bkline
 * Added CdrListSchemaDocs command.
 *
 * Revision 1.24  2001/05/14 18:07:46  bkline
 * Added CdrGetLinkType.
 *
 * Revision 1.23  2001/04/24 23:42:40  ameyer
 * Added link administration transaction processing.
 *
 * Revision 1.21  2001/04/13 12:21:40  bkline
 * Added CdrListActions, CdrGetAction, CdrAddAction, CdrRepAction, and
 * CdrDelAction.  Removed dead code for obsolete stubs.
 *
 * Revision 1.20  2001/04/08 22:45:01  bkline
 * Added CdrGetTree.
 *
 * Revision 1.19  2001/04/05 19:26:43  ameyer
 * Added CommandMap for CdrReindexDoc.
 *
 * Revision 1.18  2001/02/26 16:09:53  mruben
 * expanded information saved for version
 *
 * Revision 1.17  2001/01/17 21:49:18  bkline
 * Replaced CdrGetSchema with CdrAddDocType, CdrModDocType, CdrDelDocType,
 * and CdrGetDocType.
 *
 * Revision 1.16  2000/12/28 13:29:26  bkline
 * Added entry for CdrGetSchema command.
 *
 * Revision 1.15  2000/10/27 02:31:55  ameyer
 * ifdef'd out the delDoc stub.
 *
 * Revision 1.14  2000/10/24 13:13:16  mruben
 * added support for CdrReport
 *
 * Revision 1.13  2000/10/23 14:08:06  mruben
 * added version control commands
 *
 * Revision 1.12  2000/10/04 18:24:20  bkline
 * Added CdrSearchLinks and CdrListDocTypes to command map.
 *
 * Revision 1.11  2000/05/23 15:55:55  mruben
 * removed stub for CdrGetDoc
 *
 * Revision 1.10  2000/05/21 00:48:23  bkline
 * Added CdrShutdown command.
 *
 * Revision 1.9  2000/05/17 13:01:59  bkline
 * Added code to addDoc stub to verify that document is well-formed, including
 * that in the CDATA section.
 * Replaced addDoc and repDoc stubs with new code from Alan.
 *
 * Revision 1.8  2000/05/09 19:27:45  mruben
 * modified for CdrFilter
 *
 * Revision 1.7  2000/04/26 01:26:14  bkline
 * Replaced stub for cdr::validateDoc().
 *
 * Revision 1.6  2000/04/22 09:24:26  bkline
 * Added user and group commands.
 *
 * Revision 1.5  2000/04/21 13:57:11  bkline
 * Replaced stub for cdr::search().
 *
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

#include <iostream>
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
        CommandMap(L"CdrLogon",              cdr::logon),
        CommandMap(L"CdrLogoff",             cdr::logoff),
        CommandMap(L"CdrCheckAuth",          cdr::checkAuth),
        CommandMap(L"CdrAddGrp",             cdr::addGrp),
        CommandMap(L"CdrGetGrp",             cdr::getGrp),
        CommandMap(L"CdrModGrp",             cdr::modGrp),
        CommandMap(L"CdrDelGrp",             cdr::delGrp),
        CommandMap(L"CdrListGrps",           cdr::listGrps),
        CommandMap(L"CdrAddUsr",             cdr::addUsr),
        CommandMap(L"CdrGetUsr",             cdr::getUsr),
        CommandMap(L"CdrModUsr",             cdr::modUsr),
        CommandMap(L"CdrDelUsr",             cdr::delUsr),
        CommandMap(L"CdrListUsrs",           cdr::listUsrs),
        CommandMap(L"CdrListActions",        cdr::listActions),
        CommandMap(L"CdrGetAction",          cdr::getAction),
        CommandMap(L"CdrAddAction",          cdr::addAction),
        CommandMap(L"CdrRepAction",          cdr::repAction),
        CommandMap(L"CdrDelAction",          cdr::delAction),
        CommandMap(L"CdrAddSysValue",        cdr::sysValue),
        CommandMap(L"CdrRepSysValue",        cdr::sysValue),
        CommandMap(L"CdrDelSysValue",        cdr::sysValue),
        CommandMap(L"CdrGetSysValue",        cdr::sysValue),
        CommandMap(L"CdrCanDo",              cdr::getCanDo),
        CommandMap(L"CdrAddDoc",             cdr::addDoc),
        CommandMap(L"CdrRepDoc",             cdr::repDoc),
        CommandMap(L"CdrDelDoc",             cdr::delDoc),
        CommandMap(L"CdrValidateDoc",        cdr::validateDoc),
        CommandMap(L"CdrReindexDoc",         cdr::reIndexDoc),
        CommandMap(L"CdrSearch",             cdr::search),
        CommandMap(L"CdrListQueryTermRules", cdr::listQueryTermRules),
        CommandMap(L"CdrListQueryTermDefs",  cdr::listQueryTermDefs),
        CommandMap(L"CdrAddQueryTermDef",    cdr::addQueryTermDef),
        CommandMap(L"CdrDelQueryTermDef",    cdr::delQueryTermDef),
        CommandMap(L"CdrSearchLinks",        cdr::searchLinks),
        CommandMap(L"CdrGetDoc",             cdr::getDoc),
        CommandMap(L"CdrCheckOut",           cdr::checkVerOut),
        CommandMap(L"CdrCheckIn",            cdr::checkVerIn),
        CommandMap(L"CdrListVersions",       cdr::listVersions),
        CommandMap(L"CdrLastVersions",       cdr::lastVersions),
        CommandMap(L"CdrCreateLabel",        cdr::createLabel),
        CommandMap(L"CdrDeleteLabel",        cdr::deleteLabel),
        CommandMap(L"CdrLabelDocument",      cdr::labelDocument),
        CommandMap(L"CdrUnlabelDocument",    cdr::unlabelDocument),
        CommandMap(L"CdrReport",             cdr::report),
        CommandMap(L"CdrFilter",             cdr::filter),
        CommandMap(L"CdrGetFilters",         cdr::getFilters),
        CommandMap(L"CdrGetFilterSets",      cdr::getFilterSets),
        CommandMap(L"CdrGetFilterSet",       cdr::getFilterSet),
        CommandMap(L"CdrAddFilterSet",       cdr::addFilterSet),
        CommandMap(L"CdrRepFilterSet",       cdr::repFilterSet),
        CommandMap(L"CdrDelFilterSet",       cdr::delFilterSet),
        CommandMap(L"CdrGetLinks",           cdr::getLinks),
        CommandMap(L"CdrGetLinkType",        cdr::getLinkType),
        CommandMap(L"CdrAddLinkType",        cdr::putLinkType),
        CommandMap(L"CdrModLinkType",        cdr::putLinkType),
        CommandMap(L"CdrListLinkTypes",      cdr::listLinkTypes),
        CommandMap(L"CdrListLinkProps",      cdr::listLinkProps),
        CommandMap(L"CdrPasteLink",          cdr::pasteLink),
        CommandMap(L"CdrListDocTypes",       cdr::listDocTypes),
        CommandMap(L"CdrListSchemaDocs",     cdr::listSchemaDocs),
        CommandMap(L"CdrGetCssFiles",        cdr::getCssFiles),
        CommandMap(L"CdrAddDocType",         cdr::addDocType),
        CommandMap(L"CdrModDocType",         cdr::modDocType),
        CommandMap(L"CdrDelDocType",         cdr::delDocType),
        CommandMap(L"CdrGetDocType",         cdr::getDocType),
        CommandMap(L"CdrGetTree",            cdr::getTree),
        CommandMap(L"CdrMergeProt",          cdr::mergeProt),
        CommandMap(L"CdrMailerCleanup",      cdr::mailerCleanup),
        CommandMap(L"CdrPublish",            cdr::publish),
        CommandMap(L"CdrCacheing",           cdr::cacheInit),
        CommandMap(L"CdrGetGlossaryMap",     cdr::getGlossaryMap),
        CommandMap(L"CdrAddExternalMapping", cdr::addExternalMapping),
        CommandMap(L"CdrSetDocStatus",       cdr::setDocStatus),
        CommandMap(L"CdrLogClientEvent",     cdr::logClientEvent),
        CommandMap(L"CdrShutdown",           cdr::shutdown)
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
