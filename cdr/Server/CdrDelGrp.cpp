
/*
 * $Id: CdrDelGrp.cpp,v 1.1 2000-04-22 09:27:50 bkline Exp $
 *
 * Deletes a group (and its associated users and actions) from CDR.
 *
 * $Log: not supported by cvs2svn $
 */

#include "CdrCommand.h"
#include "CdrDbResultSet.h"

cdr::String cdr::delGrp(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection) 
{
    // Make sure our user is authorized to delete CDR groups.
    if (!session.canDo(dbConnection, L"DELETE GROUP", L""))
        throw cdr::Exception(
                L"DELETE GROUP action not authorized for this user");

    // Extract the group name from the command.
    cdr::String grpName;
    cdr::dom::Node child = commandNode.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"GrpName")
                grpName = cdr::dom::getTextContent(child);
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
        throw cdr::Exception(L"Group not found", grpName);
    int gid = grpRs.getInt(1);
    grpQuery.close();

    // Drop group members.
    dbConnection.setAutoCommit(false);
    cdr::db::Statement dropUsrs(dbConnection);
    dropUsrs.setInt(1, gid);
    dropUsrs.executeQuery("DELETE grp_usr WHERE grp_usr.grp = ?");
    //dropUsrs.close();

    // Drop group actions.
    cdr::db::Statement dropActions(dbConnection);
    dropActions.setInt(1, gid);
    dropActions.executeQuery("DELETE grp_action WHERE grp_action.grp = ?");
    //dropActions.close();

    // Finish the job.
    cdr::db::Statement dropGrp(dbConnection);
    dropGrp.setInt(1, gid);
    dropGrp.executeQuery("DELETE grp WHERE id = ?");
    dbConnection.commit();
    return L"  <CdrDelGrpResp/>\n";
}
