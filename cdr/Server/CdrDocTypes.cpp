/*
 * $Id: CdrDocTypes.cpp,v 1.9 2001-11-06 21:40:32 bkline Exp $
 *
 * Support routines for CDR document types.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.8  2001/06/28 17:38:17  bkline
 * Added method getCssFiles().
 *
 * Revision 1.7  2001/06/12 11:07:58  bkline
 * Added code to report linking elements to the caller of CdrGetDocType.
 *
 * Revision 1.6  2001/05/21 20:48:29  bkline
 * Eliminated checks for permission to perform read-only doctype actions.
 *
 * Revision 1.5  2001/05/16 15:45:07  bkline
 * Added listSchemaDocs() command; modified commands to reflect shift
 * of schema documents from doc_type table to document table.
 *
 * Revision 1.4  2001/04/13 12:23:14  bkline
 * Fixed authorization check for GET DOCTYPE and MODIFY DOCTYPE, which
 * are document-type specific.
 *
 * Revision 1.3  2001/02/28 02:36:18  bkline
 * Fixed a bug which was preventing comments from being saved.
 *
 * Revision 1.2  2001/01/17 21:50:33  bkline
 * Added CdrAdd/Mod/Del/GetDocType commands.
 *
 * Revision 1.1  2001/01/16 21:11:52  bkline
 * Initial revision
 */

// Eliminate annoying warnings about truncated debugging information.
#pragma warning(disable : 4786)

#include <algorithm>
#include "CdrDom.h"
#include "CdrCommand.h"
#include "CdrException.h"
#include "CdrDbResultSet.h"
#include "CdrDbStatement.h"
#include "CdrXsd.h"

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
    // Submit the query to the database
    cdr::db::Statement s = conn.createStatement();
    cdr::db::ResultSet r = s.executeQuery("   SELECT name         "
                                          "     FROM doc_type     "
                                          "    WHERE active = 'Y' "
                                          " ORDER BY name         ");
    
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
 * Provides a list of schema documents currently stored in the CDR.
 *
 *  @param      session     contains information about the current user.
 *  @param      node        contains the XML for the command.
 *  @param      conn        reference to the connection object for the
 *                          CDR database.
 *  @return                 String object containing the XML for the
 *                          command response.
 *  @exception  cdr::Exception if a database or processing error occurs.
 */
cdr::String cdr::listSchemaDocs(Session&          session,
                                const dom::Node&  node,
                                db::Connection&   conn)
{
    // Submit the query to the database
    cdr::db::Statement s = conn.createStatement();
    cdr::db::ResultSet r = s.executeQuery("SELECT d.title"
                                          "  FROM document d"
                                          "  JOIN doc_type t"
                                          "    ON t.id   = d.doc_type"
                                          " WHERE t.name = 'schema'");
    
    // Pull in the names from the result set.
    cdr::String response;
    while (r.next()) {
        String name = r.getString(1);
        if (name.length() < 1)
            continue;
        if (response.size() == 0)
            response = L"<CdrListSchemaDocsResp>";
        response += L"<DocTitle>" + name + L"</DocTitle>";
    }
    if (response.size() == 0)
        return L"<CdrListSchemaDocsResp/>";
    else
        return response + L"</CdrListSchemaDocsResp>";
}

cdr::StringList getLinkingElements(int id, cdr::db::Connection& conn)
{
    cdr::StringList linkingElements;
    static const char* query = "SELECT DISTINCT x.element                 "
                               "           FROM link_xml x                "
                               "           JOIN link_type t               "
                               "             ON x.link_id = t.id          "
                               "           JOIN link_target g             "
                               "             ON g.source_link_type = t.id "
                               "          WHERE x.doc_type = ?            ";
    cdr::db::PreparedStatement ps = conn.prepareStatement(query);
    ps.setInt(1, id);
    cdr::db::ResultSet rs = ps.executeQuery();
    while (rs.next()) {
        cdr::String elemName = rs.getString(1);
        if (!elemName.empty())
            linkingElements.push_back(elemName);
    }
    ps.close();
    return linkingElements;
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
    // Find out which document type we are supposed to retrieve.
    const cdr::dom::Element& cmdElement = 
        static_cast<const cdr::dom::Element&>(node);
    cdr::String docTypeString = cmdElement.getAttribute(L"Type");
    cdr::String getEnumValues = cmdElement.getAttribute(L"GetEnumValues");
    if (docTypeString.empty())
        throw cdr::Exception(L"Type attribute missing from CdrGetDocType "
                             L"command element");

    // Load the document type information.
    std::string query = "          SELECT f.name,             "
                        "                 t.comment,          "
                        "                 t.created,          "
                        "                 t.versioning,       "
                        "                 t.schema_date,      "
                        "                 d.xml,              "
                        "                 d.title,            "
                        "                 t.id                "
                        "            FROM doc_type t          "
                        "            JOIN format f            "
                        "              ON t.format = f.id     "
                        " LEFT OUTER JOIN document d          "
                        "              ON t.xml_schema = d.id "
                        "           WHERE t.name = ?          ";
    cdr::db::PreparedStatement ps = conn.prepareStatement(query);
    ps.setString(1, docTypeString);
    cdr::db::ResultSet rs = ps.executeQuery();
    if (!rs.next())
        throw cdr::Exception(L"Unknown document type", docTypeString);
    cdr::String format     = rs.getString(1);
    cdr::String comment    = rs.getString(2);
    cdr::String created    = rs.getString(3);
    cdr::String versioning = rs.getString(4);
    cdr::String schemaMod  = rs.getString(5);
    cdr::String schemaStr  = rs.getString(6);
    cdr::String schemaName = rs.getString(7);
    int         id         = rs.getInt(8);
    ps.close();
    cdr::StringList linkingElements = getLinkingElements(id, conn);

    cdr::xsd::Schema* schema = 0;
    if (!schemaStr.empty()) {
        cdr::dom::Parser parser;
        parser.parse(schemaStr);
        cdr::dom::Node schemaElem = parser.getDocument().getDocumentElement();
        schema = new cdr::xsd::Schema(schemaElem, &conn);
    }
    std::auto_ptr<cdr::xsd::Schema> schemaPtr(schema);
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
         << (schema ? schema->makeDtd(schemaName) : L"")
         << L"]]></DocDtd><DocSchema>"
         << schemaName
         << L"</DocSchema>";
    if (schema && getEnumValues == L"Y") {
        cdr::xsd::Element& topElem = schema->getTopElement();
        cdr::xsd::ValidValueSets validValueSets;
        schema->getValidValueSets(validValueSets);
        cdr::xsd::ValidValueSets::const_iterator i = validValueSets.begin();
        while (i != validValueSets.end()) {
            const cdr::StringSet* validValues = i->second;
            resp << L"<EnumSet Node='" << i->first << L"'>";
            cdr::StringSet::const_iterator j = validValues->begin();
            while (j != validValues->end())
                resp << L"<ValidValue>" << *j++ << L"</ValidValue>";
            resp << L"</EnumSet>";
            ++i;
        }
    }
    if (!linkingElements.empty()) {
        resp << L"<LinkingElements>";
        cdr::StringList::const_iterator iter = linkingElements.begin();
        while (iter != linkingElements.end())
            resp << L"<LinkingElement>" << *iter++ << L"</LinkingElement>";
        resp << L"</LinkingElements>";
    }
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
    cdr::String schema(true);
    cdr::String comment(true);  // default to NULL
    cdr::dom::Node child = node.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"DocSchema")
                schema = cdr::dom::getTextContent(child);
            else if (name == L"Comment")
                comment = cdr::dom::getTextContent(child);
        }
        child = child.getNextSibling();
    }

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
    
    // Look up the schema document's primary key.
    if (schema.empty())
        throw cdr::Exception(L"DocSchema element missing from command");
    std::string q3 = "SELECT d.id                "
                     "  FROM document d          "
                     "  JOIN doc_type t          "
                     "    ON t.id    = d.doc_type"
                     " WHERE t.name  = 'schema'  "
                     "   AND d.title = ?         ";
    cdr::db::PreparedStatement ps3 = conn.prepareStatement(q3);
    ps3.setString(1, schema);
    cdr::db::ResultSet rs3 = ps3.executeQuery();
    if (!rs3.next())
        throw cdr::Exception(L"Schema document not found", schema);
    int schemaId = rs3.getInt(1);
    ps3.close();

    // Create the new document type.
    std::string q4 = "INSERT INTO doc_type"
                     "(                   "
                     "    name,           "
                     "    format,         "
                     "    created,        "
                     "    versioning,     "
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
                     "    ?,              "
                     "    GETDATE(),      "
                     "    ?               "
                     ")                   ";
    cdr::db::PreparedStatement ps4 = conn.prepareStatement(q4);
    ps4.setString(1, docTypeString);
    ps4.setInt   (2, formatId);
    ps4.setString(3, versioningString);
    ps4.setInt   (4, schemaId);
    ps4.setString(5, comment);
    if (ps4.executeUpdate() != 1)
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

    // Make sure the user is authorized to modify an existing document type.
    if (!session.canDo(conn, L"MODIFY DOCTYPE", docTypeString))
        throw cdr::Exception(
                L"MODIFY DOCTYPE action not authorized for this user");

    // Extract the schema.
    cdr::String schema;
    cdr::String comment(true);  // default to NULL
    cdr::dom::Node child = node.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"DocSchema")
                schema = cdr::dom::getTextContent(child);
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

    // Look up the schema document's primary key.
    std::string q3 = "SELECT d.id                "
                     "  FROM document d          "
                     "  JOIN doc_type t          "
                     "    ON t.id    = d.doc_type"
                     " WHERE t.name  = 'schema'  "
                     "   AND d.title = ?         ";
    cdr::db::PreparedStatement ps3 = conn.prepareStatement(q3);
    ps3.setString(1, schema);
    cdr::db::ResultSet rs3 = ps3.executeQuery();
    if (!rs3.next())
        throw cdr::Exception(L"Schema document not found", schema);
    int schemaId = rs3.getInt(1);
    ps3.close();

    // Create the new document type.
    std::string q4 = "UPDATE doc_type                "
                     "   SET format      = ?,        "
                     "       versioning  = ?,        "
                     "       xml_schema  = ?,        "
                     "       schema_date = GETDATE(),"
                     "       comment     = ?         "
                     " WHERE id          = ?         ";
    cdr::db::PreparedStatement ps4 = conn.prepareStatement(q4);
    ps4.setInt   (1, formatId);
    ps4.setString(2, versioningString);
    ps4.setInt   (3, schemaId);
    ps4.setString(4, comment);
    ps4.setInt   (5, docTypeId);
    if (ps4.executeUpdate() != 1)
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

/**
 * Send the complete set of CSS stylesheets to the client.
 *
 *  @param      session     contains information about the current user.
 *  @param      node        contains the XML for the command.
 *  @param      conn        reference to the connection object for the
 *                          CDR database.
 *  @return                 String object containing the XML for the
 *                          command response.
 *  @exception  cdr::Exception if a database or processing error occurs.
 */
cdr::String cdr::getCssFiles(Session&          session,
                             const dom::Node&  node,
                             db::Connection&   conn)
{
    // Submit the query to the database
    cdr::db::Statement s = conn.createStatement();
    cdr::db::ResultSet r = s.executeQuery(" SELECT d.title, b.data     "
                                          "   FROM document d,         "
                                          "        doc_blob b,         "
                                          "        doc_type t          "
                                          "  WHERE t.name = 'css'      "
                                          "    AND t.id   = d.doc_type "
                                          "    AND d.id   = b.id       ");

    // Construct the response.
    cdr::String resp = L"<CdrGetCssFiles>";
    while (r.next()) {
        cdr::String title = r.getString(1);
        cdr::Blob   blob  = r.getBytes(2);

        // Preserve Latin-1 encoding; these files aren't using utf-8.
        cdr::String css(blob.size(), L'\0');
        std::copy(blob.begin(), blob.end(), css.begin());
        resp += L"<File><Name>" + title + L"</Name><Data>"
                                + css   + L"</Data></File>";
    }

    // Send back the set.
    return resp + L"</CdrGetCssFiles>";
}
