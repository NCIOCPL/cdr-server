/*
 * $Id: CdrMergeProt.cpp,v 1.3 2002-08-23 20:28:52 bkline Exp $
 *
 * Merge scientific protocol information into main in-scope protocol document.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2002/04/09 12:51:53  bkline
 * Added ProtocolTitle to scientific element list.
 *
 * Revision 1.1  2001/11/28 19:44:38  bkline
 * Initial revision
 *
 */

// Eliminate annoying warnings about truncated debugging information.
#pragma warning(disable : 4786)

#include <iostream>
#include <sstream>
#include "CdrDom.h"
#include "CdrCommand.h"
#include "CdrException.h"
#include "CdrDbResultSet.h"
#include "CdrDbPreparedStatement.h"
#include "CdrXsd.h"

// Local functions.
static cdr::String getDocTypeName(
        const cdr::String&,
        cdr::db::Connection&);
static cdr::StringList getTargElemSequence(
        cdr::db::Connection&);
static cdr::StringSet getScientificElemNames();
static void getElementNames(
        cdr::StringList&, 
        const cdr::xsd::Node*);
static cdr::dom::Element checkOut(
        const cdr::String&, 
        cdr::Session&,
        cdr::db::Connection&);
static void checkIn(
        const cdr::String&, 
        const cdr::String&, 
        const cdr::String&, 
        const cdr::String&,
        cdr::Session&,
        cdr::db::Connection&);
static void deleteDoc(
        const cdr::String&, 
        const cdr::String&,
        cdr::Session&,
        cdr::db::Connection&);
static void checkForErrorResponse(
        const cdr::String&, 
        const cdr::String&);
static bool goesBefore(
        const cdr::String&, 
        const cdr::String&, 
        const cdr::StringList&);

/**
 * Fold ScientificProtocolInfo document into InScopeProtocol document.
 */
cdr::String cdr::mergeProt(Session& session,
                           const dom::Node& commandNode,
                           db::Connection& conn)
{
    // Extract command arguments
    String sourceDocIdStr;
    String targetDocIdStr;
    dom::Node child = commandNode.getFirstChild();
    while (child != NULL) {
        if (child.getNodeType() == dom::Node::ELEMENT_NODE) {
            String name = child.getNodeName();
            if (name == L"SourceDoc")
                sourceDocIdStr = dom::getTextContent(child);
            else if (name == L"TargetDoc")
                targetDocIdStr = dom::getTextContent(child);
        }
        child = child.getNextSibling();
    }

    // Make sure we got everything.
    if (sourceDocIdStr.empty() || targetDocIdStr.empty())
        throw Exception(L"SourceDoc and TargetDoc elements both required");

    // Make sure we got the right document types.
    String docTypeString = getDocTypeName(sourceDocIdStr, conn);
    if (docTypeString != L"ScientificProtocolInfo")
        throw Exception(L"Source document has type", docTypeString);
    docTypeString = getDocTypeName(targetDocIdStr, conn);
    if (docTypeString != L"InScopeProtocol")
        throw Exception(L"Source document has type", docTypeString);

    // Find out the order of the InScopeProtocol element's children.
    cdr::StringList elemSequence = getTargElemSequence(conn);

    // Find out which elements belong to the ScientificProtocolInfo doc.
    cdr::StringSet scientificElems = getScientificElemNames();

    // Everything we do from this point needs to be atomic.
    conn.setAutoCommit(false);

    // Check out the two documents.
    dom::Element sourceDoc = checkOut(sourceDocIdStr, session, conn);
    if (sourceDoc == NULL)
        throw Exception(L"Unable to check out source document",
                sourceDocIdStr);
    dom::Element targetDoc = checkOut(targetDocIdStr, session, conn);
    if (targetDoc == NULL)
        throw Exception(L"Unable to check out target document",
                targetDocIdStr);

    // Find the start of the child nodes of the top elements.
    dom::Node sourceChild = sourceDoc.getFirstChild();
    dom::Node targetChild = targetDoc.getFirstChild();

    // Start a new document.
    String topElementName = targetDoc.getNodeName();
    std::wostringstream os;
    os << L"<" << topElementName;
    cdr::dom::NamedNodeMap attributes = targetDoc.getAttributes();
    int nAttributes = attributes.getLength();
    for (int i = 0; i < nAttributes; ++i) {
        cdr::dom::Node attribute = attributes.item(i);
        cdr::String attrValue = attribute.getNodeValue();
        cdr::String attrName  = attribute.getNodeName();
        size_t quot = attrValue.find(L"\"");
        while (quot != attrValue.npos) {
            attrValue.replace(quot, 1, L"&quot;");
            quot = attrValue.find(L"\"", quot);
        }
        os << L" " 
           << attrName
           << L" = \""
           << attrValue
           << L"\"";
    }
    os << L">";

    // Main loop through target chain.
    while (targetChild != NULL) {

        // If this isn't an element node, copy it and move on.
        if (targetChild.getNodeType() != dom::Node::ELEMENT_NODE) {
            os << targetChild;
            targetChild = targetChild.getNextSibling();
            continue;
        }

        // Find out the element's name.
        String targetName = targetChild.getNodeName();

        // Is this an element owned by the ScientificProtocolInfo doc?
        if (scientificElems.find(targetName) != scientificElems.end()) {

            // Yes.  Drop it.
            targetChild = targetChild.getNextSibling();
            continue;
        }

        // Insert any scientific elements which go in front of this element.
        while (sourceChild != NULL) {

            // If this isn't an element node, skip past it.
            if (sourceChild.getNodeType() != dom::Node::ELEMENT_NODE) {
                sourceChild = sourceChild.getNextSibling();
                continue;
            }

            // Find out the element's name.
            String sourceName = sourceChild.getNodeName();

            // If we hit an element whose time has not yet come ...
            if (!goesBefore(sourceName, targetName, elemSequence)) {

                // Stop; we'll wait until we move on in the target chain.
                break;
            }

            // If this is a scientific element, keep it.
            if (scientificElems.find(sourceName) != scientificElems.end()) {
                os << sourceChild;
            }

            // Move to the next source node.
            sourceChild = sourceChild.getNextSibling();
        }

        // Copy node and move to the next node in the target chain.
        os << targetChild;
        targetChild = targetChild.getNextSibling();
    }

    // We're past the existing nodes in the target doc; anything left over?
    while (sourceChild != NULL) {

        // If this is a scientific element, keep it.
        if (sourceChild.getNodeType() == dom::Node::ELEMENT_NODE) {
            String sourceName = sourceChild.getNodeName();
            if (scientificElems.find(sourceName) != scientificElems.end())
                //targetDoc.appendChild(sourceChild);
                os << sourceChild;
        }

        // Move to the next node.
        sourceChild = sourceChild.getNextSibling();
    }

    // The source goes away ...
    deleteDoc(sourceDocIdStr, targetDocIdStr, session, conn);

    // ... and the target gets saved with the new elements.
    os << L"</" << topElementName << L">";
    String docString = os.str();
    checkIn(targetDocIdStr, sourceDocIdStr, docString, 
            L"InScopeProtocol", session, conn);

    // Report success.
    return L"<CdrMergeProt/>";
}

cdr::String getDocTypeName(const cdr::String& docId, cdr::db::Connection& conn)
{
    // Look in the database for the document type.
    const static char* query =
        " SELECT name "
        "   FROM doc_type "
        "   JOIN document "
        "     ON document.doc_type = doc_type.id "
        "  WHERE document.id = ?";
    cdr::db::PreparedStatement stmt = conn.prepareStatement(query);
    stmt.setInt(1, docId.extractDocId());
    cdr::db::ResultSet rslt = stmt.executeQuery();
    if (!rslt.next())
        throw cdr::Exception(L"Cannot find document type for document", docId);
    return rslt.getString(1);
}

cdr::StringList getTargElemSequence(cdr::db::Connection& conn)
{
    // Get the schema for InScopeProtocol documents.
    const static char* query =
        " SELECT xml                                 "
        "   FROM document                            "
        "   JOIN doc_type                            "
        "     ON document.id   = doc_type.xml_schema "
        "  WHERE doc_type.name = 'InScopeProtocol'   ";  
    cdr::db::Statement stmt = conn.createStatement();
    cdr::db::ResultSet rslt = stmt.executeQuery(query);
    if (!rslt.next())
        throw cdr::Exception(L"Unable to find InScopeProtocol Schema");
    cdr::String schemaXml = rslt.getString(1);
    stmt.close();

    // Parse it.
    cdr::dom::Parser parser;
    parser.parse(schemaXml);
    cdr::dom::Element docElem = parser.getDocument().getDocumentElement();
    
    // Construct the schema.
    cdr::xsd::Schema schema(docElem, &conn);

    // Get the complex type for the top-level InScopeProtocol element.
    cdr::xsd::Element topElement = schema.getTopElement();
    const cdr::xsd::Type* topElemType = topElement.getType(schema);
    const cdr::xsd::ComplexType* complexType = 
        dynamic_cast<const cdr::xsd::ComplexType*>(topElemType);
    if (!complexType)
        throw cdr::Exception(L"Unable to find InScopeProtocol complex type");

    // Build up the list of direct child elements.
    cdr::StringList elemList;
    const cdr::xsd::Node* content = complexType->getContent();
    getElementNames(elemList, content);
    return elemList;
}

void getElementNames(cdr::StringList& elemList, const cdr::xsd::Node* node)
{
    // Reality check.
    if (node == 0)
        return;

    // Find out what kind of node we have.
    const cdr::xsd::Element* e = dynamic_cast<const cdr::xsd::Element*>(node);
    const cdr::xsd::Group* g = dynamic_cast<const cdr::xsd::Group*>(node);
    const cdr::xsd::ChoiceOrSequence* cs = 
        dynamic_cast<const cdr::xsd::ChoiceOrSequence*>(node);

    // Is this schema node for an element?
    if (e)
        elemList.push_back(e->getName());

    // Is this schema node for a sequence or a choice?
    else if (cs) {
        cdr::xsd::NodeEnum nodeEnum = cs->getNodes();
        while (nodeEnum != cs->getListEnd())
            getElementNames(elemList, *nodeEnum++);
    }

    // Is this schema node for a named group?
    else if (g)
        getElementNames(elemList, g->getContent());

    // Mayday!
    else
        throw cdr::Exception(L"Internal error: unexpected schema node type!");
}

/**
 * Builds a set containing the names of the elements owned by the
 * ScientificProtocolInfo document type.
 */
cdr::StringSet getScientificElemNames()
{
    static const wchar_t* elemNames[] = {
        L"ProtocolTitle",
        L"ProtocolAmendmentInformation",
        L"ProtocolAbstract",
        L"ProtocolDetail",
        L"Eligibility",
        L"ProtocolRelatedLinks",
        L"PublishedResults",
        L"ProtocolPhase",
        L"ProtocolDesign"
    };
    cdr::StringSet nameSet;
    for (size_t i = 0; i < sizeof elemNames / sizeof *elemNames; ++i)
        nameSet.insert(elemNames[i]);
    return nameSet;
}

bool goesBefore(const cdr::String& thisElement, 
                const cdr::String& thatElement, 
                const cdr::StringList& elemSequence)
{
    cdr::StringList::const_iterator listIter = elemSequence.begin();
    while (listIter != elemSequence.end()) {
        if (thisElement == *listIter)
            return true;
        else if (thatElement == *listIter)
            return false;
        ++listIter;
    }
    return false;
}

void deleteDoc(const cdr::String& sourceId, 
               const cdr::String& targetId,
               cdr::Session& session,
               cdr::db::Connection& conn)
{
    cdr::String cmd = L"<CdrDelDoc><DocId>"
                    + sourceId
                    + L"</DocId><Validate>N</Validate><Reason>Merged with "
                    + targetId
                    + L".</Reason></CdrDelDoc>";
    cdr::dom::Parser parser;
    parser.parse(cmd);
    cdr::dom::Element cmdNode = parser.getDocument().getDocumentElement();
    cdr::String rsp = cdr::delDoc(session, cmdNode, conn);
    if (session.getStatus() == L"error")
        checkForErrorResponse(rsp, L"deleteDoc()");
}

void checkForErrorResponse(const cdr::String& rsp, const cdr::String& where)
{
    size_t pos = rsp.find(L"<Errors>");
    if (pos == rsp.npos)
        return;
    pos = rsp.find(L"<Err", pos + 1);
    if (pos == rsp.npos)
        throw cdr::Exception(where, L"Undetermined error");
    pos = rsp.find(L">", pos);
    if (pos == rsp.npos)
        throw cdr::Exception(where, L"Undetermined error");
    size_t endPos = rsp.find(L"<", pos++);
    if (endPos > pos) {
        cdr::String err = rsp.substr(pos, endPos - pos);
        throw cdr::Exception(where, err);
    }
    throw cdr::Exception(where, L"Undetermined error");
}

cdr::dom::Element checkOut(
        const cdr::String& docId,
        cdr::Session& session,
        cdr::db::Connection& conn)
{
    // Delimiter marking start of CDATA section.
    static const cdr::String cdataStart(L"<![CDATA[");

    // Build the command.
    cdr::String cmd = L"<CdrGetDoc><DocId>"
                    + docId
                    + L"</DocId><Lock>Y</Lock><Version>Current</Version>"
                      L"</CdrGetDoc>";
    cdr::dom::Parser parser;
    parser.parse(cmd);
    cdr::dom::Element cmdNode = parser.getDocument().getDocumentElement();

    // Execute the command.
    cdr::String rsp = cdr::getDoc(session, cmdNode, conn);
    size_t pos = rsp.find(L"<CdrDocXml");
    if (pos == rsp.npos) {
        if (session.getStatus() == L"error") {
            if (rsp.find(L"<Errors>") == rsp.npos)
                throw cdr::Exception(L"CheckOut failure", docId);
            else
                checkForErrorResponse(rsp, L"CheckOut");
        }
    }

    // Extract the XML for the document.
    pos = rsp.find(cdataStart);
    if (pos == rsp.npos)
        throw cdr::Exception(L"CDATA section missing checking out", docId);
    pos += cdataStart.size();
    size_t endPos = rsp.find(L"]]>");
    if (endPos == rsp.npos)
        throw cdr::Exception(L"Malformed response", rsp);
    cdr::String docXml = rsp.substr(pos, endPos - pos);
    parser.parse(docXml);
    return parser.getDocument().getDocumentElement();
}

void checkIn(
        const cdr::String& docId, 
        const cdr::String& otherDocId,
        const cdr::String& docString, 
        const cdr::String& docType,
        cdr::Session& session,
        cdr::db::Connection& conn)
{
    // Build the CdrDoc element.
    cdr::String cdrDoc = L"<CdrDoc Type='"
                       + docType
                       + L"' Id='"
                       + docId
                       + L"'><CdrDocXml><![CDATA["
                       + docString
                       + L"]]></CdrDocXml></CdrDoc>";

    // Build the command.
    cdr::String cmd = L"<CdrRepDoc><CheckIn>Y</CheckIn><Validate>N</Validate>"
                      L"<Reason>Absorbed ScientificProtocolInfo document "
                    + otherDocId
                    + L".</Reason><Version>N</Version>"
                    + cdrDoc
                    + L"</CdrRepDoc>";
    cdr::dom::Parser parser;
    parser.parse(cmd);
    cdr::dom::Element cmdNode = parser.getDocument().getDocumentElement();

    // Execute it.
    cdr::String rsp = cdr::repDoc(session, cmdNode, conn);
    if (session.getStatus() == L"error")
        checkForErrorResponse(rsp, L"checkIn");
}
