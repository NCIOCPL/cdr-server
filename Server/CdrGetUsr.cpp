
/*
 * $Id$
 *
 * Retrieves information about requested user.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.5  2001/04/13 12:25:57  bkline
 * Fixed typo in tag (UsrName -> UserName).
 *
 * Revision 1.4  2000/12/28 13:27:17  bkline
 * Fixed typo in closing tag for CdrGetUsrResp.
 *
 * Revision 1.3  2000/05/03 15:25:41  bkline
 * Fixed database statement creation.
 *
 * Revision 1.2  2000/04/23 01:19:58  bkline
 * Added function-level comment header.
 *
 * Revision 1.1  2000/04/22 09:25:50  bkline
 * Initial revision
 */

#include "CdrCommand.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"
#include "CdrString.h"

/**
 * Load the row from the usr table belonging to the specified user, as well as
 * the rows from the grp_usr table joined to the usr row.
 */
cdr::String cdr::getUsr(cdr::Session& session,
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& conn)
{
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

    // Look up the base information for the user.
    std::string query = "SELECT id,"
                        "       name,"
                        "       password,"
                        "       fullname,"
                        "       office,"
                        "       email,"
                        "       phone,"
                        "       comment"
                        "  FROM usr"
                        " WHERE name = ?";
    cdr::db::PreparedStatement usrQuery = conn.prepareStatement(query);
    usrQuery.setString(1, usrName);
    cdr::db::ResultSet usrRs = usrQuery.executeQuery();
    if (!usrRs.next())
        throw cdr::Exception(L"User not found", usrName);
    int         uid      = usrRs.getInt(1);
    cdr::String uName    = usrRs.getString(2);
    cdr::String password = usrRs.getString(3);
    cdr::String fullname = usrRs.getString(4);
    cdr::String office   = usrRs.getString(5);
    cdr::String email    = usrRs.getString(6);
    cdr::String phone    = usrRs.getString(7);
    cdr::String comment  = usrRs.getString(8);

    // Make sure our user is authorized to retrieve this information.
    // Any user can access his own info, but special authorization is
    //   required to access other users' info.
    // Note that user name info is case sensitive
    cdr::String sessionUsr = session.getUserName();
    if (uName != sessionUsr)
        if (!session.canDo(conn, L"GET USER", L""))
            throw cdr::Exception(
                    L"GET USER action not authorized for this user");

    // Initialize a successful response.
    cdr::String response = cdr::String(L"  <CdrGetUsrResp>\n"
                                       L"   <UserName>")
                                     + uName
                                     + L"</UserName>\n"
                                     + L"   <Password>"
                                     + password
                                     + L"</Password>\n";
    if (fullname.size() > 0)
        response += L"   <FullName>"
                 +  fullname
                 +  L"</FullName>\n";
    if (office.size() > 0)
        response += L"   <Office>"
                 +  office
                 +  L"</Office>\n";
    if (email.size() > 0)
        response += L"   <Email>"
                 +  email
                 +  L"</Email>\n";
    if (phone.size() > 0)
        response += L"   <Phone>"
                 +  phone
                 +  L"</Phone>\n";

    // Find the groups to which this user is assigned.
    query = "SELECT g.name"
            "  FROM grp g,"
            "       grp_usr gu"
            " WHERE gu.usr = ?"
            "   AND g.id   = gu.grp";
    cdr::db::PreparedStatement grpSelect = conn.prepareStatement(query);
    grpSelect.setInt(1, uid);
    cdr::db::ResultSet grpRs = grpSelect.executeQuery();
    while (grpRs.next())
        response += L"   <GrpName>" + grpRs.getString(1) + L"</GrpName>\n";

    // Add the comment if present.
    if (!comment.isNull() && comment.size() > 0)
        response += L"   <Comment>" + comment + L"</Comment>\n";

    // Report success.
    return response + L"  </CdrGetUsrResp>\n";
}
