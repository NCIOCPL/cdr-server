
/*
 * $Id$
 *
 * Modifies the attributes and group assignments for an existing CDR user.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2000/05/03 15:25:41  bkline
 * Fixed database statement creation.
 *
 * Revision 1.2  2000/04/23 01:25:07  bkline
 * Added function-level comment header.
 *
 * Revision 1.1  2000/04/22 09:24:57  bkline
 * Initial revision
 */

#include "CdrCommand.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"

/**
 * Updates the appropriate row in the usr table, then drops all rows in the
 * grp_usr table joined to that row, and replaces them with a fresh set from
 * the command.
 */
cdr::String cdr::modUsr(cdr::Session& session,
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& conn)
{
    // Make sure our user is authorized to add users.
    if (!session.canDo(conn, L"MODIFY USER", L""))
        throw
            cdr::Exception(L"MODIFY USER action not authorized for this user");

    // Extract the data elements from the command node.
    cdr::StringList grpList;
    cdr::String uName, password, fullName(true), office(true), email(true),
                phone(true), comment(true), newName(true);
    cdr::dom::Node child = commandNode.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"UserName")
                uName = cdr::dom::getTextContent(child);
            else if (name == L"Password")
                password = cdr::dom::getTextContent(child);
            else if (name == L"FullName")
                fullName = cdr::dom::getTextContent(child);
            else if (name == L"Office")
                office = cdr::dom::getTextContent(child);
            else if (name == L"Email")
                email = cdr::dom::getTextContent(child);
            else if (name == L"Phone")
                phone = cdr::dom::getTextContent(child);
            else if (name == L"Comment")
                comment = cdr::dom::getTextContent(child);
            else if (name == L"GrpName")
                grpList.push_back(cdr::dom::getTextContent(child));
            else if (name == L"NewName")
                newName = cdr::dom::getTextContent(child);
        }
        child = child.getNextSibling();
    }
    if (uName.size() == 0)
        throw cdr::Exception(L"Missing user name");
    // if (password.isNull())
    //     throw cdr::Exception(L"Missing password");

    // Look up the user.
    std::string query = "SELECT id FROM usr WHERE name = ?";
    cdr::db::PreparedStatement usrQuery = conn.prepareStatement(query);
    usrQuery.setString(1, uName);
    cdr::db::ResultSet usrRs = usrQuery.executeQuery();
    if (!usrRs.next())
        throw cdr::Exception(L"Failure locating user information");
    int usrId = usrRs.getInt(1);

    // Update the row for the user
    if (newName.size() > 0)
        uName = newName;
    conn.setAutoCommit(false);
    query = "UPDATE usr"
            "   SET name     = ?,"
            "       fullname = ?,"
            "       office   = ?,"
            "       email    = ?,"
            "       phone    = ?,"
            "       comment  = ?"
            " WHERE id       = ?";
    cdr::db::PreparedStatement update = conn.prepareStatement(query);
    update.setString(1, uName);
    update.setString(2, fullName);
    update.setString(3, office);
    update.setString(4, email);
    update.setString(5, phone);
    update.setString(6, comment);
    update.setInt   (7, usrId);
    update.executeQuery();

    // If password is null, leave it alone, else replace it and clear failures
    if (!password.isNull() && password != L"") {
        session.setUserPw(conn, uName, password, false);
        cdr::setLoginFailedCount(conn, uName, 0);
    }

    // Clear out existing group assignments.
    query = "DELETE grp_usr WHERE usr = ?";
    cdr::db::PreparedStatement del = conn.prepareStatement(query);
    del.setInt(1, usrId);
    del.executeQuery();

    // Add groups to which user is assigned, if any.
    if (grpList.size() > 0) {
        StringList::const_iterator i = grpList.begin();
        while (i != grpList.end()) {
            const cdr::String gName = *i++;

            // Convert group name to unique ID
            // Do this separately from the insert so we can make error check
            query = "SELECT id FROM grp WHERE name = ?";
            cdr::db::PreparedStatement grpQuery =
                conn.prepareStatement(query);
            grpQuery.setString(1, gName);
            cdr::db::ResultSet rs =
                grpQuery.executeQuery();
            if (!rs.next())
                throw cdr::Exception(L"Unknown group", gName);
            int grpId = rs.getInt(1);
            grpQuery.close();

            // Link group to user
            query = "INSERT INTO grp_usr(grp, usr) VALUES(?, ?)";
            cdr::db::PreparedStatement insert = conn.prepareStatement(query);
            insert.setInt(1, grpId);
            insert.setInt(2, usrId);
            insert.executeQuery();
            insert.close();
        }
    }

    // Report success.
    conn.commit();
    return L"  <CdrModUsrResp/>\n";
}
