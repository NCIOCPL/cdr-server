/*
 * $Id: CdrGetTree.cpp,v 1.1 2001-04-07 21:17:46 bkline Exp $
 *
 * Retrieves tree context information for terminology term.
 *
 * $Log: not supported by cvs2svn $
 */

#include "CdrCommand.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"
#include <list>
#include <iostream>

// Local types.
typedef std::list<int> IdList;

// Local functions.
static cdr::String getTermInfo(int, bool, bool, cdr::db::Connection&, IdList&);
static cdr::String getParents(int, cdr::db::Connection&, IdList&);
static cdr::String getChildren(int, cdr::db::Connection&, IdList&);
static cdr::String getDocTitle(int, cdr::db::Connection&, IdList&);
static bool        haveDoc(int, const IdList&);

cdr::String cdr::getTree(cdr::Session& session, 
                         const cdr::dom::Node& commandNode,
                         cdr::db::Connection& conn) 
{
    // Make sure our user is authorized to retrieve group information.
    if (!session.canDo(conn, L"GET TREE", L""))
        throw cdr::Exception(L"GET TREE action not authorized for this user");

    // Extract the document id from the command.
    int docId = 0;
    cdr::dom::Node child = commandNode.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"DocId") {
                cdr::String idString = cdr::dom::getTextContent(child);
                docId                = idString.extractDocId();
            }
        }
        child = child.getNextSibling();
    }
    if (docId == 0)
        throw cdr::Exception(L"Missing document id");

    // Build the tree information.
    IdList idList;
    idList.push_back(docId);
    cdr::String termInfo = getTermInfo(docId, true, true, conn, idList);
    return L"<CdrGetTreeResp>" + termInfo + L"</CdrGetTreeResp>";
}

cdr::String getDocTitle(int docId, cdr::db::Connection& conn)
{
    std::string query = "SELECT title FROM document WHERE id = ?";
    cdr::db::PreparedStatement stmt = conn.prepareStatement(query);
    stmt.setInt(1, docId);
    cdr::String title = "*** DOCUMENT CANNOT BE FOUND ***";
    cdr::db::ResultSet rs = stmt.executeQuery();
    if (rs.next())
        title = rs.getString(1);
    return title;
}

cdr::String getTermInfo(int docId, bool wantParents, bool wantKids,
                        cdr::db::Connection& conn, IdList& idList)
{
    cdr::String title    = getDocTitle(docId, conn);
    cdr::String parents  = wantParents ? getParents(docId, conn, idList) : L"";
    cdr::String children = wantKids ? getChildren(docId, conn, idList) : L"";
    return L"<Term><DocId>" + cdr::stringDocId(docId)
                            + L"</DocId><TermName>" + title
                            + L"</TermName>" + parents + children
                            + L"</Term>";
}

cdr::String getParents(int docId, cdr::db::Connection& conn, IdList& idList)
{
    std::string query = "SELECT value"
                        "  FROM query_term"
                        " WHERE doc_id = ?"
                        "   AND path = '/Term/TermParent/@cdr:ref'";
    cdr::db::PreparedStatement stmt = conn.prepareStatement(query);
    stmt.setInt(1, docId);
    cdr::db::ResultSet rs = stmt.executeQuery();
    cdr::String result;
    IdList localList;
    while (rs.next()) {
        int id = rs.getString(1).extractDocId();
        if (!haveDoc(id, idList)) {
            idList.push_back(id);
            localList.push_back(id);
        }
    }
    if (!localList.empty()) {
        result = L"<Parents>";
        for (IdList::const_iterator i = localList.begin();
             i != localList.end();
             ++i)
            result += getTermInfo(*i, true, false, conn, idList);
        result += L"</Parents>";
    }
    return result;
}

cdr::String getChildren(int docId, cdr::db::Connection& conn, IdList& idList)
{
    std::string query = "SELECT doc_id"
                        "  FROM query_term"
                        " WHERE value = ?"
                        "   AND path = '/Term/TermParent/@cdr:ref'";
    cdr::db::PreparedStatement stmt = conn.prepareStatement(query);
    stmt.setString(1, cdr::stringDocId(docId));
    cdr::db::ResultSet rs = stmt.executeQuery();
    cdr::String result;
    IdList localList;
    while (rs.next()) {
        int id = rs.getInt(1);
        if (!haveDoc(id, idList)) {
            idList.push_back(id);
            localList.push_back(id);
        }
    }
    if (!localList.empty()) {
        result = L"<Children>";
        for (IdList::const_iterator i = localList.begin();
             i != localList.end();
             ++i)
            result += getTermInfo(*i, false, true, conn, idList);
        result += L"</Children>";
    }
    return result;
}

bool haveDoc(int id, const IdList& idList)
{
    for (IdList::const_iterator i = idList.begin(); i != idList.end(); ++i) {
        if (id == *i)
            return true;
    }
    return false;
}
