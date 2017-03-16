/*
 * Lookup facility for CDR commands.  Also contains stubs right now.
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
        CommandMap(L"CdrDupSession",         cdr::dupSession),
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
        CommandMap(L"CdrCanDo",              cdr::getCanDo),
        CommandMap(L"CdrAddDoc",             cdr::addDoc),
        CommandMap(L"CdrRepDoc",             cdr::repDoc),
        CommandMap(L"CdrDelDoc",             cdr::delDoc),
        CommandMap(L"CdrValidateDoc",        cdr::validateDoc),
        CommandMap(L"CdrReindexDoc",         cdr::reIndexDoc),
        CommandMap(L"CdrUpdateTitle",        cdr::updateTitle),
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
        CommandMap(L"CdrMailerCleanup",      cdr::mailerCleanup),
        CommandMap(L"CdrPublish",            cdr::publish),
        CommandMap(L"CdrCacheing",           cdr::cacheInit),
        CommandMap(L"CdrGetGlossaryMap",     cdr::getGlossaryMap),
        CommandMap(L"CdrGetSpanishGlossaryMap", cdr::getSpanishGlossaryMap),
        CommandMap(L"CdrAddExternalMapping", cdr::addExternalMapping),
        CommandMap(L"CdrSetDocStatus",       cdr::setDocStatus),
        CommandMap(L"CdrSetCtl",             cdr::setCtlValues),
        CommandMap(L"CdrLogClientEvent",     cdr::logClientEvent),
        CommandMap(L"CdrSaveClientTraceLog", cdr::saveClientTraceLog),
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
