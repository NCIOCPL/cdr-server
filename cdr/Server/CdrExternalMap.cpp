/*
 * $Id: CdrExternalMap.cpp,v 1.1 2004-08-11 17:48:15 bkline Exp $
 *
 * Commands for manipulating mappings of external values to CDR documents.
 *
 * $Log: not supported by cvs2svn $
 */

#include "CdrCommand.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"

/**
 * Inserts a new row into the external mapping table.
 *
 *  @param      session     contains information about the current user.
 *  @param      node        contains the XML for the command.
 *  @param      conn        reference to the connection object for the
 *                          CDR database.
 *  @return                 String object containing the XML for the
 *                          command response.
 *  @exception  cdr::Exception if a database or processing error occurs.
 */
cdr::String cdr::addExternalMapping(cdr::Session& session,
                                    const cdr::dom::Node& commandNode,
                                    cdr::db::Connection& conn)
{
    // Extract the data elements from the command node.
    String usage(true);
    String value(true);
    String cdrId(true);
    dom::Node child = commandNode.getFirstChild();
    while (child != 0) {
        String name = child.getNodeName();
        if (name == L"Usage")
            usage = dom::getTextContent(child);
        else if (name == L"Value")
            value = dom::getTextContent(child);
        else if (name == L"CdrId")
            cdrId = dom::getTextContent(child);
        child = child.getNextSibling();
    }
    if (usage.size() == 0)
        throw Exception(L"Missing usage name");
    if (value.isNull())
        throw Exception(L"Missing mapping value");

    // Find the usage ID and action name.
    std::string usageQuery = "SELECT u.id, a.name         "
                             "  FROM external_map_usage u "
                             "  JOIN action a             "
                             "    ON a.id = u.auth_action " 
                             " WHERE u.name = ?           ";
    db::PreparedStatement usageStatement = conn.prepareStatement(usageQuery);
    usageStatement.setString(1, usage);
    db::ResultSet usageResults = usageStatement.executeQuery();
    if (!usageResults.next())
        throw Exception(L"Unknown usage " + usage);
    int    usageId    = usageResults.getInt(1);
    String authAction = usageResults.getString(2);

    // Check to make sure this user is allowed to add the row.
    if (!session.canDo(conn, authAction, L""))
        throw Exception(L"User not authorized to add " + usage + L" mappings");

    // Add the row.
    std::string insertQuery =
        "INSERT INTO external_map (usage, value, doc_id, usr, last_mod) "
        "     VALUES (?, ?, ?, ?, GETDATE())                            ";
    db::PreparedStatement insertStatement = conn.prepareStatement(insertQuery);
    Int docId(true);
    if (!cdrId.isNull())
        docId = cdrId.extractDocId();
    insertStatement.setInt(1, usageId);
    insertStatement.setString(2, value);
    insertStatement.setInt(3, docId);
    insertStatement.setInt(4, session.getUserId());
    insertStatement.executeQuery();

    // Report success.
    return L"<CdrAddExternalMappingResp/>\n";
}
