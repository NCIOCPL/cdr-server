
/*
 * $Id: CdrAddGrp.cpp,v 1.1 2000-04-22 09:32:18 bkline Exp $
 *
 * Adds group definition to CDR.
 *
 * $Log: not supported by cvs2svn $
 */

#include <list>
#include "CdrCommand.h"
#include "CdrDbResultSet.h"

struct Auth { cdr::String action, docType; };
typedef std::list<Auth> AuthList;

cdr::String cdr::addGrp(cdr::Session& session, 
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection) 
{
    // Make sure our user is authorized to add groups.
    if (!session.canDo(dbConnection, L"ADD GROUP", L""))
        throw cdr::Exception(L"ADD GROUP action not authorized for this user");

    // Extract the data elements from the command node.
    AuthList authList;
    cdr::StringList usrList;
    cdr::String grpName, comment;
    cdr::dom::Node child = commandNode.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"GrpName")
                grpName = cdr::dom::getTextContent(child);
            else if (name == L"Comment")
                comment = cdr::dom::getTextContent(child);
            else if (name == L"UserName")
                usrList.push_back(cdr::dom::getTextContent(child));
            else if (name == L"Auth") {
                Auth auth;
                cdr::dom::Node authChild = child.getFirstChild();
                while (authChild != 0) {
                    cdr::String authName = authChild.getNodeName();
                    if (authName == L"Action")
                        auth.action = cdr::dom::getTextContent(authChild);
                    else if (authName == L"DocType")
                        auth.docType = cdr::dom::getTextContent(authChild);
                    authChild = authChild.getNextSibling();
                }
                if (auth.action.size() == 0)
                    throw cdr::Exception(L"Missing Action element in Auth");
                authList.push_back(auth);
            }
        }
        child = child.getNextSibling();
    }
    if (grpName.size() == 0)
        throw cdr::Exception(L"Missing group name");

    // Make sure the group doesn't already exist.
    cdr::db::Statement grpQuery(dbConnection);
    grpQuery.setString(1, grpName);
    cdr::db::ResultSet grpRs = grpQuery.executeQuery("SELECT COUNT(*)"
                                                     "  FROM grp"
                                                     " WHERE name = ?");
    if (!grpRs.next())
        throw cdr::Exception(L"Failure checking unique group name");
    int count = grpRs.getInt(1);
    if (count != 0)
        throw cdr::Exception(L"Group name already exists");

    // Add a new row to the grp table.
    if (authList.size() > 0)
        dbConnection.setAutoCommit(false);
    cdr::db::Statement grpInsert(dbConnection);
    grpInsert.setString(1, grpName);
    if (comment.size() > 0) {
        grpInsert.setString(2, comment);
        grpInsert.executeQuery("INSERT into grp(name, comment) VALUES(?, ?)");
    }
    else
        grpInsert.executeQuery("INSERT into grp(name) VALUES(?)");
    int grpId = dbConnection.getLastIdent();

    // Add users, if any present.
    if (usrList.size() > 0) {
        StringList::const_iterator i = usrList.begin();
        while (i != usrList.end()) {
            const cdr::String uName = *i++;

            // Do INSERT the hard way so we can determine success.
            cdr::db::Statement usrQuery(dbConnection);
            usrQuery.setString(1, uName);
            cdr::db::ResultSet rs = 
                usrQuery.executeQuery("SELECT id"
                                      "  FROM usr"
                                      " WHERE name = ?");
            if (!rs.next())
                throw cdr::Exception(L"Unknown user", uName);
            int usrId = rs.getInt(1);
            cdr::db::Statement insert(dbConnection);
            insert.setInt(1, grpId);
            insert.setInt(2, usrId);
            insert.executeQuery("INSERT INTO grp_usr"
                                "("
                                "    grp,"
                                "    usr"
                                ")"
                                "VALUES(?, ?)");
        }
    }

    // Add authorizations, if any present.
    if (authList.size() > 0) {
        AuthList::const_iterator i = authList.begin();
        while (i != authList.end()) {
            const Auth& auth = *i++;

            // Do INSERT the hard way so we can determine success.
            int actionId, docTypeId;
            cdr::db::Statement actionQuery(dbConnection);
            actionQuery.setString(1, auth.action);
            cdr::db::ResultSet rs1 = 
                actionQuery.executeQuery("SELECT id"
                                         "  FROM action"
                                         " WHERE name = ?");
            if (!rs1.next())
                throw cdr::Exception(L"Unknown action", auth.action);
            actionId = rs1.getInt(1);
            cdr::db::Statement docTypeQuery(dbConnection);
            docTypeQuery.setString(1, auth.docType);
            cdr::db::ResultSet rs2 = 
                docTypeQuery.executeQuery("SELECT id"
                                          "  FROM doc_type"
                                          " WHERE name = ?");
            if (!rs2.next())
                throw cdr::Exception(L"Unknown doc type", auth.docType);
            docTypeId = rs2.getInt(1);
            cdr::db::Statement insert(dbConnection);
            insert.setInt(1, grpId);
            insert.setInt(2, actionId);
            insert.setInt(3, docTypeId);
            insert.executeQuery("INSERT INTO grp_action"
                                "("
                                "    grp,"
                                "    action,"
                                "    doc_type"
                                ")"
                                "VALUES(?, ?, ?)");
        }
    }

    // Report success.
    if (authList.size() > 0)
        dbConnection.commit();
    return L"  <CdrAddGrpResp/>\n";
}
