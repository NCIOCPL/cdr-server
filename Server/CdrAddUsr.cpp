/*
 * $Id$
 *
 * Adds new user to CDR.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.4  2000/05/09 21:06:56  bkline
 * Fixed typo in SQL statement (removed superfluous comma).
 *
 * Revision 1.3  2000/05/03 15:25:41  bkline
 * Fixed database statement creation.
 *
 * Revision 1.2  2000/04/23 01:13:47  bkline
 * Added function-level comment.
 *
 * Revision 1.1  2000/04/22 09:26:12  bkline
 * Initial revision
 */

#include "CdrCommand.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"

/**
 * Adds a row to the user table, as well as one row to the grp_usr table for
 * each group to which the user has been assigned.
 */
cdr::String cdr::addUsr(cdr::Session& session,
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& conn)
{
    // Make sure our user is authorized to add users.
    if (!session.canDo(conn, L"CREATE USER", L""))
        throw
            cdr::Exception(L"CREATE USER action not authorized for this user");

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
        }
        child = child.getNextSibling();
    }
    if (uName.size() == 0)
        throw cdr::Exception(L"Missing user name");
    if (password.isNull())
        throw cdr::Exception(L"Missing password");

    // Don't create the record if the password fails standards
    // Throws exception if pw fails
    testPasswordString(password);

    // Make sure the user doesn't already exist.
    std::string select = "SELECT COUNT(*) FROM usr WHERE name = ?";
    cdr::db::PreparedStatement usrQuery = conn.prepareStatement(select);
    usrQuery.setString(1, uName);
    cdr::db::ResultSet usrRs = usrQuery.executeQuery();
    if (!usrRs.next())
        throw cdr::Exception(L"Failure checking unique user name");
    int count = usrRs.getInt(1);
    usrQuery.close();

    if (count != 0)
        throw cdr::Exception(L"User name already exists");

    // Add a new row to the usr table.
    conn.setAutoCommit(false);
    std::string insert =
        "INSERT INTO usr(name,"
        "                password,"
        "                created,"
        "                fullname,"
        "                office,"
        "                email, "
        "                phone,"
        "                comment)"
        "        VALUES (?, 'tHiS-W1LlBEEreP8sEd', GETDATE(), ?, ?, ?, ?, ?)";
    cdr::db::PreparedStatement usrInsert = conn.prepareStatement(insert);
    usrInsert.setString(1, uName);
    usrInsert.setString(2, fullName);
    usrInsert.setString(3, office);
    usrInsert.setString(4, email);
    usrInsert.setString(5, phone);
    usrInsert.setString(6, comment);
    usrInsert.executeQuery();
    int usrId = conn.getLastIdent();
    usrInsert.close();

    // Install the password for new user
    session.setUserPw(conn, uName, password, true);

    // Add groups to which user is assigned, if any.
    if (grpList.size() > 0) {
        StringList::const_iterator i = grpList.begin();
        while (i != grpList.end()) {
            const cdr::String gName = *i++;

            // Do INSERT the hard way so we can determine success.
            std::string select = "SELECT id FROM grp WHERE name = ?";
            cdr::db::PreparedStatement grpQuery = conn.prepareStatement(select);
            grpQuery.setString(1, gName);
            cdr::db::ResultSet rs = grpQuery.executeQuery();
            if (!rs.next())
                throw cdr::Exception(L"Unknown group", gName);
            int grpId = rs.getInt(1);
            grpQuery.close();

            std::string insert = "INSERT INTO grp_usr(grp, usr) VALUES(?, ?)";
            cdr::db::PreparedStatement ps = conn.prepareStatement(insert);
            ps.setInt(1, grpId);
            ps.setInt(2, usrId);
            ps.executeQuery();
            ps.close();
        }
    }

    // Report success.
    conn.commit();
    return L"  <CdrAddUsrResp/>\n";
}
