
/*
 * $Id$
 *
 * Deletes a user (and any of the user's group memberships) from the CDR.  
 * Fails if any actions have been performed by the user.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2000/05/03 15:25:41  bkline
 * Fixed database statement creation.
 *
 * Revision 1.2  2000/04/23 01:19:58  bkline
 * Added function-level comment header.
 *
 * Revision 1.1  2000/04/22 09:27:18  bkline
 * Initial revision
 */

#include "CdrCommand.h"
#include "CdrDbResultSet.h"
#include "CdrDbPreparedStatement.h"

/**
 * Drops any rows from the grp_usr table associated with this user, then drops
 * the row from the usr table.
 */
cdr::String cdr::delUsr(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& conn) 
{
    // Make sure our user is authorized to delete CDR users.
    if (!session.canDo(conn, L"DELETE USER", L""))
        throw cdr::Exception(
                L"DELETE USER action not authorized for this user");

    // Extract the user name from the command.
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
    std::string query = "SELECT id FROM usr WHERE name = ?";
    cdr::db::PreparedStatement usrQuery = conn.prepareStatement(query);
    usrQuery.setString(1, usrName);
    cdr::db::ResultSet usrRs = usrQuery.executeQuery();
    if (!usrRs.next())
        throw cdr::Exception(L"User not found", usrName);
    int usrId = usrRs.getInt(1);
    usrQuery.close();

    // Mark the user as inactive.
    query = "UPDATE usr SET expired = GETDATE() WHERE id = ?";
    cdr::db::PreparedStatement expire = conn.prepareStatement(query);
    expire.setInt(1, usrId);
    expire.executeQuery();
    return L"  <CdrDelUsrResp/>\n";
}
