/*
 * $Id: CdrSearch.cpp,v 1.1 2000-04-21 13:52:58 bkline Exp $
 *
 * Queries the CDR to create subset list of documents.
 *
 * $Log: not supported by cvs2svn $
 */

#include <iostream>
#include "CdrSearch.h"
#include "CdrException.h"
#include "CdrCommand.h"
#include "CdrDbResultSet.h"
#include "CdrParserInput.h"

static std::string extractParams(const cdr::String&, cdr::db::Statement&);

cdr::String cdr::search(cdr::Session& session, 
                        const cdr::dom::Node& node, 
                        cdr::db::Connection& conn)
{
    // Extract the query from the command buffer.
    cdr::String queryString;
    cdr::dom::Node child = node.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name != L"Query")
                throw cdr::Exception(L"Unexpected element", name);
            queryString = cdr::dom::getTextContent(child);
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
    cdr::String sql = query.getSql();

    // Submit the query.to the DBMS.
    cdr::db::Statement select(conn);
    const char* pQuery = extractParams(sql, select).c_str();
    cdr::db::ResultSet rs = select.executeQuery(pQuery);

    // Construct the response.
    cdr::String response = L"  <CdrSearchResp>\n";
    
    int rows = 0;
    while (rs.next()) {
        if (rows++ == 0)
            response += L"   <QueryResults>\n";
        int id = rs.getInt(1);
        wchar_t tmp[40];
        swprintf(tmp, L"    <DocId>CDR%010ld</DocId>\n", id);
        response += tmp;
    }
    if (rows > 0)
        response += L"   </QueryResults>\n  </CdrSearchResp>\n";
    else
        response += L"   <QueryResults/>\n  </CdrSearchResp>\n";
    return response;
}

std::string extractParams(const cdr::String& sql, cdr::db::Statement& st)
{
    size_t      len = sql.size();       // Characters in original string
    std::string str(len, ' ');          // Work area for narrow string
    int         pos = 1;                // Parameter position in query
    size_t      i = 0, j = 0;           // Indices into sql and str

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
                    st.setString(pos++, param);
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
                    st.setString(pos++, param.getInt());
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
    if (op != CONTAINS)
        return cdr::String(1, STRING_MARK) + s + cdr::String(1, STRING_MARK);
    else
        return cdr::String(1, STRING_MARK) + L"%" + s + L"%"
            + cdr::String(1, STRING_MARK);
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

cdr::String cdr::Query::getSql()
{
    cdr::String sql = L"SELECT document.id FROM document";
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
        if (hasDocTypeTest) {
            sql += L", doc_type";
            where += L"document.doc_type = doc_type.id AND ";
        }
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
