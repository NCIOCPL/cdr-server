/*
 * $Id: CdrSearch.cpp,v 1.4 2001-03-02 13:59:57 bkline Exp $
 *
 * Queries the CDR to create subset list of documents.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2000/10/04 18:28:26  bkline
 * Fixed comment typo; added support for MaxDocs attribute; implemented
 * command to search for link target candidates.
 *
 * Revision 1.2  2000/05/03 15:25:41  bkline
 * Fixed database statement creation.
 *
 * Revision 1.1  2000/04/21 13:52:58  bkline
 * Initial revision
 *
 */

#include <iostream>
#include "CdrSearch.h"
#include "CdrException.h"
#include "CdrCommand.h"
#include "CdrLink.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"
#include "CdrParserInput.h"

// Remember the parameters so we can set them after prepareStatement().
struct SearchParam {
    SearchParam(const cdr::String& p) : t(STRING), s(p) {}
    SearchParam(int p) : t(INT), i(p) {}
    enum Type { STRING, INT };
    Type        t;
    cdr::String s;
    int         i;
};
typedef std::vector<SearchParam> SearchParams;

static std::string extractParams(const cdr::String&, SearchParams&);

cdr::String cdr::search(cdr::Session& session, 
                        const cdr::dom::Node& node, 
                        cdr::db::Connection& conn)
{
    // Extract the query from the command buffer.
    cdr::String queryString;
    int maxRows = 0;
    cdr::dom::Node child = node.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name != L"Query")
                throw cdr::Exception(L"Unexpected element", name);
            queryString = cdr::dom::getTextContent(child);
            const cdr::dom::Element& docElement =
                        static_cast<const cdr::dom::Element&>(child);
            cdr::String attr = docElement.getAttribute (L"MaxDocs");
            if (!attr.empty())
                maxRows = wcstoul(attr.c_str(), 0, 10);
        }
        child = child.getNextSibling();
    }

    // Parse it
    cdr::ParserInput input(queryString);
    cdr::Query query;
    cdr::QueryParam qp(&query, &input);
    std::wcout << queryString << L"\n";
    try {
        CdrSearchparse(static_cast<void*>(&qp));
    }
    catch (cdr::Exception e) { 
        std::wcout << L"LAST TOKEN: " << input.getLastTok() << std::endl;
        throw;
    }
    cdr::String sql = query.getSql(maxRows);

    // Submit the query to the DBMS.
    SearchParams params;
    std::string qString = extractParams(sql, params);
    cdr::db::PreparedStatement select = conn.prepareStatement(qString);
    for (size_t i = 0; i < params.size(); ++i) {
        switch (params[i].t) {
        case SearchParam::STRING:
            select.setString(i + 1, params[i].s);
            break;
        case SearchParam::INT:
            select.setInt(i + 1, params[i].i);
            break;
        default:
            throw cdr::Exception(L"INTERNAL ERROR: Unknown SearchParam type");
        }
    }
    cdr::db::ResultSet rs = select.executeQuery();

    // Construct the response.
    cdr::String response = L"  <CdrSearchResp>\n";
    
    int rows = 0;
    while (rs.next()) {
        if (rows++ == 0)
            response += L"   <QueryResults>\n";
        int         id      = rs.getInt(1);
        cdr::String title   = cdr::entConvert(rs.getString(2));
        cdr::String docType = rs.getString(3);
        wchar_t tmp[1000];
        swprintf(tmp, L"    <QueryResult>\n     <DocId>CDR%010ld</DocId>\n"
                      L"     <DocType>%s</DocType>\n"
                      L"     <DocTitle>%.500s</DocTitle>\n"
                      L"    </QueryResult>\n", 
                 id, docType.c_str(), title.c_str());
        response += tmp;
    }
    if (rows > 0)
        response += L"   </QueryResults>\n  </CdrSearchResp>\n";
    else
        response += L"   <QueryResults/>\n  </CdrSearchResp>\n";
    return response;
}

std::string extractParams(const cdr::String& sql, SearchParams& params)
{
    size_t            len = sql.size();    // Characters in original string
    std::string       str(len, ' ');       // Work area for narrow string
    size_t            i = 0, j = 0;        // Indices into sql and str

    // Walk through all characters in original sql string.
    while (i < len) {

        // Extract string value.
        if (sql[i] == cdr::QueryNode::STRING_MARK) {
            size_t startPos = ++i;
            int n = 0;
            bool foundDelim = false;
            while (!foundDelim && i < len) {
                if (sql[i] == cdr::QueryNode::STRING_MARK) {
                    cdr::String param = sql.substr(startPos, n);
                    params.push_back(SearchParam(param));
                    str[j++] = '?';
                    foundDelim = true;
                }
				++n;
                ++i;
            }
            if (!foundDelim)
                throw cdr::Exception(L"Unpaired string delimiter", sql);
        }

        // Extract integer value.
        else if (sql[i] == cdr::QueryNode::INT_MARK) {
            size_t startPos = ++i;
            int n = 0;
            bool foundDelim = false;
            while (!foundDelim && i < len) {
                if (sql[i] == cdr::QueryNode::INT_MARK) {
                    cdr::String param = sql.substr(startPos, n);
                    params.push_back(SearchParam(param.getInt()));
                    str[j++] = '?';
                    foundDelim = true;
                }
                ++i;
				++n;
            }
            if (!foundDelim)
                throw cdr::Exception(L"Unpaired integer delimiter", sql);
        }

        // Not a delimiter.
        else
            str[j++] = sql[i++];
    }

    str.resize(j);
    std::cout << str << '\n';
    return str;
}

cdr::String cdr::QueryNode::markString(const cdr::String& s) const
{
    if (op == CONTAINS)
        return cdr::String(1, STRING_MARK) + L"%" + s + L"%"
            + cdr::String(1, STRING_MARK);
    else if (op == BEGINS)
        return cdr::String(1, STRING_MARK) + s + L"%"
            + cdr::String(1, STRING_MARK);
    else
        return cdr::String(1, STRING_MARK) + s + cdr::String(1, STRING_MARK);
}

cdr::String cdr::QueryNode::markInt(const cdr::String& s) const
{
    return cdr::String(1, INT_MARK) + s + cdr::String(1, STRING_MARK);
}

cdr::String cdr::QueryNode::getOpString() const
{
    switch (op) {
    case EQ:        return L" = ";
    case NE:        return L" != ";
    case GT:        return L" > ";
    case LT:        return L" < ";
    case GTE:       return L" >= ";
    case LTE:       return L" <= ";
    case BEGINS:
    case CONTAINS:  return L" LIKE ";
    default:
        throw cdr::Exception(L"QueryNode: invalid op code");
    }
}

cdr::String cdr::QueryNode::strip(const cdr::String& docId) const
{
    size_t n = 0;
    while (docId.size() > n && !wcschr(L"0123456789", docId[n]))
        ++n;
    return docId.substr(n);
}

cdr::String cdr::QueryNode::getSql() const
{
    switch (nodeType) {
    case NEGATION:
        return cdr::String(L"NOT(") + right->getSql() + L")";
    case BOOLEAN:
        if (op == AND)
            return cdr::String(L"(") + left->getSql() 
                                     + L") AND (" 
                                     + right->getSql()
                                     + L")";
        else if (op == OR)
            return cdr::String(L"(") + left->getSql() 
                                     + L") OR (" 
                                     + right->getSql()
                                     + L")";
        else
            throw cdr::Exception(L"QueryNode: invalid boolean op");
    case ASSERTION:
        switch (lValueType) {
        case ATTR:
            return cdr::String(L"attr.name = ")        + markString(lValueName)
                                                       + L" AND attr.val"
                                                       + getOpString() 
                                                       + markString(rValue);
        case DOC_ID:
            return cdr::String(L"document.id")         + getOpString() 
                                                       + markInt(strip(rValue));
        case CREATOR:
            return cdr::String(L"creator.name")        + getOpString()
                                                       + markString(rValue);
        case CREATED:
            return cdr::String(L"document.created")    + getOpString()
                                                       + markString(rValue);
        case VAL_STATUS:
            return cdr::String(L"document.val_status") + getOpString()
                                                       + markString(rValue);
        case VAL_DATE:
            return cdr::String(L"document.val_date")   + getOpString()
                                                       + markString(rValue);
        case APPROVED:
            return cdr::String(L"document.approved")   + getOpString()
                                                       + markString(rValue);
        case DOC_TYPE:
            return cdr::String(L"doc_type.name")       + getOpString()
                                                       + markString(rValue);
        case TITLE:
            return cdr::String(L"document.title")      + getOpString()
                                                       + markString(rValue);
        case MODIFIED:
            return cdr::String(L"document.modified")   + getOpString()
                                                       + markString(rValue);
        case MODIFIER:
            return cdr::String(L"modifier.name")       + getOpString()
                                                       + markString(rValue);
        default:
            throw cdr::Exception(L"QueryNode: invalid control name");
        }
    default:
        throw cdr::Exception(L"QueryNode: invalid node type");
    }
}

cdr::Query::Query()
{
    tree            = 0;
    hasCreatorTest  = false;
    hasModifierTest = false;
    hasAttrTest     = false;
    hasDocTypeTest  = false;
}

cdr::String cdr::Query::getSql(int maxDocs)
{
    cdr::String sql = L"SELECT ";
    if (maxDocs > 0) {
        wchar_t tBuf[40];
        swprintf(tBuf, L"TOP %d ", maxDocs);
        sql += tBuf;
    }
    sql += "document.id, document.title, doc_type.name FROM document";
    if (tree) {
        checkTablesJoined(tree);
        cdr::String where = L" WHERE ";
        if (hasAttrTest) {
            sql += L", attr";
            where += L"document.id = attr.id AND ";
        }
        if (hasCreatorTest) {
            sql += L", usr AS creator";
            where += L"document.creator = creator.id AND ";
        }
        if (hasModifierTest) {
            sql += L", usr AS modifier";
            where += L"document.modifier = modifier.id AND ";
        }
        /* if (hasDocTypeTest) { */
            sql += L", doc_type";
            where += L"document.doc_type = doc_type.id AND ";
        /* } */
        sql += where + L"(" + tree->getSql() + L")";
    }
    return sql;
}

void cdr::Query::checkTablesJoined(const cdr::QueryNode* n)
{
    if (n) {
        switch (n->getNodeType()) {
        case cdr::QueryNode::NEGATION:
            checkTablesJoined(n->getRightNode());
            break;
        case cdr::QueryNode::BOOLEAN:
            checkTablesJoined(n->getLeftNode());
            checkTablesJoined(n->getRightNode());
            break;
        case cdr::QueryNode::ASSERTION:
            switch (n->getLValueType()) {
            case cdr::QueryNode::ATTR:
                hasAttrTest = true;
                break;
            case cdr::QueryNode::CREATOR:
                hasCreatorTest = true;
                break;
            case cdr::QueryNode::MODIFIER:
                hasModifierTest = true;
                break;
            case cdr::QueryNode::DOC_TYPE:
                hasDocTypeTest = true;
                break;
            }
            break;
        default:
            throw cdr::Exception(L"Query: invalid node type");
        }
    }
}

void cdr::Query::freeNodes(cdr::QueryNode* n)
{
    if (n) {
        freeNodes(n->getLeftNode());
        freeNodes(n->getRightNode());
        delete n;
    }
}

cdr::String cdr::searchLinks(cdr::Session& session, 
                             const cdr::dom::Node& node, 
                             cdr::db::Connection& conn)
{
    // See if the client has requested a cap on the number of rows.
    int maxRows = 0;
    const cdr::dom::Element& cmdElement =
                static_cast<const cdr::dom::Element&>(node);
    cdr::String maxDocsAttr = cmdElement.getAttribute(L"MaxDocs");
    if (!maxDocsAttr.empty())
        maxRows = wcstoul(maxDocsAttr.c_str(), 0, 10);

    // Extract the parameters for the search.
    cdr::String docType;
    cdr::String elemName;
    cdr::String titlePattern;
    cdr::dom::Node child = node.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"SourceDocType")
                docType = cdr::dom::getTextContent(child);
            else if (name == L"SourceElementType")
                elemName = cdr::dom::getTextContent(child);
            else if (name == L"TargetTitlePattern")
                titlePattern = cdr::dom::getTextContent(child);
        }
        child = child.getNextSibling();
    }

    // Make sure we got everything we need.
    if (docType.empty())
        throw cdr::Exception(L"Missing required SourceDocType element");
    else if (elemName.empty())
        throw cdr::Exception(L"Missing required SourceElementType element");
    else if (titlePattern.empty())
        throw cdr::Exception(L"Missing required TargetTitlePattern element");

    // Find out which link target document types are permissible.
    std::vector<int> targetDocTypes;
    cdr::link::findTargetDocTypes(conn, elemName, docType, targetDocTypes);
    if (targetDocTypes.size() < 1)
        throw cdr::Exception(L"No links permitted from this element");

    // Construct the query.
    std::string qry = "SELECT ";
    if (maxRows > 0) {
        char buf[40];
        sprintf(buf, "TOP %d ", maxRows);
        qry += buf;
    }
    qry += "id, title FROM document WHERE title LIKE ?";
    if (targetDocTypes.size() == 1)
        qry += " AND doc_type = ?";
    else {
        qry += " AND doc_type IN (";
        const char* placeHolder = "?";
        for (size_t i = 0; i < targetDocTypes.size(); ++i) {
            qry += placeHolder;
            placeHolder = ",?";
        }
        qry += ")";
    }
    qry += " ORDER BY id";

    // Submit the query to the DBMS.
    cdr::db::PreparedStatement stmt = conn.prepareStatement(qry);
    int pos = 1;
    stmt.setString(pos++, titlePattern);
    for (size_t i = 0; i < targetDocTypes.size(); ++i)
        stmt.setInt(pos++, targetDocTypes[i]);
    cdr::db::ResultSet rs = stmt.executeQuery();

    // Construct the response.
    cdr::String response = L"<CdrSearchLinksResp>";
    int rows = 0;
    while (rs.next()) {
        if (rows++ == 0)
            response += L"<QueryResults>";
        int         id      = rs.getInt(1);
        cdr::String title   = rs.getString(2);
        wchar_t tmp[1000];
        swprintf(tmp, L"<QueryResult><DocId>CDR%010ld</DocId>"
                      L"<DocTitle>%.500s</DocTitle>"
                      L"</QueryResult>", 
                 id, title.c_str());
        response += tmp;
    }
    if (rows > 0)
        response += L"</QueryResults></CdrSearchLinksResp>";
    else
        response += L"<QueryResults/></CdrSearchLinksResp>";
    return response;
}
