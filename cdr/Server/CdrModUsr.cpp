
/*
 * $Id: CdrModUsr.cpp,v 1.2 2000-04-23 01:25:07 bkline Exp $
 *
 * Modifies the attributes and group assignments for an existing CDR user.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2000/04/22 09:24:57  bkline
 * Initial revision
 */

#include "CdrCommand.h"
#include "CdrDbResultSet.h"

/**
 * Updates the appropriate row in the usr table, then drops all rows in the
 * grp_usr table joined to that row, and replaces them with a fresh set from
 * the command.
 */
cdr::String cdr::modUsr(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection) 
{
    // Make sure our user is authorized to add users.
    if (!session.canDo(dbConnection, L"MODIFY USER", L""))
        throw 
            cdr::Exception(L"MODIFY USER action not authorized for this user");

    // Extract the data elements from the command node.
    cdr::StringList grpList;
    cdr::String uName, password, fullName(true), office(true), email(true),
                phone(true), comment(true);
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
            else if (name == L"Comment")
                comment = cdr::dom::getTextContent(child);
        }
        child = child.getNextSibling();
    }
    if (uName.size() == 0)
        throw cdr::Exception(L"Missing user name");
    if (password.isNull())
        throw cdr::Exception(L"Missing password");

    // Look up the user.
    cdr::db::Statement usrQuery(dbConnection);
    usrQuery.setString(1, uName);
    cdr::db::ResultSet usrRs = usrQuery.executeQuery("SELECT id"
                                                     "  FROM usr"
                                                     " WHERE name = ?");
    if (!usrRs.next())
        throw cdr::Exception(L"Failure locating user information");
    int usrId = usrRs.getInt(1);

    // Update the row for the user
    dbConnection.setAutoCommit(false);
    cdr::db::Statement update(dbConnection);
    update.setString(1, uName);
    update.setString(2, password);
    update.setString(3, fullName);
    update.setString(4, office);
    update.setString(5, email);
    update.setString(6, phone);
    update.setString(7, comment);
    update.setInt   (8, usrId);
    update.executeQuery("UPDATE usr"
                        "   SET name     = ?,"
                        "       password = ?,"
                        "       fullname = ?,"
                        "       office   = ?,"
                        "       email    = ?,"
                        "       phone    = ?,"
                        "       comment  = ?"
                        " WHERE id       = ?");

    // Clear out existing group assignments.
    cdr::db::Statement del(dbConnection);
    del.setInt(1, usrId);
    del.executeQuery("DELETE grp_usr WHERE usr = ?");

    // Add groups to which user is assigned, if any.
    if (grpList.size() > 0) {
        StringList::const_iterator i = grpList.begin();
        while (i != grpList.end()) {
            const cdr::String gName = *i++;

            // Do INSERT the hard way so we can determine success.
            cdr::db::Statement grpQuery(dbConnection);
            grpQuery.setString(1, gName);
            cdr::db::ResultSet rs = 
                grpQuery.executeQuery("SELECT id"
                                      "  FROM grp"
                                      " WHERE name = ?");
            if (!rs.next())
                throw cdr::Exception(L"Unknown group", gName);
            int grpId = rs.getInt(1);
            cdr::db::Statement insert(dbConnection);
            insert.setInt(1, grpId);
            insert.setInt(2, usrId);
            insert.executeQuery("INSERT INTO grp_usr"
                                "("
                                "    grp,"
                                "    usr"
                                ")"
                                "VALUES(?, ?)");
        }
    }

    // Report success.
    dbConnection.commit();
    return L"  <CdrModUsrResp/>\n";
}
