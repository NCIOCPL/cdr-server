
/*
 * $Id: CdrAddUsr.cpp,v 1.2 2000-04-23 01:13:47 bkline Exp $
 *
 * Adds new user to CDR.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2000/04/22 09:26:12  bkline
 * Initial revision
 */

#include "CdrCommand.h"
#include "CdrDbResultSet.h"

/**
 * Adds a row to the user table, as well as one row to the grp_usr table for
 * each group to which the user has been assigned.
 */
cdr::String cdr::addUsr(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection) 
{
    // Make sure our user is authorized to add users.
    if (!session.canDo(dbConnection, L"CREATE USER", L""))
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
            else if (name == L"Comment")
                comment = cdr::dom::getTextContent(child);
        }
        child = child.getNextSibling();
    }
    if (uName.size() == 0)
        throw cdr::Exception(L"Missing user name");
    if (password.isNull())
        throw cdr::Exception(L"Missing password");

    // Make sure the user doesn't already exist.
    cdr::db::Statement usrQuery(dbConnection);
    usrQuery.setString(1, uName);
    cdr::db::ResultSet usrRs = usrQuery.executeQuery("SELECT COUNT(*)"
                                                     "  FROM usr"
                                                     " WHERE name = ?");
    if (!usrRs.next())
        throw cdr::Exception(L"Failure checking unique user name");
    int count = usrRs.getInt(1);
    if (count != 0)
        throw cdr::Exception(L"User name already exists");

    // Add a new row to the usr table.
    dbConnection.setAutoCommit(false);
    cdr::db::Statement usrInsert(dbConnection);
    usrInsert.setString(1, uName);
    usrInsert.setString(2, password);
    usrInsert.setString(3, fullName);
    usrInsert.setString(4, office);
    usrInsert.setString(5, email);
    usrInsert.setString(6, phone);
    usrInsert.setString(7, comment);
    usrInsert.executeQuery("INSERT INTO usr"
                           "("
                           "    name,"
                           "    password,"
                           "    created,"
                           "    fullname,"
                           "    office,"
                           "    email,"
                           "    phone,"
                           "    comment"
                           ")"
                           "VALUES(?, ?, GETDATE(), ?, ?, ?, ?, ?)");
    int usrId = dbConnection.getLastIdent();

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
    return L"  <CdrAddUsrResp/>\n";
}
