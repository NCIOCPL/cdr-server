/*
 * $Id: CdrCheckAuth.cpp,v 1.2 2000-04-20 17:12:53 bkline Exp $
 *
 * Reports which actions are allowed for the current session.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2000/04/17 21:23:43  bkline
 * Initial revision
 */

// Eliminate annoying warnings about truncated debugging information.
#pragma warning(disable : 4786)

#include <map>
#include "CdrCommand.h"
#include "CdrDbResultSet.h"

// Matrix of action/doctype combinations.
typedef std::map<cdr::String, cdr::StringSet> Actions;

// Local function to lookup a single <Auth> element in command.
static void lookupAuth(const cdr::dom::Node&, 
                       Actions&,
                       cdr::db::Connection&,
                       int);

/**
 * Extracts permissions allowed for this user for the action/document-type
 * combinations specified by the CdrCheckAuth command.  The approach used here
 * (one SELECT query for each <AUTH> element) may not be as efficient for
 * large numbers of <AUTH> elements in a single command as building a single
 * mammoth SELECT query, but it's much simpler to implement correctly, and it
 * will be fine for the expected common uses, which will be either to ask if
 * the user can perform a single specific action, or (the most common
 * request): tell me everything this user can do.
 */
cdr::String cdr::checkAuth(cdr::Session& session, 
                           const cdr::dom::Node& node, 
                           cdr::db::Connection& conn)
{
    Actions actions;

    // Handle each <AUTH> element separately.
    cdr::dom::Node authNode = node.getFirstChild();
    while (authNode != 0) {
        if (authNode.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = authNode.getNodeName();
            if (name != L"Auth")
                throw cdr::Exception(L"Unexpected element", name);
            lookupAuth(authNode, actions, conn, session.uid);
        }
        authNode = authNode.getNextSibling();
    }

    // Construct the response.
    if (actions.size() == 0)
        return L"  <CdrCheckAuthResp/>\n";
    cdr::String response = L"  <CdrCheckAuthResp>\n";
    for (Actions::const_iterator i = actions.begin(); i != actions.end(); ++i)
    {
        const cdr::String& action = i->first;
        const cdr::StringSet& docTypes = i->second;
        cdr::StringSet::const_iterator typesIterator = docTypes.begin();
        while (typesIterator != docTypes.end()) {
            const cdr::String& docType = *typesIterator++;
            response += L"   <Auth><Action>"
                     +  action
                     +  L"</Action>";
            if (docType.size() > 0)
                response += L"<DocType>"
                         +  docType
                         +  L"</DocType>";
            response += L"</Auth>\n";
        }
    }
    response += L"  </CdrCheckAuthResp>\n";
    return response;
}

void lookupAuth(const cdr::dom::Node& authNode, 
                Actions& actions,
                cdr::db::Connection& conn,
                int uid)
{
    // Extract the <Action> and <DocType> nodes.
    cdr::String action, docType;
    cdr::dom::Node child = authNode.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"Action")
                action = cdr::dom::getTextContent(child);
            else if (name == L"DocType")
                docType = cdr::dom::getTextContent(child);
            else
                throw cdr::Exception(L"Unexpected node", name);
        }
        child = child.getNextSibling();
    }
    if (action.size() == 0)
        throw cdr::Exception(L"Missing or empty Action element");

    // Ask the database which combinations are available.
    int pos = 1;
    cdr::db::Statement select(conn);
    select.setInt(pos++, uid);
    std::string query = "SELECT DISTINCT a.name, t.name"
                        "           FROM action a,"
                        "                doc_type t,"
                        "                grp g,"
                        "                grp_action ga,"
                        "                grp_usr gu"
                        "          WHERE t.id   = ga.doc_type"
                        "            AND a.id   = ga.action"
                        "            AND gu.grp = ga.grp"
                        "            AND gu.usr = ?";
    if (action != L"*") {
        query +=        "            AND a.name = ?";
        select.setString(pos++, action);
    }
    if (docType != L"*") {
        query +=        "            AND t.name = ?";
        select.setString(pos++, docType);
    }
    cdr::db::ResultSet rs = select.executeQuery(query.c_str());

    // Add the values in the result set to the map of available actions.
    while (rs.next()) {
        cdr::String action  = rs.getString(1);
        cdr::String docType = rs.getString(2);

        // Add the action to the map if it isn't already there.
        if (actions.find(action) == actions.end())
            actions[action] = cdr::StringSet();

        // Get a reference to the set of doctypes associated with this action.
        cdr::StringSet& docTypes = actions[action];

        // Add the doctype if it isn't already present.
        if (docTypes.find(docType) == docTypes.end())
            docTypes.insert(docType);
    }
}
