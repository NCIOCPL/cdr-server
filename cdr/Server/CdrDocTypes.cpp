/*
 * $Id: CdrDocTypes.cpp,v 1.2 2001-01-17 21:50:33 bkline Exp $
 *
 * Support routines for CDR document types.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2001/01/16 21:11:52  bkline
 * Initial revision
 */

#include "CdrDom.h"
#include "CdrCommand.h"
#include "CdrException.h"
#include "CdrDbResultSet.h"
#include "CdrDbStatement.h"

/**
 * Provides a list of document types currently defined for the CDR.
 *
 *  @param      session     contains information about the current user.
 *  @param      node        contains the XML for the command.
 *  @param      conn        reference to the connection object for the
 *                          CDR database.
 *  @return                 String object containing the XML for the
 *                          command response.
 *  @exception  cdr::Exception if a database or processing error occurs.
 */
cdr::String cdr::listDocTypes(Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn)
{
    // Make sure the user is authorized to retrieve the document types.
    if (!session.canDo(conn, L"LIST DOCTYPES", L""))
        throw cdr::Exception(
                L"LIST DOCTYPES action not authorized for this user");

    // Submit the query to the database
    cdr::db::Statement s = conn.createStatement();
    cdr::db::ResultSet r = s.executeQuery("SELECT name FROM doc_type");
    
    // Pull in the names from the result set.
    cdr::String response;
    while (r.next()) {
        String name = r.getString(1);
        if (name.length() < 1)
            continue;
        if (response.size() == 0)
            response = L"  <CdrListDocTypesResp>\n";
        response += L"   <DocType>" + name + L"</DocType>\n";
    }
    if (response.size() == 0)
        return L"  <CdrListDocTypesResp/>\n";
    else
        return response + L"  </CdrListDocTypesResp>\n";
}

/**
 * Load the row from the doc_type table for the requested document type.
 * Send back the schema and other related document type information.
 *
 *  @param      session     contains information about the current user.
 *  @param      node        contains the XML for the command.
 *  @param      conn        reference to the connection object for the
 *                          CDR database.
 *  @return                 String object containing the XML for the
 *                          command response.
 *  @exception  cdr::Exception if a database or processing error occurs.
 */
cdr::String cdr::getDocType(Session&          session,
                            const dom::Node&  node,
                            db::Connection&   conn)
{
    // Make sure the user is authorized to retrieve doctype information.
    if (!session.canDo(conn, L"GET DOCTYPE", L""))
        throw cdr::Exception(
                L"GET DOCTYPE action not authorized for this user");

    // Find out which document type we are supposed to retrieve.
    const cdr::dom::Element& cmdElement = 
        static_cast<const cdr::dom::Element&>(node);
    cdr::String docTypeString = cmdElement.getAttribute(L"Type");
    if (docTypeString.empty())
        throw cdr::Exception(L"Type attribute missing from CdrGetDocType "
                             L"command element");

    // Load the document type information.
    std::string query = "SELECT f.name,        "
                        "       t.dtd,         "
                        "       t.xml_schema,  "
                        "       t.comment,     "
                        "       t.created,     "
                        "       t.versioning,  "
                        "       t.schema_date  "
                        "  FROM doc_type t,    "
                        "       format f       "
                        " WHERE t.name   = ?   "
                        "   AND t.format = f.id";
    cdr::db::PreparedStatement ps = conn.prepareStatement(query);
    ps.setString(1, docTypeString);
    cdr::db::ResultSet rs = ps.executeQuery();
    if (!rs.next())
        throw cdr::Exception(L"Unknown document type", docTypeString);
    cdr::String format     = rs.getString(1);
    cdr::String dtd        = rs.getString(2);
    cdr::String schema     = rs.getString(3);
    cdr::String comment    = rs.getString(4);
    cdr::String created    = rs.getString(5);
    cdr::String versioning = rs.getString(6);
    cdr::String schemaMod  = rs.getString(7);

    std::wostringstream resp;
    resp << L"<CdrGetDocTypeResp Type='"
         << docTypeString
         << L"' Format='"
         << format
         << L"' Versioning='"
         << versioning
         << L"' Created='"
         << created
         << L"' SchemaMod='"
         << schemaMod
         << L"'><DocDtd><![CDATA["
         << dtd
         << L"]]></DocDtd><DocSchema><![CDATA["
         << schema
         << L"]]></DocSchema>";
    if (!comment.isNull())
        resp << L"<Comment>" << comment << L"</Comment>";
    resp << L"</CdrGetDocTypeResp>";
    return resp.str();
}

/**
 * Adds a new document type to the system.
 *
 *  @param      session     contains information about the current user.
 *  @param      node        contains the XML for the command.
 *  @param      conn        reference to the connection object for the
 *                          CDR database.
 *  @return                 String object containing the XML for the
 *                          command response.
 *  @exception  cdr::Exception if a database or processing error occurs.
 */
cdr::String cdr::addDocType(Session&          session,
                            const dom::Node&  node,
                            db::Connection&   conn)
{
    // Make sure the user is authorized to add a new document type.
    if (!session.canDo(conn, L"ADD DOCTYPE", L""))
        throw cdr::Exception(
                L"ADD DOCTYPE action not authorized for this user");

    // Extract the attributes from the command element.
    const cdr::dom::Element& cmdElement = 
        static_cast<const cdr::dom::Element&>(node);
    cdr::String docTypeString    = cmdElement.getAttribute(L"Type");
    cdr::String docFormatString  = cmdElement.getAttribute(L"Format");
    cdr::String versioningString = cmdElement.getAttribute(L"Versioning");
    if (docTypeString.empty())
        throw cdr::Exception(L"Type attribute missing from CdrAddDocType "
                             L"command element");
    if (docFormatString.empty())
        docFormatString = L"xml";
    if (versioningString.empty())
        versioningString = L"Y";
    if (versioningString != L"Y" && versioningString != L"N")
        throw cdr::Exception(L"Versioning attribute can only be Y or N; was",
                             versioningString);

    // Extract the schema.
    cdr::String schema;
    cdr::String comment(true);  // default to NULL
    cdr::dom::Node child = node.getFirstChild();
    while (schema.empty() && child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"DocSchema") {
                cdr::dom::Node n = child.getFirstChild();
                while (n != 0) {
                    if (n.getNodeType() == cdr::dom::Node::CDATA_SECTION_NODE) {
                        schema = n.getNodeValue();
                        break;
                    }
                    n = n.getNextSibling();
                }
            }
            else if (name == L"Comment")
                comment = cdr::dom::getTextContent(child);
        }
        child = child.getNextSibling();
    }
    if (schema.empty())
        throw cdr::Exception(L"DocSchema element missing from command");

    // Look up the format's primary key.
    std::string q1 = "SELECT id FROM format WHERE name = ?";
    cdr::db::PreparedStatement ps1 = conn.prepareStatement(q1);
    ps1.setString(1, docFormatString);
    cdr::db::ResultSet rs1 = ps1.executeQuery();
    if (!rs1.next())
        throw cdr::Exception(L"Unrecognized doc-type format", docFormatString);
    int formatId = rs1.getInt(1);

    // Make sure there isn't already a document type with this name.
    std::string q2 = "SELECT COUNT(*) FROM doc_type WHERE name = ?";
    cdr::db::PreparedStatement ps2 = conn.prepareStatement(q2);
    ps2.setString(1, docTypeString);
    cdr::db::ResultSet rs2 = ps2.executeQuery();
    if (!rs2.next())
        throw cdr::Exception(L"Internal error checking for document type",
                             docTypeString);
    int nTypes = rs2.getInt(1);
    if (nTypes != 0)
        throw cdr::Exception(L"Document type already exists",
                             docTypeString);
    
    // Create the new document type.
    std::string q3 = "INSERT INTO doc_type"
                     "(                   "
                     "    name,           "
                     "    format,         "
                     "    created,        "
                     "    versioning,     "
                     "    dtd,            "
                     "    xml_schema,     "
                     "    schema_date,    "
                     "    comment         "
                     ")                   "
                     "VALUES              "
                     "(                   "
                     "    ?,              "
                     "    ?,              "
                     "    GETDATE(),      "
                     "    ?,              "
                     "    '',             "
                     "    ?,              "
                     "    GETDATE(),      "
                     "    ?               "
                     ")                   ";
    cdr::db::PreparedStatement ps3 = conn.prepareStatement(q3);
    ps3.setString(1, docTypeString);
    ps3.setInt   (2, formatId);
    ps3.setString(3, versioningString);
    ps3.setString(4, schema);
    ps3.setString(5, comment);
    if (ps3.executeUpdate() != 1)
        throw cdr::Exception(L"Failure inserting new document type",
                             docTypeString);
                     
    // Report success.
    return L"<CdrAddDocTypeResp/>";
}

/**
 * Modifies an existing document type in the CDR.
 *
 *  @param      session     contains information about the current user.
 *  @param      node        contains the XML for the command.
 *  @param      conn        reference to the connection object for the
 *                          CDR database.
 *  @return                 String object containing the XML for the
 *                          command response.
 *  @exception  cdr::Exception if a database or processing error occurs.
 */
cdr::String cdr::modDocType(Session&          session,
                            const dom::Node&  node,
                            db::Connection&   conn)
{
    // Make sure the user is authorized to modify an existing document type.
    if (!session.canDo(conn, L"MODIFY DOCTYPE", L""))
        throw cdr::Exception(
                L"MODIFY DOCTYPE action not authorized for this user");

    // Extract the attributes from the command element.
    const cdr::dom::Element& cmdElement = 
        static_cast<const cdr::dom::Element&>(node);
    cdr::String docTypeString    = cmdElement.getAttribute(L"Type");
    cdr::String docFormatString  = cmdElement.getAttribute(L"Format");
    cdr::String versioningString = cmdElement.getAttribute(L"Versioning");
    if (docTypeString.empty())
        throw cdr::Exception(L"Type attribute missing from CdrModDocType "
                             L"command element");
    if (docFormatString.empty())
        docFormatString = L"xml";
    if (versioningString.empty())
        versioningString = L"Y";
    if (versioningString != L"Y" && versioningString != L"N")
        throw cdr::Exception(L"Versioning attribute can only be Y or N; was",
                             versioningString);

    // Extract the schema.
    cdr::String schema;
    cdr::String comment(true);  // default to NULL
    cdr::dom::Node child = node.getFirstChild();
    while (schema.empty() && child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"DocSchema") {
                cdr::dom::Node n = child.getFirstChild();
                while (n != 0) {
                    if (n.getNodeType() == cdr::dom::Node::CDATA_SECTION_NODE) {
                        schema = n.getNodeValue();
                        break;
                    }
                    n = n.getNextSibling();
                }
            }
            else if (name == L"Comment")
                comment = cdr::dom::getTextContent(child);
        }
        child = child.getNextSibling();
    }
    if (schema.empty())
        throw cdr::Exception(L"DocSchema element missing from command");

    // Look up the format's primary key.
    std::string q1 = "SELECT id FROM format WHERE name = ?";
    cdr::db::PreparedStatement ps1 = conn.prepareStatement(q1);
    ps1.setString(1, docFormatString);
    cdr::db::ResultSet rs1 = ps1.executeQuery();
    if (!rs1.next())
        throw cdr::Exception(L"Unrecognized doc-type format", docFormatString);
    int formatId = rs1.getInt(1);

    // Look up the key for the document type.
    std::string q2 = "SELECT id FROM doc_type WHERE name = ?";
    cdr::db::PreparedStatement ps2 = conn.prepareStatement(q2);
    ps2.setString(1, docTypeString);
    cdr::db::ResultSet rs2 = ps2.executeQuery();
    if (!rs2.next())
        throw cdr::Exception(L"Unrecognized document type", docTypeString);
    int docTypeId = rs2.getInt(1);

    // Create the new document type.
    std::string q3 = "UPDATE doc_type                "
                     "   SET format      = ?,        "
                     "       versioning  = ?,        "
                     "       xml_schema  = ?,        "
                     "       schema_date = GETDATE(),"
                     "       comment     = ?         "
                     " WHERE id          = ?         ";
    cdr::db::PreparedStatement ps3 = conn.prepareStatement(q3);
    ps3.setInt   (1, formatId);
    ps3.setString(2, versioningString);
    ps3.setString(3, schema);
    ps3.setString(4, comment);
    ps3.setInt   (5, docTypeId);
    if (ps3.executeUpdate() != 1)
        throw cdr::Exception(L"Failure modifying document type",
                             docTypeString);
                     
    // Report success.
    return L"<CdrModDocTypeResp/>";
}

/**
 * Removes a document type from the CDR system.
 *
 *  @param      session     contains information about the current user.
 *  @param      node        contains the XML for the command.
 *  @param      conn        reference to the connection object for the
 *                          CDR database.
 *  @return                 String object containing the XML for the
 *                          command response.
 *  @exception  cdr::Exception if a database or processing error occurs.
 */
cdr::String cdr::delDocType(Session&          session,
                            const dom::Node&  node,
                            db::Connection&   conn)
{
    // Make sure the user is authorized to delete an existing document type.
    if (!session.canDo(conn, L"DELETE DOCTYPE", L""))
        throw cdr::Exception(
                L"DELETE DOCTYPE action not authorized for this user");

    // Find out which document type we are supposed to remove.
    const cdr::dom::Element& cmdElement = 
        static_cast<const cdr::dom::Element&>(node);
    cdr::String docTypeString = cmdElement.getAttribute(L"Type");
    if (docTypeString.empty())
        throw cdr::Exception(L"Type attribute missing from CdrDelDocType "
                             L"command element");

    // Make sure the document type exists.
    std::string q1 = "SELECT id FROM doc_type WHERE name = ?";
    cdr::db::PreparedStatement ps1 = conn.prepareStatement(q1);
    ps1.setString(1, docTypeString);
    cdr::db::ResultSet rs1 = ps1.executeQuery();
    if (!rs1.next())
        throw cdr::Exception(L"Unknown document type", docTypeString);
    int id = rs1.getInt(1);

    // Make sure there are no existing documents of this type.
    std::string q2 = "SELECT COUNT(*) FROM document WHERE doc_type = ?";
    cdr::db::PreparedStatement ps2 = conn.prepareStatement(q2);
    ps2.setInt(1, id);
    cdr::db::ResultSet rs2 = ps2.executeQuery();
    if (!rs2.next())
        throw cdr::Exception(L"Internal error checking for documents of type",
                             docTypeString);
    int nDocs = rs2.getInt(1);
    if (nDocs != 0)
        throw cdr::Exception(L"Cannot delete document type for which "
                             L"documents exist", docTypeString);
    
    // Wipe out any other related rows for this document type.
    std::string q3 = "DELETE grp_action  WHERE doc_type        = ?";
    std::string q4 = "DELETE link_xml    WHERE doc_type        = ?";
    std::string q5 = "DELETE link_target WHERE target_doc_type = ?";
    cdr::db::PreparedStatement ps3 = conn.prepareStatement(q3);
    cdr::db::PreparedStatement ps4 = conn.prepareStatement(q4);
    cdr::db::PreparedStatement ps5 = conn.prepareStatement(q5);
    ps3.setInt(1, id);
    ps4.setInt(1, id);
    ps5.setInt(1, id);
    ps3.executeUpdate();
    ps4.executeUpdate();
    ps5.executeUpdate();

    // Wipe out the document type.
    std::string q6 = "DELETE doc_type WHERE id = ?";
    cdr::db::PreparedStatement ps6 = conn.prepareStatement(q6);
    ps6.setInt(1, id);
    if (ps6.executeUpdate() != 1)
        throw cdr::Exception(L"Internal failure encountered deleting document "
                             L"type", docTypeString);

    // Report success.
    return L"<CdrDelDocTypeResp/>";
}
