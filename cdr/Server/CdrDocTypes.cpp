/*
 * $Id: CdrDocTypes.cpp,v 1.1 2001-01-16 21:11:52 bkline Exp $
 *
 * Support routines for CDR document types.
 *
 * $Log: not supported by cvs2svn $
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
extern cdr::String cdr::listDocTypes(Session&          session,
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
