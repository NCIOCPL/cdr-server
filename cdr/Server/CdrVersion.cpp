/*
 * $Id: CdrVersion.cpp,v 1.28 2006-05-17 03:41:49 ameyer Exp $
 *
 * Version control functions
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.27  2005/07/28 21:15:00  ameyer
 * Backing out some code and comments added inadvertently in
 * fumbled CVS command.
 *
 * Revision 1.25  2004/11/10 03:18:47  ameyer
 * Tested retrieval of blob versions.  Now working.
 *
 * Revision 1.24  2004/11/05 06:02:04  ameyer
 * Introduced numerous changes for new blob handling.
 *
 * Revision 1.23  2003/11/05 01:43:12  ameyer
 * Fixed bug in isChanged().  Was looking at the date/time of the last
 * entry in the audit_trail to compare to date/time of last version.  But
 * should have looked only at ADD or MODIFY transactions in the audit trail.
 *
 * Revision 1.22  2003/08/04 17:03:26  bkline
 * Fixed breakage caused by upgrade to latest version of Microsoft's
 * C++ compiler.
 *
 * Revision 1.21  2003/02/09 21:16:02  bkline
 * Added code to get version date; added code to escape special characters
 * in comment.
 *
 * Revision 1.20  2002/09/29 01:42:20  bkline
 * Fixed typo in closing tag for LastVersionsResp element (missing '/').
 *
 * Revision 1.19  2002/09/25 14:37:46  pzhang
 * Added val_status = 'V' in lastVersions().
 *
 * Revision 1.18  2002/09/25 13:55:18  pzhang
 * Added "val_status = 'V'" in getLatestPublishableVersion().
 *
 * Revision 1.17  2002/06/26 02:22:33  ameyer
 * Added lastVersions().
 *
 * Revision 1.16  2002/06/07 13:52:09  bkline
 * Added support for last publishable linked document retrieval.
 *
 * Revision 1.15  2002/05/03 20:35:46  bkline
 * New CdrListVersions command added.
 *
 * Revision 1.14  2002/04/30 12:36:35  bkline
 * Added workaround for suspicious semantics of `abandoned' parameter.
 *
 * Revision 1.13  2002/04/20 05:04:44  bkline
 * Added code to log unlock actions in the audit_trail table.
 *
 * Revision 1.12  2001/12/14 18:28:38  mruben
 * removed some testing for checkout when checking in
 *
 * Revision 1.11  2001/06/21 23:58:03  ameyer
 * Removed extra question mark from INSERT statement.
 *
 * Revision 1.10  2001/06/05 20:48:02  mruben
 * changed to maintain publishable flag on version
 *
 * Revision 1.9  2001/05/23 01:24:53  ameyer
 * Added actStatus parameter to checkIn to state that doc is publishable
 * ('A'ctive) or non-publishable ('I'nactive).
 * Modified checkVerIn command to look for "Publishable" attribute
 * in command xml to specify the same thing.
 *
 * Revision 1.8  2001/05/04 17:01:17  mruben
 * fixed bug in reading attributes
 *
 * Revision 1.7  2001/05/03 18:43:48  bkline
 * Fixed bug in query (two result set columns reversed).
 *
 * Revision 1.6  2001/02/26 16:09:53  mruben
 * expanded information saved for version
 *
 * Revision 1.5  2000/12/07 22:53:08  ameyer
 * Moved allowVersion from unnamed to cdr namespace so delDoc can see it.
 *
 * Revision 1.4  2000/11/20 15:23:32  mruben
 * fixed bug in SQL
 *
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

#include <iostream> // DEBUG
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
#include "CdrBlobExtern.h"

using std::string;
using std::auto_ptr;
using std::wostringstream;
using std::wistringstream;


static void cdr::getVerBlob(cdr::db::Connection& conn, CdrVerDoc* verdoc);

bool cdr::allowVersion(int docId, cdr::db::Connection& conn)
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

int cdr::checkIn(cdr::Session& session, int docId,
                 cdr::db::Connection& conn,
                 cdr::String publishable, const cdr::String* comment,
                 bool abandon, bool force)
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
      up.setString(2, *comment);
      up.setInt(3, docId);
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

    throw cdr::Exception(L"Document not checked out");
  }

  // now add new version if not abandoned
  if (!abandon)
  {
    // Must have new legal publishable value for new version
    if (publishable != L"Y" && publishable != L"N")
      throw cdr::Exception(
          L"Must specify publishable 'Y' or 'N' when creating new version");

    ++version;
    string query = "SELECT d.val_status, d.val_date, "
                   "       d.doc_type, d.title, d.xml, "
                   "       d.comment, a.dt, b.blob_id "
                   "FROM document d "
                   "JOIN audit_trail a ON a.document = d.id "
                   "LEFT OUTER JOIN doc_blob_usage b ON b.doc_id = d.id "
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

    cdr::String val_status = rs.getString(1);
    cdr::String val_date = rs.getString(2);
    int doc_type = rs.getInt(3);
    cdr::String title = rs.getString(4);
    cdr::String xml = rs.getString(5);
    cdr::String doc_comment = rs.getString(6);
    cdr::String updated_dt = rs.getString(7);
    cdr::Int blobId = rs.getInt(8);

    string newver = "INSERT INTO doc_version "
                    "            (id, num, dt, updated_dt, usr, val_status, "
                    "             val_date, doc_type, title, "
                    "             xml, comment, publishable) "
      "VALUES (?, ?, GETDATE(), ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    cdr::db::PreparedStatement insert = conn.prepareStatement(newver);
    insert.setInt(1, docId);
    insert.setInt(2, version);
    insert.setString(3, updated_dt);
    insert.setInt(4, usr);
    insert.setString(5, val_status);
    insert.setString(6, val_date);
    insert.setInt(7, doc_type);
    insert.setString(8, title);
    insert.setString(9, xml);
    insert.setString(10, doc_comment);
    insert.setString(11, publishable);
    insert.executeQuery();

    // If there was a blob, version it too
    if (!blobId.isNull())
        cdr::addVersionBlobUsage(conn, docId, blobId);
  }

  // Track unlock actions.
  // XXX Note that it is not sufficient to use the `abandon' parameter
  //     to determine whether the checkout of the document has been
  //     abandoned.  This unfortunately-named variable has been over-
  //     loaded to indicate whether a new version should be created
  //     for the document ("!abandoned" means "create a new version").
  //     The workaround here for this problem relies on the fact that
  //     the XMetaL client always sets "force" to true whether or not
  //     the user is unlocking a document checked out to herself.
  //     Not a perfect solution, but it works for this client.
  else if (force)
  {
    const char* const insert = " INSERT INTO audit_trail     "
                               " (                           "
                               "             document,       "
                               "             dt,             "
                               "             usr,            "
                               "             action,         "
                               "             program,        "
                               "             comment         "
                               " )                           "
                               "      SELECT ?,              "
                               "             GETDATE(),      "
                               "             ?,              "
                               "             id,             "
                               "             'CdrCheckIn',   "
                               "             ?               "
                               "        FROM action          "
                               "       WHERE name = 'UNLOCK' ";

    cdr::db::PreparedStatement ps = conn.prepareStatement(insert);
    cdr::String nullString(true);
    ps.setInt   (1, docId);
    ps.setInt   (2, usr);
    ps.setString(3, comment ? *comment : nullString);
    ps.executeUpdate();
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
      // Note forced checkins do not create new versions
      checkIn(session, docId, conn, L"I", NULL, true, force);
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
          fullname = L" (" + fullname + L")";
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
                           + L" at " + dt_out);
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
  string query = "SELECT v.dt, v.updated_dt, v.usr, v.val_status, "
                        "v.val_date, v.doc_type, v.title, "
                        "v.xml, vb.blob_id, v.comment, v.publishable "
                 "FROM doc_version v "
                 "LEFT OUTER JOIN version_blob_usage vb "
                 "  ON v.id = vb.doc_id AND v.num = vb.doc_version "
                 "WHERE v.id = ? and v.num = ?";
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
    verdoc->val_status = rs.getString(4);
    verdoc->val_date = rs.getString(5);
    verdoc->doc_type = rs.getInt(6);
    verdoc->title = rs.getString(7);
    verdoc->xml = rs.getString(8);
    verdoc->blob_id = rs.getInt(9);
    verdoc->comment = rs.getString(10);
    verdoc->publishable = rs.getString(11);

    // Check for blob id and get bytes if there are any
    getVerBlob(conn, verdoc);
  }

  return true;
}

bool cdr::getVersion(int docId, cdr::db::Connection& conn,
                     const cdr::String& dt,
                     CdrVerDoc* verdoc)
{
  string query = "SELECT v.num, v.dt, v.updated_dt, v.usr, v.val_status, "
                        "v.val_date, v.doc_type, v.title, "
                        "v.xml, vb.blob_id, v.comment, v.publishable "
                 "FROM doc_version v "
                 "LEFT OUTER JOIN version_blob_usage vb "
                 "  ON v.id = vb.doc_id "
                 " AND v.num = vb.doc_version "
                 "WHERE id = ? "
                 "  AND v.dt = (SELECT MAX(dt) FROM doc_version "
                 "               WHERE id = ? AND dt <= ?)";
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
    verdoc->num = rs.getInt(1);
    verdoc->dt = rs.getString(2);
    verdoc->updated_dt = cdr::toXmlDate(rs.getString(3));
    verdoc->usr = rs.getInt(4);
    verdoc->val_status = rs.getString(5);
    verdoc->val_date = rs.getString(6);
    verdoc->doc_type = rs.getInt(7);
    verdoc->title = rs.getString(8);
    verdoc->xml = rs.getString(9);
    verdoc->blob_id = rs.getInt(10);
    verdoc->comment = rs.getString(11);
    verdoc->publishable = rs.getString(12);

    // Search for blob data, if needed
    getVerBlob(conn, verdoc);
  }

  return true;
}

bool cdr::getLabelVersion(int docId, cdr::db::Connection& conn,
                          const cdr::String& label_name,
                          CdrVerDoc* verdoc)
{
  string query = "SELECT d.num, d.dt, d.updated_dt, d.usr, d.val_status, "
                        "d.val_date, d.doc_type, d.title, "
                        "d.xml, b.blob_id, d.comment, d.publishable "
                        "FROM doc_version d "
                        "INNER JOIN doc_version_label dl "
                        "   ON d.id = dl.document "
                        "INNER JOIN version_label vl "
                        "   ON dl.label = vl.id "
                        "LEFT OUTER JOIN version_blob_usage b "
                        "  ON d.id = b.doc_id "
                        " AND dl.num = b.doc_version "
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
    verdoc->val_status = rs.getString(5);
    verdoc->val_date = rs.getString(6);
    verdoc->doc_type = rs.getInt(7);
    verdoc->title = rs.getString(8);
    verdoc->xml = rs.getString(9);
    verdoc->blob_id = rs.getInt(10);
    verdoc->comment = rs.getString(11);
    verdoc->publishable = rs.getString(12);

    // Check for blob id and get bytes if there are any
    getVerBlob(conn, verdoc);
  }

  return true;
}

int cdr::getVersionNumber(int docId, cdr::db::Connection& conn,
                          cdr::String* date, const cdr::String& maxDate)
{
  string qry=
    "SELECT v.num, v.updated_dt FROM document d "
    "  LEFT OUTER JOIN doc_version v "
    "    ON v.id = d.id AND v.num = (SELECT MAX(num) FROM doc_version "
    "                        WHERE (id = d.id and updated_dt <= ?)) "
    "  WHERE d.id = ?";

  // A simpler, faster version below, but could retrieve a deleted doc
  // string qry="select max(num) from doc_version where id=? and updated_dt<?";

  cdr::db::PreparedStatement stmt = conn.prepareStatement(qry);
  stmt.setString(1, maxDate);
  stmt.setInt(2, docId);

  try {
    cdr::db::ResultSet rs = stmt.executeQuery();
    if (rs.next())
    {
      if (date != NULL)
        *date = cdr::toXmlDate(rs.getString(2));
      int verNum = rs.getInt(1);
      return verNum;
    }
  }
  catch(const cdr::Exception &e) {
    // Rethrow, but with information about where it happened
    throw cdr::Exception(L"getVersionNumber failure: " + e.what());
  }

  return -1;
}

int cdr::getLatestPublishableVersion(int docId, cdr::db::Connection& conn,
                                     cdr::String* date,
                                     const cdr::String& maxDate)
{
  string query = "SELECT v.num, v.dt                      "
                 "  FROM doc_version v                    "
                 " WHERE v.id = ?                         "
                 "   AND v.num = (SELECT MAX(num)         "
                 "                  FROM doc_version      "
                 "                 WHERE id = v.id        "
                 "                   AND val_status = 'V' "
                 "                   AND publishable = 'Y'";

  // Add date limit if requested
  if (maxDate == DFT_VERSION_DATE)
      query += ")";
  else
      query += " AND updated_dt <= ?)";

  cdr::db::PreparedStatement select = conn.prepareStatement(query);
  select.setInt(1, docId);
  if (maxDate != DFT_VERSION_DATE)
      select.setString(2, maxDate);
  cdr::db::ResultSet rs = select.executeQuery();
  if (rs.next())
  {
    if (date != NULL)
      *date = cdr::toXmlDate(rs.getString(2));

      int verNum = rs.getInt(1);
    return verNum;
  }

  return -1;
}

bool cdr::isChanged(int docId, cdr::db::Connection& conn)
{
  // Compare updated_dt datetime on last version to datetime in audit_trail
  //   for last action that was an add or modify
  // If a match is found, CWD = last version, else CWD last saved without
  //   a version.
  string query = "SELECT num FROM doc_version d "
                 "INNER JOIN audit_trail a ON d.id = a.document "
                 "WHERE d.updated_dt = a.dt "
                 "  AND d.id = ? "
                 "  AND d.num = (SELECT MAX(dd.num) from doc_version dd"
                 "                  WHERE dd.id = ?)"
                 "  AND a.dt = (SELECT MAX(aa.dt) from audit_trail aa "
                 "                JOIN action act on aa.action = act.id "
                 "               WHERE aa.document = ? "
                 "       AND act.name in ('ADD DOCUMENT', 'MODIFY DOCUMENT'))";

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
  if (attr != NULL && cdr::String(attr.getNodeValue()) == L"Y")
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
  cdr::String publishable(L"N");
  int version = -1;
  auto_ptr<cdr::String> comment;
  cdr::String ui_string;

  cdr::dom::NamedNodeMap cmdattr = commandNode.getAttributes();
  cdr::dom::Node attr = cmdattr.getNamedItem("Abandon");
  if (attr != NULL && cdr::String(attr.getNodeValue()) == L"Y")
    abandon = true;
  attr = cmdattr.getNamedItem("ForceCheckIn");
  if (attr != NULL && cdr::String(attr.getNodeValue()) == L"Y")
    force = true;

  attr = cmdattr.getNamedItem("Publishable");
  if (attr != NULL)
    publishable = cdr::String(attr.getNodeValue());

  // If creating a new version, must specify publishable
  if (!abandon && publishable != L"Y" && publishable != L"N")
    throw cdr::Exception(
            L"Must specify Publishable='Y' or 'N' when creating new version");

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
                    publishable, comment.get(), abandon, force);

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
    throw cdr::Exception(L"Invalid or missing document ID");
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

cdr::String cdr::listVersions(Session& session,
                              const dom::Node& commandNode,
                              db::Connection& dbConnection)
{
  int docId     = 0;
  int nVersions = -1;

  for (dom::Node child = commandNode.getFirstChild();
       child != NULL;
       child = child.getNextSibling())
  {
    if (child.getNodeType() == dom::Node::ELEMENT_NODE)
    {
      String name = child.getNodeName();
      if (name == L"DocId")
      {
        String str = dom::getTextContent(child);
        docId      = str.extractDocId();
      }
      if (name == L"NumVersions")
      {
        String str = dom::getTextContent(child);
        nVersions  = str.getInt();
        if (nVersions == 0)
          throw Exception(L"Invalid value for NumVersions", str);
      }
    }
  }

  if (docId <= 0)
    throw Exception(L"Invalid or missing document ID");

  char top[80];
  char query[256];
  *top = '\0';
  if (nVersions > 0)
      sprintf(top, "TOP %d ", nVersions);
  sprintf(query, "SELECT %snum,      "
                 "       dt,         "
                 "       comment     "
                 "  FROM doc_version "
                 " WHERE id = ?      "
                 " ORDER BY num DESC ", top);
  db::PreparedStatement ps = dbConnection.prepareStatement(query);
  ps.setInt(1, docId);
  db::ResultSet rs = ps.executeQuery();
  wostringstream response;
  response << L"<ListVersionsResp>";
  while (rs.next())
  {
    int num        = rs.getInt(1);
    String dt      = rs.getString(2);
    String comment = rs.getString(3);
    response << L"<Version><Num>"
             << String::toString(num)
             << L"</Num><Date>"
             << dt.c_str()
             << L"</Date>";
    if (!comment.isNull())
      response << L"<Comment>"
               << entConvert(comment.c_str())
               << L"</Comment>";
    response << L"</Version>";
  }
  response << L"</ListVersionsResp>";

  return response.str();
}

// See CdrCommand.h
cdr::String cdr::lastVersions(Session& session,
                              const dom::Node& commandNode,
                              db::Connection& dbConnection)
{
  int docId   = -1;     // Doc identifier
  int lastAny = -1;     // Version num of last version, -1 if none
  int lastPub = -1;     // Version num of last publishable version or -1
                        //  may be same as last version

  // Parse input XML command
  for (dom::Node child = commandNode.getFirstChild();
       child != NULL;
       child = child.getNextSibling())
  {
    if (child.getNodeType() == dom::Node::ELEMENT_NODE)
    {
      String name = child.getNodeName();
      if (name == L"DocId")
      {
        String str = dom::getTextContent(child);
        docId      = str.extractDocId();
      }
    }
  }

  if (docId <= 0)
    throw Exception(L"Invalid or missing document ID");

  // Prepare response
  wostringstream response;
  response << L"<LastVersionsResp>\n";

  // Find last version of this document, publishable or not
  db::PreparedStatement anyPs = dbConnection.prepareStatement(
     "SELECT max(num) FROM doc_version WHERE id = ?");
  anyPs.setInt(1, docId);
  db::ResultSet anyRs = anyPs.executeQuery();
  if (anyRs.next()) {
    lastAny = anyRs.getInt(1);
    if (lastAny == 0)
        lastAny = -1;
  }

  response << L" <LastVersionNum>"
           << String::toString (lastAny)
           << L"</LastVersionNum>\n";

  // Find last publishable version, if any
  db::PreparedStatement pubPs = dbConnection.prepareStatement(
     "SELECT max(num) FROM doc_version WHERE id = ? "
     "AND val_status = 'V' AND publishable = 'Y'");
  pubPs.setInt(1, docId);
  db::ResultSet pubRs = pubPs.executeQuery();
  if (pubRs.next()) {
    lastPub = pubRs.getInt(1);
    if (lastPub == 0)
        lastPub = -1;
  }

  response << L" <LastPubVersionNum>"
           << String::toString (lastPub)
           << L"</LastPubVersionNum>\n";

  // Is the last version the same as the current working document
  String chg = isChanged(docId, dbConnection) ? L"Y" : L"N";
  response << L" <IsChanged>"
           << chg
           << L"</IsChanged>\n";

  // Return response
  response << L"</LastVersionsResp>\n";

  return response.str();
}

/*
 * Fill in the blob data in a CdrVerDoc structure, if and only if
 * verdoc->blob_id is not null.
 */
static void cdr::getVerBlob(cdr::db::Connection& conn, CdrVerDoc* verdoc)
{
    // If there is a blob for this version
    if (!verdoc->blob_id.isNull()) {

        // Get pointer to dynamically allocated blob
        cdr::Blob *pBlob;
        if ((pBlob = cdr::getBlobPtrByBlobId(conn, verdoc->blob_id)) == 0) {
            wchar_t msg[128];
            swprintf (msg, L"getVerBlob: version has blob_id but not blob.  "
                           L"DocId=%d, Version=%d, BlobId=%d",
                           verdoc->document, verdoc->num, verdoc->blob_id);
            throw cdr::Exception (msg);
        }

        // Deep copy from heap to verdoc memory
        // Takes more memory and milliseconds than optimum, but shouldn't
        //   happen too often.
        // If it becomes a problem, can optimize with a lazy evaluation,
        //   i.e., only fetch the actual blob before returning to the
        //   client - if the client has actually requested it.
        // But that would require more change to running code and may not
        //   be necessary.
        verdoc->data = cdr::Blob(*pBlob);

        // Release heap memory
        delete pBlob;
    }
}
