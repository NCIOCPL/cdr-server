
/*
 * $Id: CdrDelUsr.cpp,v 1.1 2000-04-22 09:27:18 bkline Exp $
 *
 * Deletes a user (and any of the user's group memberships) from the CDR.  
 * Fails if any actions have been performed by the user.
 *
 * $Log: not supported by cvs2svn $
 */

#include "CdrCommand.h"
#include "CdrDbResultSet.h"

cdr::String cdr::delUsr(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection) 
{
    // Make sure our user is authorized to delete CDR users.
    if (!session.canDo(dbConnection, L"DELETE USER", L""))
        throw cdr::Exception(
                L"DELETE USER action not authorized for this user");

    // Extract the group name from the command.
    cdr::String usrName;
    cdr::dom::Node child = commandNode.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"UserName")
                usrName = cdr::dom::getTextContent(child);
        }
        child = child.getNextSibling();
    }
    if (usrName.size() == 0)
        throw cdr::Exception(L"Missing user name");

    // Make sure the user exists.
    cdr::db::Statement usrQuery(dbConnection);
    usrQuery.setString(1, usrName);
    cdr::db::ResultSet usrRs = usrQuery.executeQuery("SELECT id"
                                                     "  FROM usr"
                                                     " WHERE name = ?");
    if (!usrRs.next())
        throw cdr::Exception(L"User not found", usrName);
    int usrId = usrRs.getInt(1);
    usrQuery.close();

    // Drop group memberships.
    dbConnection.setAutoCommit(false);
    cdr::db::Statement dropGrps(dbConnection);
    dropGrps.setInt(1, usrId);
    dropGrps.executeQuery("DELETE grp_usr WHERE usr = ?");

    // Finish the job.
    cdr::db::Statement dropUsr(dbConnection);
    dropUsr.setInt(1, usrId);
    dropUsr.executeQuery("DELETE usr WHERE id = ?");
    dbConnection.commit();
    return L"  <CdrDelUsrResp/>\n";
}
