/*
 * CdrSysValue.cpp
 *
 * Implements maintenance and access of the sys_value control
 * table.
 *
 *                                           Alan Meyer  January, 2003
 *
 * $Id$
 * $Log: not supported by cvs2svn $
 */

#include "CdrSysValue.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"
#include "CdrCommand.h"
#include "CdrDom.h"

// See CdrSysValue.h
void cdr::addSysValue (
     cdr::Session& session,
     cdr::db::Connection& conn,
     const cdr::String& name,
     const cdr::String& value,
     const cdr::String& notes,
     const cdr::String& program
) {
     // Is user authorized to do this
     if (!session.canDo (conn, L"SET_SYS_VALUE", L""))
          throw cdr::Exception (L"addSysValue: User '" + session.getUserName()
                                + L"' not authorized to set system values");

     // Can't overwrite an existing value
     if (!((cdr::getSysValue (session, conn, name)).isNull()))
          throw cdr::Exception (L"addSysValue: Attempt to add name '" + name
                                + L"' when name already exists");

     // A null string if we need it to replace empties
     cdr::String nullStr (true);

     // Create the new row in the table
     std::string sql =
          "INSERT INTO sys_value "
          "    (name, value, last_change, usr, notes, program) "
          "VALUES (?, ?, GETDATE(), ?, ?, ?)";

     cdr::db::PreparedStatement stmt = conn.prepareStatement (sql);
     stmt.setString (1, name);
     stmt.setString (2, value);
     stmt.setInt    (3, session.getUserId());
     // Convert empty strings to nulls on notes and program
     stmt.setString (4, (!notes.isNull() && notes != L"") ?
                     notes : nullStr);
     stmt.setString (5, (!program.isNull() && program != L"") ?
                     program : nullStr);

     // Any exceptions will bubble up to the top
     stmt.executeUpdate();
}

// See CdrSysValue.h
void cdr::repSysValue (
     cdr::Session& session,
     cdr::db::Connection& conn,
     const cdr::String& name,
     const cdr::String& value,
     const cdr::String& notes,
     const cdr::String& program
) {
     // Is user authorized
     if (!session.canDo (conn, L"SET_SYS_VALUE", L""))
          throw cdr::Exception (L"repSysValue: User '" + session.getUserName()
                                + L"' not authorized to set system values");

     // Name must exist to be replaced
     if ((cdr::getSysValue (session, conn, name)).isNull())
          throw cdr::Exception (L"repSysValue: Attempt to replace name '" +name
                                + L"' but name does not exist");

     // Replace the row in the table
     // If notes not passed, do not replace existing notes
     std::string sql;
     if (!notes.isNull() && notes != L"")
          sql =
               "UPDATE sys_value "
               "   SET value = ?, "
               "       last_change = GETDATE(), "
               "       usr = ?, "
               "       program = ?, "
               "       notes = ? "
               " WHERE name = ?";
     else
          sql =
               "UPDATE sys_value "
               "   SET value = ?, "
               "       last_change = GETDATE(), "
               "       usr = ?, "
               "       program = ? "
               " WHERE name = ?";

     // Values
     cdr::db::PreparedStatement stmt = conn.prepareStatement (sql);
     stmt.setString (1, value);
     stmt.setInt    (2, session.getUserId());
     // Convert empty strings to nulls on program
     stmt.setString (3, (program != L"") ? program : cdr::String(true));

     // If notes passed
     if (!notes.isNull() && notes != L"") {
          stmt.setString (4, notes);
          stmt.setString (5, name);
     }
     else
          stmt.setString (4, name);

     // Any exceptions bubble up to the top
     stmt.executeUpdate();
}

// See CdrSysValue.h
void cdr::delSysValue (
     cdr::Session& session,
     cdr::db::Connection& conn,
     const cdr::String& name
) {
     // Is user authorized
     if (!session.canDo (conn, L"SET_SYS_VALUE", L""))
          throw cdr::Exception (L"delSysValue: User '" + session.getUserName()
                                + L"' not authorized to set system values");

     // Name must exist to be deleted
     if ((cdr::getSysValue (session, conn, name)).isNull())
          throw cdr::Exception (L"repSysValue: Attempt to delete name '" +name
                                + L"' but name does not exist");

     // Delete row
     std::string sql = "DELETE FROM sys_value WHERE name = ?";
     cdr::db::PreparedStatement stmt = conn.prepareStatement (sql);
     stmt.setString (1, name);

     // Any exceptions bubble up to the top
     stmt.executeUpdate();
}

// See CdrSysValue.h
cdr::String cdr::getSysValue (
     cdr::Session& session,
     cdr::db::Connection& conn,
     const cdr::String& name
) {
     // No authorization required

     // Try to find it
     std::string sql = "SELECT value FROM sys_value WHERE name = ?";

     cdr::db::PreparedStatement stmt = conn.prepareStatement (sql);
     stmt.setString (1, name);

     // Should be 0 or 1 rows
     cdr::db::ResultSet rs = stmt.executeQuery();

     // Create a default null string to return if nothing found
     cdr::String value (true);

     // 0 rows is okay here, caller decides if it's an error
     if (!rs.next())
          return value;

     // Else get actual value
     value = rs.getString (1);

     // If more than one row, something is wrong with the table
     // May add a function later to do LIKE retrievals if it turns out
     //   that we need them
     if (rs.next())
          throw cdr::Exception (L"getSysValue found more than one occurrence"
                                L" of name='" + name + L"' in table");
     return value;
}

/*
 * This is the XML transaction command front end to all of the
 * sys_value table manipulations.
 *
 * The specific command name comes from the top node in the passed
 * DOM element.
 *
 * Defined in CdrCommand.h
 */
cdr::String cdr::sysValue (
    cdr::Session& session,
    const cdr::dom::Node& node,
    cdr::db::Connection& conn
) {
     // Get the name of the passed node.  It is our command type.
     cdr::String cmd = node.getNodeName();

     // Pull out any subelements
     cdr::String name(true);
     cdr::String value(true);
     cdr::String program(true);
     cdr::String notes(true);
     cdr::dom::Node child = node.getFirstChild();
     while (child != 0) {
          // Get contents of transaction
          if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
               cdr::String nodeName = child.getNodeName();
               if (nodeName == L"Name")
                    name = cdr::dom::getTextContent (child);
               else if (nodeName == L"Value")
                    value = cdr::dom::getTextContent (child);
               else if (nodeName == L"Program")
                    program = cdr::dom::getTextContent (child);
               else if (nodeName == L"Notes")
                    notes = cdr::dom::getTextContent (child);
          }

          // Look for more
          child = child.getNextSibling();
     }

     // Return value for CdrGetSysValue, initially null
     cdr::String retValue (true);

     // Call the appropriate internal function based on transaction type
     if (cmd == L"CdrGetSysValue")
          // This should be most common use
          retValue = cdr::getSysValue (session, conn, name);

     else if (cmd == L"CdrAddSysValue")
          cdr::addSysValue (session, conn, name, value, notes, program);

     else if (cmd == L"CdrRepSysValue")
          cdr::repSysValue (session, conn, name, value, notes, program);

     else if (cmd == L"CdrDelSysValue")
          cdr::delSysValue (session, conn, name);

     else
          throw cdr::Exception (L"sysValue: unrecognized command '" + cmd +
                                L"' can't happen!");

     // Construct response
     cdr::String tag = cmd + L"Resp";
     cdr::String resp = L"<" + tag + L">";
     if (!retValue.isNull())
          resp += L"\n <Value>" + cdr::entConvert(retValue) + L"</Value>\n";
     resp += L"</" + tag + L">\n";

     return resp;
}
