/*
 * $Id: CdrVersion.cpp,v 1.4 2000-11-20 15:23:32 mruben Exp $
 *
 * Version control functions
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2000/10/31 15:46:13  mruben
 * added tests for authorization
 *
 * Revision 1.2  2000/10/25 19:06:34  mruben
 * fixed date representation
 *
 * Revision 1.1  2000/10/23 14:08:34  mruben
 * Initial revision
 *
 *
 */

#if defined _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <cstdio>
#include <memory>
#include <sstream>
#include <string>

#include "CdrCommand.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"
#include "CdrException.h"
#include "CdrString.h"
#include "CdrVersion.h"

using std::string;
using std::auto_ptr;
using std::wostringstream;
using std::wistringstream;

namespace
{
  bool allowVersion(int docId, cdr::db::Connection& conn)
  {
    string query = "SELECT dt.versioning "
                   "FROM document d "
                   "JOIN doc_type dt "
                   "ON d.doc_type = dt.id "
                   "WHERE d.id = ?";
    cdr::db::PreparedStatement select = conn.prepareStatement(query);
    select.setInt(1, docId);
    cdr::db::ResultSet rs = select.executeQuery();
    if (!rs.next())
      throw cdr::Exception(L"Error getting document type");

    return rs.getString(1) == L"Y";
  }
}

int cdr::checkIn(cdr::Session& session, int docId,
                 cdr::db::Connection& conn, 
                 const cdr::String* comment, bool abandon, bool force)
{
  // we need to serialize.  Save current autocommit state so if the
  // caller has already done so, we don't mess him up
  bool autocommitted = conn.getAutoCommit();
  conn.setAutoCommit(false);

  int usr = session.getUserId();

  // what is the current version?
  int version = getVersionNumber(docId, conn);
  if (version < 0)
    throw cdr::Exception(L"Document does not exist");

  if (version > 0 && !abandon)
    abandon = !isChanged(docId, conn);
    
  int checked_out_usr;
  cdr::String dt_out;
  if (isCheckedOut(docId, conn, &checked_out_usr, &dt_out))
  {
    // someone already has it checked out.  Is it this user
    if (usr != checked_out_usr)
      if (!force)
      {
        string usr_query = "SELECT name, fullname from usr "
                           "WHERE id = ?";
        cdr::db::PreparedStatement ch_select
          = conn.prepareStatement(usr_query);
        ch_select.setInt(1, checked_out_usr);
        cdr::db::ResultSet usr_rs = ch_select.executeQuery();
        cdr::String usrname;
        cdr::String fullname;
        if (usr_rs.next())
        {
          usrname = usr_rs.getString(1);
          fullname = usr_rs.getString(2);
          if (fullname.size() != 0)
            fullname = L" (" + fullname + L")";
        }
        else
        {
          wostringstream u;
          u << checked_out_usr;
          usrname = u.str();
        }
        throw cdr::Exception(L"Document not checked out to current user"
                             L".  Checked out to"
                             + usrname
                             + fullname
                             + L" at " + dt_out);
      }
      else
        if (!session.canDo(conn, "FORCE CHECKIN", docId))
          throw cdr::Exception(L"User not authorized to force checkin");

    if (comment != NULL)
    {
      string update = "UPDATE checkout "
                      "SET dt_in = GETDATE(), version = ?, comment = ? "
                      "WHERE id = ? and dt_in is NULL";
      cdr::db::PreparedStatement up = conn.prepareStatement(update);
      up.setInt(1, abandon ? Int(true) : version);
      up.setInt(2, docId);
      up.setString(3, *comment);
      up.executeQuery();    }
    else
    {
      string update = "UPDATE checkout "
                      "SET dt_in = GETDATE(), version = ? "
                      "WHERE id = ? and dt_in is NULL";
      cdr::db::PreparedStatement up = conn.prepareStatement(update);
      up.setInt(1, abandon ? Int(true) : version);
      up.setInt(2, docId);
      up.executeQuery();
    }
  }
  else
  {
    if (!allowVersion(docId, conn))
      throw cdr::Exception("Version control  not allowed for document type");
    if (version > 0)
      throw cdr::Exception(L"Document not checked out");
    if (!session.canDo(conn, "MODIFY DOCUMENT", docId))
      throw cdr::Exception(L"User not authorized to modify document");
  }

      
  // now add new version if not abandoned
  if (!abandon)
  {
    ++version;
    string query = "SELECT d.doc_type, d.title, d.xml, "
                   "       d.comment, a.dt, b.data "
                   "FROM document d "
                   "JOIN audit_trail a ON a.document = d.id "
                   "LEFT OUTER JOIN doc_blob b ON b.id = d.id "
                   "WHERE d.id = ? "
                   "  AND a.dt = (SELECT MAX(dt) "
                   "              FROM audit_trail aa"
                   "              WHERE aa.document = ?)";
    cdr::db::PreparedStatement select = conn.prepareStatement(query);
    select.setInt(1, docId);
    select.setInt(2, docId);
    cdr::db::ResultSet rs = select.executeQuery();
    if (!rs.next())
      throw cdr::Exception(L"Error getting document data");

    int doc_type = rs.getInt(1);
    cdr::String title = rs.getString(2);
    cdr::String xml = rs.getString(3);
    cdr::String doc_comment = rs.getString(4);
    cdr::String updated_dt = rs.getString(5);
    cdr::Blob data = rs.getBytes(6);

    string newver = "INSERT INTO doc_version "
      "(id, num, dt, updated_dt, usr, doc_type, title, "
      " xml, data, comment) "
      "VALUES (?, ?, GETDATE(), ?, ?, ?, ?, ?, ?, ?)";
    cdr::db::PreparedStatement insert = conn.prepareStatement(newver);
    insert.setInt(1, docId);
    insert.setInt(2, version);
    insert.setString(3, updated_dt);
    insert.setInt(4, usr);
    insert.setInt(5, doc_type);
    insert.setString(6, title);
    insert.setString(7, xml);
    insert.setBytes(8, data);
    insert.setString(9, doc_comment);
    insert.executeQuery();
  }

  conn.setAutoCommit(autocommitted);
  return abandon ? -1 : version;
}
  
int cdr::checkOut(cdr::Session& session,
                  int docId, cdr::db::Connection& conn,
                  const cdr::String& comment, bool force)
{
  // we need to serialize.  Save current autocommit state so if the
  // caller has already done so, we don't mess him up
  bool autocommitted = conn.getAutoCommit();
  conn.setAutoCommit(false);

  int usr = session.getUserId();
  
  if (!allowVersion(docId, conn))
    throw cdr::Exception("Version control  not allowed for document type");

  if (!session.canDo(conn, "MODIFY DOCUMENT", docId))
    throw cdr::Exception(L"User not authorized to modify document");
  
  // is the document checked out?    
  string query = "SELECT dt_out, usr "
    "FROM checkout "
    "WHERE id = ? AND dt_in IS NULL";
  cdr::db::PreparedStatement select = conn.prepareStatement(query);
  select.setInt(1, docId);
  cdr::db::ResultSet rs = select.executeQuery();
  if (rs.next())
  {
    // someone already has it checked out
    if (force)
    {
      if (!session.canDo(conn, "FORCE CHECKOUT", docId))
        throw cdr::Exception(L"User not authorized to force checkout");
      checkIn(session, docId, conn, NULL, true, force);
    }
    else
    {
      cdr::String dt_out = rs.getString(1);
      int checked_out_usr = rs.getInt(2);

      string ch_query = "SELECT name, fullname from usr "
        "WHERE id = ?";
      cdr::db::PreparedStatement ch_select = conn.prepareStatement(ch_query);
      ch_select.setInt(1, checked_out_usr);
      cdr::db::ResultSet ch_rs = ch_select.executeQuery();
      cdr::String usrname;
      cdr::String fullname;
      if (ch_rs.next())
      {
        usrname = ch_rs.getString(1);
        fullname = ch_rs.getString(2);
        if (fullname.size() != 0)
          fullname = L" (" + fullname + ")";
      }
      else
      {
        wostringstream u;
        u << checked_out_usr;
        usrname = u.str();
      }
      throw cdr::Exception(L"Document already checked out to "
                           + usrname
                           + fullname
                           + " at " + dt_out);
    }
  }

  // what is the current version of the document?
  int version = getVersionNumber(docId, conn);
  if (version < 0)
    throw cdr::Exception(L"Document does not exist");

  string insert = "INSERT INTO checkout "
    "(id, dt_out, usr, comment) "
    "VALUES (?, GETDATE(), ?, ?)";
  cdr::db::PreparedStatement stmt = conn.prepareStatement(insert);
  stmt.setInt(1, docId);
  stmt.setInt(2, usr);
  stmt.setString(3, (comment.size() == 0) ? cdr::String(false) : comment);
  stmt.executeQuery();

  conn.setAutoCommit(autocommitted);
  return version;
}
  
bool cdr::getVersion(int docId, cdr::db::Connection& conn, int num,
                     CdrVerDoc* verdoc)
{
  string query = "SELECT dt, updated_dt, usr, doc_type, title, "
    "xml, data, comment "
    "FROM doc_version "
    "WHERE id = ? and num = ?";
  cdr::db::PreparedStatement select = conn.prepareStatement(query);
  select.setInt(1, docId);
  select.setInt(2, num);
  cdr::db::ResultSet rs = select.executeQuery();

  if (!rs.next())
    return false;

  if (verdoc != NULL)
  {
    verdoc->document = docId;
    verdoc->num = num;
    verdoc->dt = rs.getString(1);
    verdoc->updated_dt = cdr::toXmlDate(rs.getString(2));
    verdoc->usr = rs.getInt(3);
    verdoc->doc_type = rs.getInt(4);
    verdoc->title = rs.getString(5);
    verdoc->xml = rs.getString(6);
    verdoc->data = rs.getBytes(7);
    verdoc->comment = rs.getString(8);
  }
    
  return true;
}

bool cdr::getVersion(int docId, cdr::db::Connection& conn,
                     const cdr::String& dt,
                     CdrVerDoc* verdoc)
{
  string query = "SELECT num, dt, updated_dt, usr, doc_type, title, "
                 "xml, data, comment "
                 "FROM doc_version "
                 "WHERE document = ? "
                 "  AND dt = (SELECT MAX(dt) FROM doc_version "
                 "WHERE document = ? AND dt <= ?)";
  cdr::db::PreparedStatement select = conn.prepareStatement(query);
  select.setInt(1, docId);
  select.setInt(2, docId);
  select.setString(3, dt);
  cdr::db::ResultSet rs = select.executeQuery();

  if (!rs.next())
    return false;

  if (verdoc != NULL)
  {
    verdoc->document = docId;
    verdoc->num = rs.getInt(1);;
    verdoc->dt = rs.getString(2);
    verdoc->updated_dt = cdr::toXmlDate(rs.getString(3));
    verdoc->usr = rs.getInt(4);
    verdoc->doc_type = rs.getInt(5);
    verdoc->title = rs.getString(6);
    verdoc->xml = rs.getString(7);
    verdoc->data = rs.getBytes(8);
    verdoc->comment = rs.getString(9);
  }
    
  return true;
}

bool cdr::getLabelVersion(int docId, cdr::db::Connection& conn,
                          const cdr::String& label_name,
                          CdrVerDoc* verdoc)
{
  string query = "SELECT d.num, d.dt, d.updated_dt, d.usr, "
                        "d.doc_type, d.title, d.xml, d.data, d.comment "
                        "FROM doc_version d "
                        "INNER JOIN doc_version_label dl "
                        "ON d.id = dl.document "
                        "INNER JOIN version_label vl "
                        "ON dl.label = vl.id "
                        "WHERE d.id = ? "
                        "  AND vl.name = ?";
  cdr::db::PreparedStatement select = conn.prepareStatement(query);
  select.setInt(1, docId);
  select.setString(2, label_name);
  cdr::db::ResultSet rs = select.executeQuery();

  if (!rs.next())
    return false;

  if (verdoc != NULL)
  {
    verdoc->document = docId;
    verdoc->num = rs.getInt(1);;
    verdoc->dt = rs.getString(2);
    verdoc->updated_dt = cdr::toXmlDate(rs.getString(3));
    verdoc->usr = rs.getInt(4);
    verdoc->doc_type = rs.getInt(5);
    verdoc->title = rs.getString(6);
    verdoc->xml = rs.getString(7);
    verdoc->data = rs.getBytes(8);
    verdoc->comment = rs.getString(9);
  }
    
  return true;
}

int cdr::getVersionNumber(int docId, cdr::db::Connection& conn,
                          cdr::String* date)
{
  string query = "SELECT v.num, v.dt "
                 "FROM document d "
                 "LEFT OUTER JOIN doc_version v "
                 "  ON d.id = v.id "
                 "WHERE d.id = ? "
                 "  AND (v.num = (SELECT MAX(num) FROM doc_version vv "
                                 "WHERE vv.id = ?) "
                           "OR v.num IS NULL)";
  cdr::db::PreparedStatement select = conn.prepareStatement(query);
  select.setInt(1, docId);
  select.setInt(2, docId);
  cdr::db::ResultSet rs = select.executeQuery();
  if (rs.next())
  {
    if (date != NULL)
      *date = cdr::toXmlDate(rs.getString(2));

    return rs.getInt(1);
  }

  return -1;
}

bool cdr::isChanged(int docId, cdr::db::Connection& conn)
{
  string query = "SELECT num FROM doc_version d "
                 "INNER JOIN audit_trail a ON d.id = a.document "
                 "WHERE d.updated_dt = a.dt "
                 "  AND d.id = ? "
                 "  AND d.num = (SELECT MAX(dd.num) from doc_version dd"
                 "                  WHERE dd.id = ?)"
                 "  AND a.dt = (SELECT MAX(aa.dt) from audit_trail aa "
                 "                 WHERE aa.document = ?)";
  cdr::db::PreparedStatement select = conn.prepareStatement(query);
  select.setInt(1, docId);
  select.setInt(2, docId);
  select.setInt(3, docId);
  cdr::db::ResultSet rs = select.executeQuery();
  return !rs.next();
}

bool cdr::isCheckedOut(int docId, cdr::db::Connection& conn, int* usr,
                       cdr::String* dt_out)
{
  string query = "SELECT dt_out, usr "
    "FROM checkout "
    "WHERE id = ? and dt_in IS NULL";
  cdr::db::PreparedStatement select = conn.prepareStatement(query);
  select.setInt(1, docId);
  cdr::db::ResultSet rs = select.executeQuery();
  if (!rs.next())
    return false;

  if (dt_out != NULL)
    *dt_out = rs.getString(1);
  if (usr != NULL)
    *usr = rs.getInt(2);

  return true;
}

cdr::String cdr::checkVerOut(cdr::Session& session,
                             const cdr::dom::Node& commandNode,
                             cdr::db::Connection& dbConnection)
{
  bool force = false;
  int version = -1;
  cdr::String comment;
  cdr::String ui_string;

  cdr::dom::NamedNodeMap cmdattr = commandNode.getAttributes();
  cdr::dom::Node attr = cmdattr.getNamedItem("ForceCheckOut");
  if (attr != NULL && attr.getNodeValue() == "Y")
    force = true;

  for (cdr::dom::Node child = commandNode.getFirstChild();
       child != NULL;
       child = child.getNextSibling())
  {
    if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE)
    {
      cdr::String name = child.getNodeName();
      if (name == L"DocumentId")
      {
        cdr::dom::Node n = child.getFirstChild();
        if (n == NULL || n.getNodeType() != cdr::dom::Node::TEXT_NODE)
          throw cdr::Exception(L"Invalid document ID");
        ui_string = n.getNodeValue();
      }
      if (name == L"Comment")
      {
        cdr::dom::Node c = child.getFirstChild();
        if (c != NULL)
        {
          cdr::dom::Node::NodeType t =
            static_cast<cdr::dom::Node::NodeType>(c.getNodeType());
          if (t == cdr::dom::Node::CDATA_SECTION_NODE
              || t == cdr::dom::Node::TEXT_NODE)
            comment = c.getNodeValue();
        }
      }
    }
  }
  version = checkOut(session, ui_string.extractDocId(), dbConnection,
                    comment, force);
  
  wostringstream ver_string;
  ver_string << L"<Version>" << version << L"</Version>";

  return L"<CdrCheckOutResp>" + ver_string.str() + L"<CdrCheckOutResp/>";
}

cdr::String cdr::checkVerIn(cdr::Session& session,
                            const cdr::dom::Node& commandNode,
                            cdr::db::Connection& dbConnection)
{
  bool abandon = false;
  bool force = false;
  int version = -1;
  auto_ptr<cdr::String> comment;
  cdr::String ui_string;

  cdr::dom::NamedNodeMap cmdattr = commandNode.getAttributes();
  cdr::dom::Node attr = cmdattr.getNamedItem("Abandon");
  if (attr != NULL && attr.getNodeValue() == "Y")
    abandon = true;
  attr = cmdattr.getNamedItem("ForceCheckIn");
  if (attr != NULL && attr.getNodeValue() == "Y")
    force = true;

  for (cdr::dom::Node child = commandNode.getFirstChild();
       child != NULL;
       child = child.getNextSibling())
  {
    if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE)
    {
      cdr::String name = child.getNodeName();
      if (name == L"DocumentId")
      {
        cdr::dom::Node n = child.getFirstChild();
        if (n == NULL || n.getNodeType() != cdr::dom::Node::TEXT_NODE)
          throw cdr::Exception(L"Invalid document ID");
        ui_string = n.getNodeValue();
      }
      if (name == L"Comment")
      {
        cdr::dom::Node c = child.getFirstChild();
        if (c == NULL)
          comment = auto_ptr<cdr::String>(new cdr::String(false));
        else
        {
          cdr::dom::Node::NodeType t =
            static_cast<cdr::dom::Node::NodeType>(c.getNodeType());
          if (t == cdr::dom::Node::CDATA_SECTION_NODE
              || t == cdr::dom::Node::TEXT_NODE)
            comment = auto_ptr<cdr::String>(new cdr::String(c.getNodeValue()));
        }
      }
    }
  }
  version = checkIn(session, ui_string.extractDocId(), dbConnection,
                    comment.get(), abandon, force);
  
  wostringstream ver_string;
  if (version >= 0)
    ver_string << L"<Version>" << version << L"</Version>";
  else
    ver_string << L"<Version/>";

  return L"<CdrCheckInResp>" + ver_string.str() + L"<CdrCheckInResp/>";
}

cdr::String cdr::createLabel(cdr::Session& session,
                             const cdr::dom::Node& commandNode,
                             cdr::db::Connection& dbConnection)
{
  cdr::String label_name;
  cdr::String label_comment;

  for (cdr::dom::Node child = commandNode.getFirstChild();
       child != NULL;
       child = child.getNextSibling())
  {
    if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE)
    {
      cdr::String name = child.getNodeName();
      if (name == L"Name")
      {
        cdr::dom::Node n = child.getFirstChild();
        if (n == NULL || n.getNodeType() != cdr::dom::Node::TEXT_NODE)
          throw cdr::Exception(L"Invalid label name");
        label_name = n.getNodeValue();
      }
      if (name == L"Comment")
      {
        cdr::dom::Node c = child.getFirstChild();
          cdr::dom::Node::NodeType t =
            static_cast<cdr::dom::Node::NodeType>(c.getNodeType());
        if (t == cdr::dom::Node::CDATA_SECTION_NODE
            || t == cdr::dom::Node::TEXT_NODE)
          label_comment = c.getNodeValue();
      }
    }
  }

  if (label_name.size() == 0)
    throw cdr::Exception(L"Invalid or missing label name");

  if (label_comment.size() == 0)
    label_comment = cdr::String(true);

  string insert_sql = "INSERT into version_label "
                      "(name, comment) "
                      "VALUES (?, ?)";
  cdr::db::PreparedStatement insert
        = dbConnection.prepareStatement(insert_sql);
  insert.setString(1, label_name);
  insert.setString(2, label_comment);
  insert.executeQuery();
  
  return L"<CdrCreateLabelResp/>";
}

cdr::String cdr::deleteLabel(cdr::Session& session,
                             const cdr::dom::Node& commandNode,
                             cdr::db::Connection& dbConnection)
{
  cdr::String label_name;

  for (cdr::dom::Node child = commandNode.getFirstChild();
       child != NULL;
       child = child.getNextSibling())
  {
    if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE)
    {
      cdr::String name = child.getNodeName();
      if (name == L"Name")
      {
        cdr::dom::Node n = child.getFirstChild();
        if (n == NULL || n.getNodeType() != cdr::dom::Node::TEXT_NODE)
          throw cdr::Exception(L"Invalid label name");
        label_name = n.getNodeValue();
      }
    }
  }

  if (label_name.size() == 0)
    throw cdr::Exception(L"Invalid or missing label name");

  // we need to serialize.  Save current autocommit state so if the
  // caller has already done so, we don't mess him up
  bool autocommitted = dbConnection.getAutoCommit();
  dbConnection.setAutoCommit(false);

  string query = "SELECT id from version_label WHERE name = ?";
  cdr::db::PreparedStatement select = dbConnection.prepareStatement(query);
  select.setString(1, label_name);
  cdr::db::ResultSet rs = select.executeQuery();
  if (!rs.next())
    throw cdr::Exception(L"Label does not exist");
  
  int label_id = rs.getInt(1);

  string delete_sql = "DELETE FROM doc_version_label "
                      "WHERE label = ?"; 
  cdr::db::PreparedStatement del
        = dbConnection.prepareStatement(delete_sql);
  del.setInt(1, label_id);
  del.executeQuery();

  string delete2_sql = "DELETE FROM version_label "
                      "WHERE id = ?"; 
  cdr::db::PreparedStatement del2
        = dbConnection.prepareStatement(delete2_sql);
  del2.setInt(1, label_id);
  del2.executeQuery();

  dbConnection.setAutoCommit(autocommitted);
  
  return L"<CdrDeleteLabelResp/>";
}

cdr::String cdr::labelDocument(cdr::Session& session,
                               const cdr::dom::Node& commandNode,
                               cdr::db::Connection& dbConnection)
{
  int document_id = 0;
  int version = 0;
  cdr::String label_name;

  for (cdr::dom::Node child = commandNode.getFirstChild();
       child != NULL;
       child = child.getNextSibling())
  {
    if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE)
    {
      cdr::String name = child.getNodeName();
      if (name == L"DocumentId")
      {
        cdr::dom::Node n = child.getFirstChild();
        if (n == NULL || n.getNodeType() != cdr::dom::Node::TEXT_NODE)
          throw cdr::Exception(L"Invalid document ID");
        cdr::String id = n.getNodeValue();
        document_id = id.extractDocId();
      }
      if (name == L"DocumentVersion")
      {
        cdr::dom::Node n = child.getFirstChild();
        if (n == NULL || n.getNodeType() != cdr::dom::Node::TEXT_NODE)
          throw cdr::Exception(L"Invalid version number");
        
        wistringstream vs(cdr::String(n.getNodeValue()));
        vs >> version;
        if (version <= 0)
          throw cdr::Exception(L"Invalid version number");
      }
      if (name == L"LabelName")
      {
        cdr::dom::Node n = child.getFirstChild();
        if (n == NULL || n.getNodeType() != cdr::dom::Node::TEXT_NODE)
          throw cdr::Exception(L"Invalid label name");
        label_name = n.getNodeValue();
      }
    }
  }

  if (document_id <= 0)
    throw cdr::Exception(L"Invalid or missing document ID");
  if (!allowVersion(document_id, dbConnection))
    throw cdr::Exception("Version control  not allowed for document type");
  if (version <= 0)
    throw(L"Invalid or missing version number");
  if (label_name.size() == 0)
    throw cdr::Exception(L"Invalid or missing label name");

  string insert_sql = "INSERT into doc_version_label "
                      "(label, document, num) "
                      "SELECT id, ?, ? "
                      "FROM version_label "
                      "WHERE name = ?";
  cdr::db::PreparedStatement insert
        = dbConnection.prepareStatement(insert_sql);
  insert.setInt(1, document_id);
  insert.setInt(2, version);
  insert.setString(3, label_name);
  insert.executeQuery();
  
  return L"<CdrLabelDocumentResp/>";
}

cdr::String cdr::unlabelDocument(cdr::Session& session,
                               const cdr::dom::Node& commandNode,
                               cdr::db::Connection& dbConnection)
{
  int document_id = 0;
  cdr::String label_name;

  for (cdr::dom::Node child = commandNode.getFirstChild();
       child != NULL;
       child = child.getNextSibling())
  {
    if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE)
    {
      cdr::String name = child.getNodeName();
      if (name == L"DocumentId")
      {
        cdr::dom::Node n = child.getFirstChild();
        if (n == NULL || n.getNodeType() != cdr::dom::Node::TEXT_NODE)
          throw cdr::Exception(L"Invalid document ID");
        cdr::String id = n.getNodeValue();
        document_id = id.extractDocId();
      }
      if (name == L"LabelName")
      {
        cdr::dom::Node n = child.getFirstChild();
        if (n == NULL || n.getNodeType() != cdr::dom::Node::TEXT_NODE)
          throw cdr::Exception(L"Invalid label name");
        label_name = n.getNodeValue();
      }
    }
  }

  if (document_id <= 0)
    throw cdr::Exception(L"Invalid or missing label name");
  if (label_name.size() == 0)
    throw cdr::Exception(L"Invalid or missing label name");

  string delete_sql = "DELETE FROM doc_version_label "
                      "WHERE document = ? "
                      "  AND label = (SELECT id FROM version_label "
                                               "WHERE name = ?)";
  cdr::db::PreparedStatement del
        = dbConnection.prepareStatement(delete_sql);
  del.setInt(1, document_id);
  del.setString(2, label_name);
  del.executeQuery();
  
  return L"<CdrUnlabelDocumentResp/>";
}

