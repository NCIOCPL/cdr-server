
/*
 * $Id: CdrModGrp.cpp,v 1.1 2000-04-22 09:31:32 bkline Exp $
 *
 * Modifies authorizations, comment, and possibly name of existing group.
 *
 * $Log: not supported by cvs2svn $
 */

#include <list>
#include "CdrCommand.h"
#include "CdrDbResultSet.h"

struct Auth { cdr::String action, docType; };
typedef std::list<Auth> AuthList;

cdr::String cdr::modGrp(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection) 
{
    // Make sure our user is authorized to MODIFY groups.
    if (!session.canDo(dbConnection, L"MODIFY GROUP", L""))
        throw 
            cdr::Exception(L"MODIFY GROUP action not authorized for this user");

    // Extract the data elements from the command node.
    AuthList authList;
    cdr::String grpName, newGrpName, comment(true);
    cdr::dom::Node child = commandNode.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"GrpName")
                grpName = cdr::dom::getTextContent(child);
            else if (name == L"Comment")
                comment = cdr::dom::getTextContent(child);
            else if (name == L"NewGrpName")
                newGrpName = cdr::dom::getTextContent(child);
            else if (name == L"Auth") {
                Auth auth;
                cdr::dom::Node authChild = child.getFirstChild();
                while (authChild != 0) {
                    cdr::String authName = authChild.getNodeName();
                    if (authName == L"Action")
                        auth.action = cdr::dom::getTextContent(authChild);
                    else if (authName == L"DocType")
                        auth.docType = cdr::dom::getTextContent(authChild);
                    authChild = authChild.getNextSibling();
                }
                if (auth.action.size() == 0)
                    throw cdr::Exception(L"Missing Action element in Auth");
                authList.push_back(auth);
            }
        }
        child = child.getNextSibling();
    }
    if (grpName.size() == 0)
        throw cdr::Exception(L"Missing group name");

    // Make sure the group exists.
    cdr::db::Statement grpQuery(dbConnection);
    grpQuery.setString(1, grpName);
    cdr::db::ResultSet grpRs = grpQuery.executeQuery("SELECT id"
                                                     "  FROM grp"
                                                     " WHERE name = ?");
    if (!grpRs.next())
        throw cdr::Exception(L"Unknown group", grpName);
    int grpId = grpRs.getInt(1);

    // Update the values for the group row.
    dbConnection.setAutoCommit(false);
    cdr::db::Statement update(dbConnection);
    if (newGrpName.size() > 0) {
        update.setString(1, newGrpName);
        update.setString(2, comment);
        update.setInt(3, grpId);
        update.executeQuery("UPDATE grp"
                            "   SET name = ?,"
                            "       comment = ?"
                            " WHERE id = ?");
    }
    else {
        update.setString(1, comment);
        update.setInt(2, grpId);
        update.executeQuery("UPDATE grp"
                            "   SET comment = ?"
                            " WHERE id = ?");
    }

    // Clear out any existing authorizations.
    cdr::db::Statement del(dbConnection);
    del.setInt(1, grpId);
    del.executeQuery("DELETE grp_action"
                     " WHERE grp = ?");

    // Add authorizations back in, if any present.
    if (authList.size() > 0) {
        AuthList::const_iterator i = authList.begin();
        while (i != authList.end()) {
            const Auth& auth = *i++;

            // Do INSERT the hard way so we can determine success.
            int actionId, docTypeId;
            cdr::db::Statement actionQuery(dbConnection);
            actionQuery.setString(1, auth.action);
            cdr::db::ResultSet rs1 = 
                actionQuery.executeQuery("SELECT id"
                                         "  FROM action"
                                         " WHERE name = ?");
            if (!rs1.next())
                throw cdr::Exception(L"Unknown action", auth.action);
            actionId = rs1.getInt(1);
            cdr::db::Statement docTypeQuery(dbConnection);
            docTypeQuery.setString(1, auth.docType);
            cdr::db::ResultSet rs2 = 
                docTypeQuery.executeQuery("SELECT id"
                                          "  FROM doc_type"
                                          " WHERE name = ?");
            if (!rs2.next())
                throw cdr::Exception(L"Unknown doc type", auth.docType);
            docTypeId = rs2.getInt(1);
            cdr::db::Statement insert(dbConnection);
            insert.setInt(1, grpId);
            insert.setInt(2, actionId);
            insert.setInt(3, docTypeId);
            insert.executeQuery("INSERT INTO grp_action"
                                "("
                                "    grp,"
                                "    action,"
                                "    doc_type"
                                ")"
                                "VALUES(?, ?, ?)");
        }
    }

    // Report success.
    dbConnection.commit();
    return L"  <CdrModGrpResp/>\n";
}
