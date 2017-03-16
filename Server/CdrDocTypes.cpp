/*
 * Support routines for CDR document types.
 *
 * JIRA::OCECDR-4091 - allow CdrModDocType command to alter 'active' column
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
        response += L"<DocTitle>" + cdr::entConvert(name) + L"</DocTitle>";
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
    cdr::String omitDtdString = cmdElement.getAttribute(L"OmitDtd");
    bool omitDtd = omitDtdString == L"Y";
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
                        "                 t.id,               "
                        "                 t.active            "
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
    cdr::String active     = rs.getString(9);
    ps.close();

    // Create a parser and, if necessary, a schema
    // Parser is created here to stay in scope until the schema that uses
    //   its dependent DOM tree is out of scope - even though it may turn
    //   out that we don't need to parse at all.
    cdr::dom::Parser parser;
    cdr::xsd::Schema* schema = 0;
    if (!schemaStr.empty()) {
        parser.parse(schemaStr);
        cdr::dom::Node schemaElem = parser.getDocument().getDocumentElement();
        schema = new cdr::xsd::Schema(schemaElem, &conn);
    }
    std::auto_ptr<cdr::xsd::Schema> schemaPtr(schema);
    cdr::StringList linkingElements;
    if (schema)
        schema->elemsWithAttr(L"cdr:ref", linkingElements);
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
         << L"' Active='"
         << active
         << L"'>";
    if (!omitDtd)
        resp << L"<DocDtd><![CDATA["
             << (schema ? schema->makeDtd(schemaName) : L"")
             << L"]]></DocDtd>";
    resp << L"<DocSchema>"
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
                resp << L"<ValidValue>" << cdr::entConvert(*j++)
                     << L"</ValidValue>";
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
        resp << L"<Comment>" << cdr::entConvert(comment) << L"</Comment>";
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
    cdr::String activeString     = cmdElement.getAttribute(L"Active");
    if (docTypeString.empty())
        throw cdr::Exception(L"Type attribute missing from CdrModDocType "
                             L"command element");
    if (docFormatString.empty())
        docFormatString = L"xml";
    if (versioningString.empty())
        versioningString = L"Y";
    if (versioningString != L"Y" && versioningString != L"N")
        throw cdr::Exception(L"Versioning attribute can only be Y or N; was ",
                             versioningString);
    if (!activeString.empty()) {
        if (activeString != L"Y" && activeString != L"N")
            throw cdr::Exception(L"Active attribute can only be Y or N; was ",
                                 activeString);
    }

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
    // OCECDR-4091: also grab the current value of the doc_type.active column.
    std::string q3 = "SELECT d.id, t.active      "
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
    cdr::String currentActiveFlag = rs3.getString(2);
    if (activeString.empty())
        activeString = currentActiveFlag;
    ps3.close();

    // Create the new document type.
    std::string q4 = "UPDATE doc_type                "
                     "   SET format      = ?,        "
                     "       versioning  = ?,        "
                     "       xml_schema  = ?,        "
                     "       schema_date = GETDATE(),"
                     "       comment     = ?,        "
                     "       active      = ?         "
                     " WHERE id          = ?         ";
    cdr::db::PreparedStatement ps4 = conn.prepareStatement(q4);
    ps4.setInt   (1, formatId);
    ps4.setString(2, versioningString);
    ps4.setInt   (3, schemaId);
    ps4.setString(4, comment);
    ps4.setString(5, activeString);
    ps4.setInt   (6, docTypeId);
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
        resp += L"<File><Name>" + cdr::entConvert(title) + L"</Name><Data>"
                                + cdr::entConvert(css)   + L"</Data></File>";
    }

    // Send back the set.
    return resp + L"</CdrGetCssFiles>";
}
