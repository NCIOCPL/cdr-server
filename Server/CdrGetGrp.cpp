
/*
 * $Id$
 *
 * Retrieves information about requested group.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.4  2002/02/01 20:49:19  bkline
 * Modified query to use expired column.
 *
 * Revision 1.3  2001/04/13 12:25:13  bkline
 * Retrieve grp.name so case of group name is preserved.
 *
 * Revision 1.2  2000/05/03 15:25:41  bkline
 * Fixed database statement creation.
 *
 * Revision 1.1  2000/04/22 09:31:58  bkline
 * Initial revision
 *
 */

#include "CdrCommand.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"

cdr::String cdr::getGrp(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& conn) 
{
    // Make sure our user is authorized to retrieve group information.
    if (!session.canDo(conn, L"GET GROUP", L""))
        throw cdr::Exception(L"GET GROUP action not authorized for this user");

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

    // Look up the base information for the group.
    std::string query = "SELECT id, name, comment FROM grp WHERE name = ?";
    cdr::db::PreparedStatement grpQuery = conn.prepareStatement(query);
    grpQuery.setString(1, grpName);
    cdr::db::ResultSet grpRs = grpQuery.executeQuery();
    if (!grpRs.next())
        throw cdr::Exception(L"Group not found", grpName);
    int         gid     = grpRs.getInt(1);
    cdr::String name    = grpRs.getString(2);
    cdr::String comment = grpRs.getString(3);

    // Initialize a successful response.
    cdr::String response = cdr::String(L"  <CdrGetGrpResp>\n"
                                       L"   <GrpName>")
                                     + name
                                     + L"</GrpName>\n";

    // Find the authorizations associated with this group.
    query = "SELECT a.name,"
            "       t.name"
            "  FROM action a,"
            "       doc_type t,"
            "       grp_action ga"
            " WHERE ga.grp = ?"
            "   AND t.id   = ga.doc_type"
            "   AND a.id   = ga.action";
    cdr::db::PreparedStatement authSelect = conn.prepareStatement(query);
    authSelect.setInt(1, gid);
    cdr::db::ResultSet authRs = authSelect.executeQuery();
    while (authRs.next()) {
        cdr::String action  = authRs.getString(1);
        cdr::String docType = authRs.getString(2);
        response += L"   <Auth><Action>" +  action + L"</Action>";
        if (docType.size() > 0)
            response += L"<DocType>" +  docType + L"</DocType>";
        response += L"</Auth>\n";
    }

    // Find all the users associated with this group.
    query = "SELECT u.name"
            "  FROM usr u,"
            "       grp_usr g"
            " WHERE g.grp = ?"
            "   AND g.usr = u.id"
            "   AND (u.expired IS NULL"
            "    OR u.expired > GETDATE())";
    cdr::db::PreparedStatement usrSelect = conn.prepareStatement(query);
    usrSelect.setInt(1, gid);
    cdr::db::ResultSet usrRs = usrSelect.executeQuery();
    while (usrRs.next())
        response += L"   <UserName>" + usrRs.getString(1) + L"</UserName>\n";

    // Add the comment if present.
    if (!comment.isNull() && comment.size() > 0)
        response += L"   <Comment>" + comment + L"</Comment>\n";


    // Report success.
    return response + L"  </CdrGetGrpResp>\n";
}
