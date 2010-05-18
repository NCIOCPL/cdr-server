
/*
 * $Id$
 *
 * Deletes a group (and its associated users and actions) from CDR.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2000/04/22 09:27:50  bkline
 * Initial revision
 *
 */

#include "CdrCommand.h"
#include "CdrDbResultSet.h"
#include "CdrDbPreparedStatement.h"

cdr::String cdr::delGrp(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& conn) 
{
    // Make sure our user is authorized to delete CDR groups.
    if (!session.canDo(conn, L"DELETE GROUP", L""))
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
    std::string query = "SELECT id FROM grp WHERE name = ?";
    cdr::db::PreparedStatement grpQuery = conn.prepareStatement(query);
    grpQuery.setString(1, grpName);
    cdr::db::ResultSet grpRs = grpQuery.executeQuery();
    if (!grpRs.next())
        throw cdr::Exception(L"Group not found", grpName);
    int gid = grpRs.getInt(1);
    grpQuery.close();

    // Drop group members.
    conn.setAutoCommit(false);
    query = "DELETE grp_usr WHERE grp_usr.grp = ?";
    cdr::db::PreparedStatement dropUsrs = conn.prepareStatement(query);
    dropUsrs.setInt(1, gid);
    dropUsrs.executeQuery();

    // Drop group actions.
    query = "DELETE grp_action WHERE grp_action.grp = ?";
    cdr::db::PreparedStatement dropActions = conn.prepareStatement(query);
    dropActions.setInt(1, gid);
    dropActions.executeQuery();
    //dropActions.close();

    // Finish the job.
    query = "DELETE grp WHERE id = ?";
    cdr::db::PreparedStatement dropGrp = conn.prepareStatement(query);
    dropGrp.setInt(1, gid);
    dropGrp.executeQuery();
    conn.commit();
    return L"  <CdrDelGrpResp/>\n";
}
