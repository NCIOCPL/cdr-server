/*
 * $Id: CdrGetTree.cpp,v 1.3 2002-01-02 21:59:25 bkline Exp $
 *
 * Retrieves tree context information for terminology term.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2001/04/08 22:44:31  bkline
 * Optimized to use stored procedure; much faster.
 *
 * Revision 1.1  2001/04/07 21:17:46  bkline
 * Initial revision
 */

#include "CdrCommand.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"
#include <iostream>

cdr::String cdr::getTree(cdr::Session& session, 
                         const cdr::dom::Node& commandNode,
                         cdr::db::Connection& conn) 
{
    // Make sure our user is authorized to retrieve group information.
    if (!session.canDo(conn, L"GET TREE", L""))
        throw cdr::Exception(L"GET TREE action not authorized for this user");

    // Extract the document id from the command.
    int docId = 0;
    int depth = 1;
    cdr::dom::Node child = commandNode.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"DocId") {
                cdr::String idString = cdr::dom::getTextContent(child);
                docId                = idString.extractDocId();
            }
            else if (name == L"ChildDepth") {
                cdr::String depthString = cdr::dom::getTextContent(child);
                depth = depthString.getInt();
            }

        }
        child = child.getNextSibling();
    }
    if (docId == 0)
        throw cdr::Exception(L"Missing document id");

    // Construct a response.
    std::wostringstream resp;
    resp << L"<CdrGetTreeResp>\n";

    // Invoke the stored procedure to collect the tree information.
    std::string proc = "{call cdr_get_term_tree(?,?)}";
    cdr::db::PreparedStatement stmt = conn.prepareStatement(proc);
    stmt.setInt(1, docId);
    stmt.setInt(2, depth);
    cdr::db::ResultSet rs1 = stmt.executeQuery();

    // Extract the child/parent pairs.
    resp << L"<Pairs>\n";
    while (rs1.next()) {
        int child  = rs1.getInt(1);
        int parent = rs1.getInt(2);
        resp << L"<Pair><Child>" << child << L"</Child>"
             << L"<Parent>" << parent << L"</Parent></Pair>\n";
    }
    resp << L"</Pairs>\n";

    // Extract the Term names.
    resp << L"<Terms>\n";
    if (!stmt.getMoreResults())
        throw cdr::Exception(L"Failure retrieving Term data");
    cdr::db::ResultSet rs2 = stmt.getResultSet();
    while (rs2.next()) {
        int id            = rs2.getInt(1);
        cdr::String title = rs2.getString(2);
        resp << L"<Term><Id>" << id << L"</Id>"
             << L"<Name>" << title << L"</Name></Term>\n";
    }
    resp << L"</Terms>\n";
    
    // All done.
    resp << L"</CdrGetTreeResp>\n";
    return resp.str();
}
