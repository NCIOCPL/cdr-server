/*
 * $Id$
 *
 * Commands for viewing, editing, creating CDR actions.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2001/11/28 19:28:20  bkline
 * Initial revision
 *
 */

#include "CdrCommand.h"
#include "CdrDbStatement.h"
#include "CdrDbResultSet.h"

/**
 * Selects all rows in the action table and displays the values from the name
 * and doctype_specific columns.
 */
cdr::String cdr::listActions(cdr::Session& session,
                             const cdr::dom::Node& commandNode,
                             cdr::db::Connection& dbConnection)
{
    // Make sure our user is authorized to list groups.
    if (!session.canDo(dbConnection, L"LIST ACTIONS", L""))
        throw cdr::Exception(
                L"LIST ACTIONS action not authorized for this user");

    // Submit the query to the database
    cdr::db::Statement s = dbConnection.createStatement();
    cdr::db::ResultSet r = s.executeQuery("SELECT name,"
                                          "       doctype_specific"
                                          "  FROM action");

    // Pull in the rows from the result set.
    cdr::String resp;
    while (r.next()) {
        if (resp.empty())
            resp = L"<CdrListActionsResp>\n";
        cdr::String name = r.getString(1);
        cdr::String flag = r.getString(2);
        resp += L"<Action><Name>" + name
              + L"</Name><NeedDoctype>" + flag
              + L"</NeedDoctype></Action>\n";
    }
    if (resp.empty())
        return L"<CdrListActionsResp/>\n";
    else
        return resp + L"</CdrListActionsResp>\n";
}

/**
 * Retrieves the information for a single CDR action.
 */
cdr::String cdr::getAction(cdr::Session& session,
                           const cdr::dom::Node& commandNode,
                           cdr::db::Connection& dbConnection)
{
    // Make sure our user is authorized to list groups.
    if (!session.canDo(dbConnection, L"GET ACTION", L""))
        throw cdr::Exception(
                L"GET ACTION action not authorized for this user");

    // Extract the action name from the command.
    cdr::String actionName;
    cdr::dom::Node child = commandNode.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"Name")
                actionName = cdr::dom::getTextContent(child);
        }
        child = child.getNextSibling();
    }
    if (actionName.empty())
        throw cdr::Exception(L"Missing action name");

    // Look up the information for the action.
    std::string query = "SELECT name,"
                        "       doctype_specific,"
                        "       comment"
                        "  FROM action"
                        " WHERE name = ?";
    cdr::db::PreparedStatement s = dbConnection.prepareStatement(query);
    s.setString(1, actionName);
    cdr::db::ResultSet rs = s.executeQuery();
    if (!rs.next())
        throw cdr::Exception(L"Action not found", actionName);
    cdr::String name    = rs.getString(1);
    cdr::String flag    = rs.getString(2);
    cdr::String comment = rs.getString(3);

    // Create the response.
    cdr::String resp = L"<CdrGetActionResp>"
                       L"<Name>" + name + L"</Name>"
                       L"<DoctypeSpecific>" + flag + L"</DoctypeSpecific>"
                       L"<Comment>" + comment + L"</Comment>"
                       L"</CdrGetActionResp>";
    return resp;
}

/**
 * Adds a row to the action table.
 */
cdr::String cdr::addAction(cdr::Session& session,
                           const cdr::dom::Node& commandNode,
                           cdr::db::Connection& conn)
{
    // Make sure our user is authorized to add an action.
    if (!session.canDo(conn, L"ADD ACTION", L""))
        throw cdr::Exception(L"ADD ACTION action "
                             L"not authorized for this user");


    // Extract the data elements from the command node.
    cdr::String action, flag, comment(true);
    cdr::dom::Node child = commandNode.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"Name")
                action = cdr::dom::getTextContent(child);
            else if (name == L"DoctypeSpecific")
                flag = cdr::dom::getTextContent(child);
            else if (name == L"Comment")
                comment = cdr::dom::getTextContent(child);
        }
        child = child.getNextSibling();
    }
    if (action.size() == 0)
        throw cdr::Exception(L"Missing action name");
    if (flag != L"N" && flag != L"Y")
        throw cdr::Exception(L"DoctypeScecific element "
                             L"must contain 'Y' or 'N'");

    // Make sure the action doesn't already exist.
    std::string selectString = "SELECT COUNT(*) FROM action WHERE name = ?";
    cdr::db::PreparedStatement query = conn.prepareStatement(selectString);
    query.setString(1, action);
    cdr::db::ResultSet rs = query.executeQuery();
    if (!rs.next())
        throw cdr::Exception(L"Failure determining uniqueness of action");
    int count = rs.getInt(1);
    if (count != 0)
        throw cdr::Exception(L"Action already exists", action);

    // Add a new row to the action table.
    std::string insertString =
        "INSERT INTO action(name,"
        "                   doctype_specific,"
        "                   comment)"
        "           VALUES (?, ?, ?)";
    cdr::db::PreparedStatement insert = conn.prepareStatement(insertString);
    insert.setString(1, action);
    insert.setString(2, flag);
    insert.setString(3, comment);
    insert.executeQuery();

    return L"<CdrAddActionResp/>";
}

/**
 * Modifies an existing row in the action table.
 */
cdr::String cdr::repAction(cdr::Session& session,
                           const cdr::dom::Node& commandNode,
                           cdr::db::Connection& conn)
{
    // Make sure our user is authorized to modify actions.
    if (!session.canDo(conn, L"MODIFY ACTION", L""))
        throw cdr::Exception(L"MODIFY ACTION action "
                             L"not authorized for this user");


    // Extract the data elements from the command node.
    cdr::String action, flag, newName(true), comment(true);
    cdr::dom::Node child = commandNode.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"Name")
                action = cdr::dom::getTextContent(child);
            else if (name == L"NewName")
                newName = cdr::dom::getTextContent(child);
            else if (name == L"DoctypeSpecific")
                flag = cdr::dom::getTextContent(child);
            else if (name == L"Comment")
                comment = cdr::dom::getTextContent(child);
        }
        child = child.getNextSibling();
    }
    if (action.size() == 0)
        throw cdr::Exception(L"Missing action name");

    // Make sure the action exists.
    std::string selectString = "SELECT id FROM action WHERE name = ?";
    cdr::db::PreparedStatement query = conn.prepareStatement(selectString);
    query.setString(1, action);
    cdr::db::ResultSet rs = query.executeQuery();
    if (!rs.next())
        throw cdr::Exception(L"Failure locating action information", action);
    int id = rs.getInt(1);
    query.close();

    // Check for a name change.
    if (!newName.isNull()) {
        if (newName.empty())
            throw cdr::Exception(L"Cannot replace action name with empty name");
        action = newName;
    }

    // Make sure the doctype_specific flag is consistent with the assignments.
    if (flag == L"N") {
        std::string q = "SELECT COUNT(*)"
                        "  FROM grp_action"
                        " WHERE action = ?"
                        "   AND doc_type <> 1";
        cdr::db::PreparedStatement s = conn.prepareStatement(q);
        s.setInt(1, id);
        cdr::db::ResultSet r = s.executeQuery();
        if (!r.next())
            throw cdr::Exception(L"Failure checking doctype_specific flag");
        if (r.getInt(1) != 0)
            throw cdr::Exception(L"Cannot set doctype_specific flag to 'N'"
                                 L" because action has been assigned to "
                                 L"groups for specific doctypes");
        s.close();
    }
    else if (flag == L"Y") {
        std::string q = "SELECT COUNT(*)"
                        "  FROM grp_action"
                        " WHERE action = ?"
                        "   AND doc_type = 1";
        cdr::db::PreparedStatement s = conn.prepareStatement(q);
        s.setInt(1, id);
        cdr::db::ResultSet r = s.executeQuery();
        if (!r.next())
            throw cdr::Exception(L"Failure checking doctype_specific flag");
        if (r.getInt(1) != 0)
            throw cdr::Exception(L"Cannot set doctype_specific flag to 'Y'"
                                 L" because doctype-independent assignements"
                                 L" of this action have been made to groups");
        s.close();
    }
    else
        throw cdr::Exception(L"DoctypeScecific element "
                             L"must contain 'Y' or 'N'");

    // Update the row for the action.
    std::string updateString =
        "UPDATE action"
        "   SET name = ?,"
        "       doctype_specific = ?,"
        "       comment = ?"
        " WHERE id = ?";
    cdr::db::PreparedStatement update = conn.prepareStatement(updateString);
    update.setString(1, action);
    update.setString(2, flag);
    update.setString(3, comment);
    update.setInt(4, id);
    update.executeQuery();

    return L"<CdrRepActionResp/>";
}

/**
 * Drops any rows from the grp_action table associated with this action,
 * then drops the row from the action table.
 */
cdr::String cdr::delAction(cdr::Session& session,
                           const cdr::dom::Node& commandNode,
                           cdr::db::Connection& conn)
{
    // Make sure our user is authorized to delete CDR actions.
    if (!session.canDo(conn, L"DELETE ACTION", L""))
        throw cdr::Exception(
                L"DELETE ACTION action not authorized for this user");

    // Extract the group name from the command.
    cdr::String action;
    cdr::dom::Node child = commandNode.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"Name")
                action = cdr::dom::getTextContent(child);
        }
        child = child.getNextSibling();
    }
    if (action.size() == 0)
        throw cdr::Exception(L"Missing action name");

    // Make sure the action exists.
    std::string query = "SELECT id FROM action WHERE name = ?";
    cdr::db::PreparedStatement actQuery = conn.prepareStatement(query);
    actQuery.setString(1, action);
    cdr::db::ResultSet actRs = actQuery.executeQuery();
    if (!actRs.next())
        throw cdr::Exception(L"Action not found", action);
    int actionId = actRs.getInt(1);
    actQuery.close();

    // Drop action assignments to groups.
    conn.setAutoCommit(false);
    query = "DELETE grp_action WHERE action = ?";
    cdr::db::PreparedStatement dropGrpActions = conn.prepareStatement(query);
    dropGrpActions.setInt(1, actionId);
    dropGrpActions.executeQuery();

    // Finish the job.
    query = "DELETE action WHERE id = ?";
    cdr::db::PreparedStatement dropAction = conn.prepareStatement(query);
    dropAction.setInt(1, actionId);
    dropAction.executeQuery();
    conn.commit();
    return L"  <CdrDelActionResp/>\n";
}
