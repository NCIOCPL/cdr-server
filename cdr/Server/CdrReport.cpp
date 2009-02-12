/*
 * $Id: CdrReport.cpp,v 1.25 2009-02-12 14:21:57 bkline Exp $
 *
 * Reporting functions
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.24  2008/10/04 22:01:13  bkline
 * Cleanup of warnings in (possible) migration to Visual Studio 2008.
 *
 * Revision 1.23  2008/01/29 15:17:12  bkline
 * Added report to find patient equivalent of a health professional
 * summary.
 *
 * Revision 1.22  2007/07/27 21:35:05  bkline
 * Added report to collect titles and document IDs for GlossaryTermName
 * documents linked to a specific GlossaryTermConcept document.
 *
 * Revision 1.21  2007/07/10 14:01:21  bkline
 * Added new Term Sets report.
 *
 * Revision 1.20  2006/01/05 16:05:11  bkline
 * Added support for finding document for summary translation.
 *
 * Revision 1.19  2004/11/03 19:14:03  bkline
 * Fixed code formatting.
 *
 * Revision 1.18  2004/11/03 19:00:07  venglisc
 * Modified SQL query for creating the Menu Hierarchy Report by adding the
 * SortOrder attribute value to the output. (Bug 1363)
 *
 * Revision 1.17  2004/10/19 22:02:33  venglisc
 * Modified SQL query for creating the Menu Hierarchy Report by eliminating
 * display of terms with MenuStatus of Offline. (Bug 1363)
 *
 * Revision 1.16  2003/04/15 21:01:22  bkline
 * Added parameter for MenuType in menu tree hierarchy report.
 *
 * Revision 1.15  2003/04/15 18:15:08  bkline
 * Added MenuTermTree report.
 *
 * Revision 1.14  2003/02/14 17:57:42  bkline
 * Fixed typo in query for PUP picklist (issue #595).
 *
 * Revision 1.13  2003/01/02 13:28:59  bkline
 * Fixed bug in report of checked out docs without recent activity.
 *
 * Revision 1.12  2002/09/08 12:27:21  bkline
 * Added PublishLinkedDocs code (not finished or enabled yet).
 *
 * Revision 1.11  2002/08/01 19:17:15  bkline
 * Removed debugging output.
 *
 * Revision 1.10  2002/07/26 23:59:33  bkline
 * Extra parameter for Person Location Picklist.
 *
 * Revision 1.9  2002/06/07 13:53:18  bkline
 * Added missing fragment id for participating org personnel.
 *
 * Revision 1.8  2002/03/14 13:32:22  bkline
 * Modified the dated action report to avoid conflicting database locks.
 *
 * Revision 1.7  2002/03/12 20:42:44  bkline
 * Added report for dated actions.
 *
 * Revision 1.6  2002/02/14 17:59:09  bkline
 * Added code to recognize empty-element tag syntax.
 *
 * Revision 1.5  2002/02/01 22:43:15  bkline
 * Modified logic in LeadOrgPicklist class to avoid duplicates.
 *
 * Revision 1.4  2001/09/19 18:46:07  bkline
 * Added reports to support Protocol customization.
 *
 * Revision 1.3  2001/04/08 22:47:11  bkline
 * Fixed name of response; added DocType element; fixed CDATA delimiter.
 *
 * Revision 1.2  2000/10/26 15:03:17  mruben
 * fixed various bugs
 *
 *
 */

#if defined _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <cstdio>
#include <map>
#include <vector>
#include <sstream>
#include <string>

#include "CdrCommand.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"
#include "CdrException.h"
#include "CdrReport.h"
#include "CdrString.h"
#include "CdrFilter.h"

using std::map;
using std::string;
using std::wistringstream;
using std::wostringstream;

static cdr::String applyNamedFilter(const cdr::String&, 
                                    const cdr::String&,
                                    cdr::FilterParmVector*,
                                    cdr::db::Connection& dbConnection);
static cdr::String getDocumentXml(
              int                    docId,
              cdr::db::Connection&   dbConnection);
static cdr::String getDocVersionXml(
              int                    docId,
              int                    docVersion,
              cdr::db::Connection&   dbConnection);
static cdr::String getNamedFilter(
        const cdr::String&           filterName,
              cdr::db::Connection&   dbConnection);

map<cdr::String, cdr::Report*> cdr::Report::reportMap;

cdr::Report::Report(const cdr::String& name) : reportName(name)
{
  reportMap[name] = this;
}

cdr::Report::~Report()
{
  reportMap[reportName] = NULL;
}

/*****************************************************************************/
/* No default parameters.  Override to set defaults for derived class        */
/*****************************************************************************/
cdr::Report::ReportParameters cdr::Report::defaultParameters()
{
  static ReportParameters r;
  return r;
}

/*****************************************************************************/
/* produce the report                                                        */
/*****************************************************************************/
cdr::String cdr::Report::doReport(cdr::Session& session,
                                  const cdr::dom::Node& commandNode,
                                  cdr::db::Connection& dbConnection)
{
  Report* report = NULL;
  ReportParameters parm;
  
  for (cdr::dom::Node child = commandNode.getFirstChild();
       child != NULL;
       child = child.getNextSibling())
  {
    if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE)
    {
      cdr::String name = child.getNodeName();
      if (name == L"ReportName")
      {
        cdr::dom::Node n = child.getFirstChild();
        if (n == NULL || n.getNodeType() != cdr::dom::Node::TEXT_NODE)
          throw cdr::Exception(L"Missing Report Name");
        cdr::String rname = n.getNodeValue();
        std::map<cdr::String, cdr::Report*>::iterator rpt;
        if ((rpt = reportMap.find(rname)) == reportMap.end()
              || rpt->second == NULL)
          throw cdr::Exception(L"report " + rname + L"not found");

        report = rpt->second;
        parm = report->defaultParameters();
      }
      else
      if (name == L"ReportParams")
      {
        if (report == NULL)
          throw cdr::Exception(L"missing report name");

        for (cdr::dom::Node param = child.getFirstChild();
             param != NULL;
             param = param.getNextSibling())
        {
          if (param.getNodeType() == cdr::dom::Node::ELEMENT_NODE)
          {
            cdr::String pname = param.getNodeName();
            if (pname == L"ReportParam")
            {
              cdr::dom::NamedNodeMap cmdattr = param.getAttributes();
              cdr::dom::Node attrname = cmdattr.getNamedItem("Name");
              cdr::dom::Node attrvalue = cmdattr.getNamedItem("Value");
              if (attrname != NULL)
                if (attrvalue != NULL)
                  parm[attrname.getNodeValue()] = attrvalue.getNodeValue();
                else
                  parm[attrname.getNodeValue()] = L"";
            }
          }
        }
      }
    }
  }

  if (report == NULL)
    throw cdr::Exception(L"missing report name");
  
  return L"<CdrReportResp>"
       + report->execute(session, dbConnection, parm)
       + L"</CdrReportResp>";
}

/*****************************************************************************/
/* command interface to CdrReport class                                      */
/*****************************************************************************/
cdr::String cdr::report(cdr::Session& session,
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& dbConnection)
{
  return cdr::Report::doReport(session, commandNode, dbConnection);
}

namespace
{

  /*************************************************************************/
  /* Inactive Checked Out Documents report                                 */
  /*************************************************************************/
  class Inactive : public cdr::Report
  {
    public:
      Inactive() : cdr::Report("Inactive Checked Out Documents") {}

    private:
      virtual cdr::String execute(cdr::Session& session,
                                  cdr::db::Connection& dbConnection,
                                  cdr::Report::ReportParameters parm);
  };

  /*************************************************************************/
  /* Report on documents locked by specified user.                         */
  /*************************************************************************/
  class LockedDocuments : public cdr::Report
  {
    public:
      LockedDocuments() : cdr::Report("Locked Documents") {}

    private:
      virtual cdr::String execute(cdr::Session& session,
                                  cdr::db::Connection& dbConnection,
                                  cdr::Report::ReportParameters parm);
  };

  /*************************************************************************/
  /* Report for lead organization picklist.                                */
  /*************************************************************************/
  class LeadOrgPicklist : public cdr::Report
  {
    public:
      LeadOrgPicklist() : cdr::Report("Lead Organization Picklist") {}

    private:
      virtual cdr::String execute(cdr::Session& session,
                                  cdr::db::Connection& dbConnection,
                                  cdr::Report::ReportParameters parm);
  };

  /*************************************************************************/
  /* Report for protocol update persons picklist.                          */
  /*************************************************************************/
  class ProtocolUpdatePersonsPicklist : public cdr::Report
  {
    public:
      ProtocolUpdatePersonsPicklist() : 
          cdr::Report("Protocol Update Persons Picklist") {}

    private:
      virtual cdr::String execute(cdr::Session& session,
                                  cdr::db::Connection& dbConnection,
                                  cdr::Report::ReportParameters parm);
  };

  /*************************************************************************/
  /* Report for person locations picklist.                                 */
  /*************************************************************************/
  class PersonLocations : public cdr::Report
  {
    public:
      PersonLocations() : 
          cdr::Report("Person Locations Picklist") {}

    private:
      virtual cdr::String execute(cdr::Session& session,
                                  cdr::db::Connection& dbConnection,
                                  cdr::Report::ReportParameters parm);
  };

  /*************************************************************************/
  /* Report for person address fragment.                                   */
  /*************************************************************************/
  class PersonAddressFragment : public cdr::Report
  {
    public:
      PersonAddressFragment() : 
          cdr::Report("Person Address Fragment") {}

    private:
      virtual cdr::String execute(cdr::Session& session,
                                  cdr::db::Connection& dbConnection,
                                  cdr::Report::ReportParameters parm);
  };

  /*************************************************************************/
  /* Report for protocol participating organizations.                      */
  /*************************************************************************/
  class ParticipatingOrgs : public cdr::Report
  {
    public:
      ParticipatingOrgs() : 
          cdr::Report("Participating Organizations Picklist") {}

    private:
      virtual cdr::String execute(cdr::Session& session,
                                  cdr::db::Connection& dbConnection,
                                  cdr::Report::ReportParameters parm);
  };

  /*************************************************************************/
  /* Dated actions report.                                                 */
  /*************************************************************************/
  class DatedActions : public cdr::Report
  {
    public:
      DatedActions() : 
          cdr::Report("Dated Actions") {}

    private:
      virtual cdr::String execute(cdr::Session& session,
                                  cdr::db::Connection& dbConnection,
                                  cdr::Report::ReportParameters parm);
  };

  /*************************************************************************/
  /* Menu term tree report.                                                */
  /*************************************************************************/
  class MenuTermTree : public cdr::Report
  {
    public:
      MenuTermTree() : 
          cdr::Report("Menu Term Tree") {}

    private:
      virtual cdr::String execute(cdr::Session& session,
                                  cdr::db::Connection& dbConnection,
                                  cdr::Report::ReportParameters parm);
  };

  /*************************************************************************/
  /* Find corresponding translation of English summary.                    */
  /*************************************************************************/
  class TranslatedSummary : public cdr::Report
  {
    public:
      TranslatedSummary() :
          cdr::Report("Translated Summary") {}

    private:
      virtual cdr::String execute(cdr::Session& session,
                                  cdr::db::Connection& dbConnection,
                                  cdr::Report::ReportParameters parm);
  };

  /*************************************************************************/
  /* Find corresponding patient version for HP summary.
  /*************************************************************************/
  class PatientSummary : public cdr::Report
  {
    public:
      PatientSummary() :
          cdr::Report("Patient Summary") {}

    private:
      virtual cdr::String execute(cdr::Session& session,
                                  cdr::db::Connection& dbConnection,
                                  cdr::Report::ReportParameters parm);
  };

  /*************************************************************************/
  /* Find corresponding person document linked from board member document.
  /*************************************************************************/
  class BoardMember : public cdr::Report
  {
    public:
      BoardMember() :
          cdr::Report("Board Member") {}

    private:
      virtual cdr::String execute(cdr::Session& session,
                                  cdr::db::Connection& dbConnection,
                                  cdr::Report::ReportParameters parm);
  };

  /*************************************************************************/
  /* Find GlossaryTermName documents linked to a GlossaryTermConcept doc.
  /*************************************************************************/
  class GlossaryTermNames : public cdr::Report
  {
    public:
      GlossaryTermNames() :
          cdr::Report("Glossary Term Names") {}

    private:
      virtual cdr::String execute(cdr::Session& session,
                                  cdr::db::Connection& dbConnection,
                                  cdr::Report::ReportParameters parm);
  };

  /*************************************************************************/
  /* Collect named sets of CDR terms.                                      */
  /*************************************************************************/
  class TermSets : public cdr::Report
  {
    public:
      TermSets() :
          cdr::Report("Term Sets") {}

    private:
      virtual cdr::String execute(cdr::Session& session,
                                  cdr::db::Connection& dbConnection,
                                  cdr::Report::ReportParameters parm);
  };

#if 0
  /*************************************************************************/
  /* Find linked documents (direct or indirect) except Citations.          */
  /*************************************************************************/
  class PublishLinkedDocs : public cdr::Report
  {
    public:
      PublishLinkedDocs() : 
          cdr::Report("Publish Linked Docs") {}

    private:
      virtual cdr::String execute(cdr::Session& session,
                                  cdr::db::Connection& dbConnection,
                                  cdr::Report::ReportParameters parm);
  };
#endif

  /*
   * Methods ('execute') which specialize the behavior for each report.
   */
  cdr::String Inactive::execute(cdr::Session& session,
                                 cdr::db::Connection& dbConnection,
                                 cdr::Report::ReportParameters parm)
  {
    ReportParameters::iterator i = parm.find(L"InactivityLength");
    if (i == parm.end())
      throw cdr::Exception(L"Must specify InactivityLength");

    // WARNING: this depends on the fact that the date is at the start
    //          of the parameter and that the separator is "-"
    wistringstream inactivity_length(i->second);
    int year;
    int month;
    int day;
    inactivity_length >> year >> month >> day;
    year = -year;

    string query = "SELECT c.id, c.dt_out, u.name, a.dt, ac.name, dt.name "
                   "FROM checkout c "
                   "INNER JOIN usr u "
                   "ON c.usr = u.id "
                   "INNER JOIN audit_trail a "
                   "   ON c.id = a.document "
                   "INNER JOIN action ac "
                   "   ON a.action = ac.id "
                   "INNER JOIN document d"
                   "   ON c.id = d.id "
                   "INNER JOIN doc_type dt"
                   "   ON dt.id = d.doc_type "
                   "WHERE c.dt_in IS NULL "
                   "  AND a.dt < DATEADD(day, ?, "
                   "                     DATEADD(month, ?, "
                   "                             DATEADD(year, ?, "
                   "                                     GETDATE()))) "
                   "  AND c.dt_out < DATEADD(day, ?, "
                   "                     DATEADD(month, ?, "
                   "                             DATEADD(year, ?, "
                   "                                     GETDATE()))) "
                   "  AND a.dt = (SELECT MAX(aa.dt) FROM audit_trail aa "
                   "              WHERE aa.document = a.document) "
                   "ORDER BY c.id ASC";
      
    cdr::db::PreparedStatement select = dbConnection.prepareStatement(query);
    select.setInt(1, day);
    select.setInt(2, month);
    select.setInt(3, year);
    select.setInt(4, day);
    select.setInt(5, month);
    select.setInt(6, year);
    
    cdr::db::ResultSet rs = select.executeQuery();

    wostringstream result;
    result << L"<ReportBody><![CDATA[\n"
              L"<ReportName>" << getName() << L"</ReportName>\n";
    while (rs.next())
    {
      int id = rs.getInt(1);
      cdr::String dt_out = cdr::toXmlDate(rs.getString(2));
      cdr::String name = rs.getString(3);
      cdr::String dt = cdr::toXmlDate(rs.getString(4));
      cdr::String aname = rs.getString(5);
      cdr::String dtype = rs.getString(6);
      result << L"<ReportRow>\n"
                L"<DocId>" << cdr::stringDocId(id) << L"</DocId>\n"
                L"<DocType>" << dtype << L"</DocType>\n"
                L"<CheckedOutTo>" << name << L"</CheckedOutTo>\n"
                L"<WhenCheckedOut>" << dt_out << L"</WhenCheckedOut>\n"
                L"<LastActivity>\n"
                L"<ActionType>" << aname << L"</ActionType>\n"
                L"<ActionWhen>" << dt << L"</ActionWhen>\n"
                L"</LastActivity>\n"
                L"</ReportRow>\n";
    }

    result << L"]]></ReportBody>\n";
    select.close();
    return result.str();
  }

  cdr::String LockedDocuments::execute(cdr::Session& session,
                                       cdr::db::Connection& dbConnection,
                                       cdr::Report::ReportParameters parm)
  {
    ReportParameters::iterator i = parm.find(L"UserId");
    if (i == parm.end())
      throw cdr::Exception(L"Must specify UserId");

    cdr::String userId(i->second);
    string query = "SELECT DISTINCT c.id, t.name, c.dt_out "
                   "           FROM checkout c             "
                   "           JOIN usr u                  "
                   "             ON u.id = c.usr           "
                   "           JOIN document d             "
                   "             ON d.id = c.id            "
                   "           JOIN doc_type t             "
                   "             ON t.id = d.doc_type      "
                   "          WHERE c.dt_in IS NULL        "
                   "            AND u.name = ?             "
                   "       ORDER BY c.id                   ";
      
    cdr::db::PreparedStatement select = dbConnection.prepareStatement(query);
    select.setString(1, userId);
    cdr::db::ResultSet rs = select.executeQuery();
    wostringstream result;
    result << L"<ReportBody><![CDATA[\n"
              L"<ReportName>" << getName() << L"</ReportName>\n";
    while (rs.next())
    {
      int id = rs.getInt(1);
      cdr::String dtype = rs.getString(2);
      cdr::String dt_out = cdr::toXmlDate(rs.getString(3));
      result << L"<ReportRow>\n"
                L"<DocId>" << cdr::stringDocId(id) << L"</DocId>\n"
                L"<DocType>" << dtype << L"</DocType>\n"
                L"<WhenCheckedOut>" << dt_out << L"</WhenCheckedOut>\n"
                L"</ReportRow>\n";
    }

    result << L"]]></ReportBody>\n";
    select.close();
    return result.str();
  }

  cdr::String LeadOrgPicklist::execute(cdr::Session& session,
                                       cdr::db::Connection& dbConnection,
                                       cdr::Report::ReportParameters parm)
  {
    wchar_t* grpTypes[] = {
        L"US clinical trials group",
        //L"Non-US clinical trials group", // Per LG 02Aug2001
        L"Ad hoc group"
    };
    ReportParameters::iterator i = parm.find(L"SearchTerm");
    if (i == parm.end())
      throw cdr::Exception(L"Must specify SearchTerm");
    cdr::String titlePattern = i->second;

    string query = 
        "         SELECT d.id,                                     "
        "                d.title,                                  "
        "                q.value                                   "
        "           FROM document d                                "
        "           JOIN doc_type t                                "
        "             ON d.doc_type = t.id                         "
        "LEFT OUTER JOIN query_term q                              "
        "             ON q.doc_id = d.id                           "
        "            AND q.path = '/Organization/OrganizationType' "
        "          WHERE d.title LIKE ?                            "
        "            AND t.name = 'Organization'                   "
        "       ORDER BY d.title,                                  "
        "                d.id                                      ";
      
    cdr::db::PreparedStatement select = dbConnection.prepareStatement(query);
    select.setString(1, titlePattern);
    
    cdr::db::ResultSet rs = select.executeQuery();

    wostringstream result;
    result << L"<ReportBody><![CDATA[\n"
              L"<ReportName>" << getName() << L"</ReportName>\n";

    // The reason we hold off on writing out a ReportRow element for each row
    // in the result set is that with the join on the query_term table it is
    // possible for the same organization to appear twice because of multiple
    // occurrences of the OrganizationType element.  So we have to bide our
    // time checking each row to find out if any of these occurrences makes
    // our organization eligible to be considered as a Group organization.
    cdr::String currentTitle = L"";
    int         currentId    = 0;
    cdr::String group        = L"No";
    while (rs.next())
    {
      int         id    = rs.getInt(1);
      cdr::String title = rs.getString(2);
      cdr::String type  = rs.getString(3);
      if (id != currentId)
      {
        // Flush the previous organization's data ...
        if (currentId)
        {
          result << L"<ReportRow>\n"
                    L"<DocId>" << cdr::stringDocId(currentId) << L"</DocId>\n"
                    L"<DocTitle>" << currentTitle << L"</DocTitle>\n"
                    L"<Group>" << group << L"</Group>\n"
                    L"</ReportRow>\n";
        }

        // ... and start a new one.
        currentTitle = title;
        currentId    = id;
        group        = L"No";
      }

      // Is the organization a group?
      for (size_t i = 0; i < sizeof grpTypes / sizeof *grpTypes; ++i) {
        if (type == grpTypes[i]) 
        {
          group = L"Yes";
          break;
        }
      }
    }
    select.close();

    // Write out the last organization.
    if (currentId)
    {
      result << L"<ReportRow>\n"
                L"<DocId>" << cdr::stringDocId(currentId) << L"</DocId>\n"
                L"<DocTitle>" << currentTitle << L"</DocTitle>\n"
                L"<Group>" << group << L"</Group>\n"
                L"</ReportRow>\n";
    }
    result << L"]]></ReportBody>\n";
    return result.str();
  }

  cdr::String ProtocolUpdatePersonsPicklist::execute(
          cdr::Session& session,
          cdr::db::Connection& dbConnection,
          cdr::Report::ReportParameters parm)
  {
    ReportParameters::iterator i = parm.find(L"LeadOrg");
    if (i == parm.end())
      throw cdr::Exception(L"Must specify LeadOrg");
    cdr::String leadOrg = i->second;
    string query = 
        "SELECT DISTINCT d.id,                                          "
        "                d.title                                        "
        "           FROM document d                                     "
        "           JOIN query_term t1                                  "
        "             ON (t1.doc_id = d.id)                             "
        "           JOIN query_term t2                                  "
        "             ON (t2.doc_id = d.id)                             "
        "          WHERE t1.path    = '/Person/PersonLocations' +       "
        "                             '/OtherPracticeLocation' +        "
        "                             '/OrganizationLocation/@cdr:ref'  "
        "            AND t1.int_val = ?                                 "
        "            AND t2.path    = '/Person/PersonLocations' +       "
        "                             '/OtherPracticeLocation' +        "
        "                             '/PersonRole'                     "
        "            AND t2.value   = 'Protocol Update Person'          "
        "            AND LEFT(t1.node_loc, 8) = LEFT(t2.node_loc, 8)    ";
      
    cdr::db::PreparedStatement select = dbConnection.prepareStatement(query);
    select.setInt(1, leadOrg.extractDocId());
    
    cdr::db::ResultSet rs = select.executeQuery();

    wostringstream result;
    result << L"<ReportBody><![CDATA[\n"
              L"<ReportName>" << getName() << L"</ReportName>\n";
    while (rs.next())
    {
      int         id    = rs.getInt(1);
      cdr::String title = rs.getString(2);
      result << L"<ReportRow>\n"
                L"<DocId>" << cdr::stringDocId(id) << L"</DocId>\n"
                L"<DocTitle>" << title << L"</DocTitle>\n"
                L"</ReportRow>\n";
    }

    result << L"]]></ReportBody>\n";
    select.close();
    return result.str();
  }

  cdr::String PersonLocations::execute(
          cdr::Session& session,
          cdr::db::Connection& dbConnection,
          cdr::Report::ReportParameters parm)
  {
    ReportParameters::iterator i = parm.find(L"DocId");
    if (i == parm.end())
      throw cdr::Exception(L"Must specify Document ID");
    cdr::String docId = i->second;
    cdr::String privatePracticeOnly = L"no";
    i = parm.find(L"PrivatePracticeOnly");
    if (i != parm.end())
        privatePracticeOnly = i->second;
    cdr::String filterName = L"Person Locations Picklist";
    cdr::FilterParmVector parms;
    parms.push_back(std::pair<cdr::String, cdr::String>(L"docId", docId));
    parms.push_back(std::pair<cdr::String, cdr::String>(L"repName", getName()));
    parms.push_back(std::pair<cdr::String, cdr::String>(L"privatePracticeOnly",
                                                        privatePracticeOnly));
    cdr::String report = applyNamedFilter(filterName, docId, &parms,
                                          dbConnection);
    size_t startTag = report.find(L"<ReportName");
    if (startTag == cdr::String::npos)
      throw cdr::Exception(L"Unable to find ReportName element", report);
    size_t endTag = report.find(L"</ReportBody>", startTag);
    if (endTag == cdr::String::npos)
      throw cdr::Exception(L"Unable to find ReportBody closing tag", report);

    cdr::String result = L"<ReportBody><![CDATA["
                       + report.substr(startTag, endTag - startTag)
                       + L"]]></ReportBody>";

    return result;
  }

  cdr::String PersonAddressFragment::execute(
          cdr::Session& session,
          cdr::db::Connection& dbConnection,
          cdr::Report::ReportParameters parm)
  {
    ReportParameters::iterator i = parm.find(L"Link");
    if (i == parm.end())
      throw cdr::Exception(L"Must specify fragment link");
    cdr::String fragLink = i->second;
    cdr::String filterName = L"Person Address Fragment";
    size_t delimPos = fragLink.find(L"#");
    if (delimPos == cdr::String::npos)
      throw cdr::Exception(L"Fragment ID missing from link", fragLink);
    cdr::String fragId = fragLink.substr(delimPos + 1);
    if (fragId.empty())
      throw cdr::Exception(L"Fragment ID missing from link", fragLink);
    cdr::FilterParmVector parms;
    parms.push_back(std::pair<cdr::String, cdr::String>(L"fragId", fragId));
    cdr::String report = applyNamedFilter(filterName, fragLink, &parms,
                                          dbConnection);
    size_t startTag = report.find(L"<ReportBody");
    if (startTag == cdr::String::npos)
      throw cdr::Exception(L"Unable to find ReportBody element", report);
    size_t tagClose = report.find(L">", startTag);
    if (tagClose == cdr::String::npos)
      throw cdr::Exception(L"Unable to find end of ReportBody tag", report);
    if (report[tagClose - 1] == wchar_t('/'))
        throw cdr::Exception(L"Unable to find address fragment");
    size_t endTag = report.find(L"</ReportBody>", tagClose);
    if (endTag == cdr::String::npos)
      throw cdr::Exception(L"Unable to find ReportBody closing tag", report);

    ++tagClose;
    cdr::String result = L"<ReportBody><![CDATA["
                       + report.substr(tagClose, endTag - tagClose)
                       + L"]]></ReportBody>";

    return result;
  }

  cdr::String ParticipatingOrgs::execute(
          cdr::Session& session,
          cdr::db::Connection& dbConnection,
          cdr::Report::ReportParameters parm)
  {
    ReportParameters::iterator i = parm.find(L"LeadOrg");
    if (i == parm.end())
      throw cdr::Exception(L"Must specify Lead Organization ID");
    cdr::String docId = i->second;
    string call = "{call get_participating_orgs(?)}";
    cdr::db::PreparedStatement ps = dbConnection.prepareStatement(call);
    ps.setInt(1, docId.extractDocId());
    cdr::db::ResultSet rs = ps.executeQuery();

    wostringstream result;
    result << L"<ReportBody><![CDATA[\n"
              L"<ReportName>" << getName() << L"</ReportName>\n";
    int prevId = 0;
    while (rs.next())
    {
      int         poId   = rs.getInt(1);
      cdr::String poName = rs.getString(2);
      cdr::String poPath = rs.getString(3);
      cdr::Int    piId   = rs.getInt(4);
      cdr::String piName = rs.getString(5);
      cdr::String fragId = rs.getString(6);
      if (poId != prevId) 
      {
        cdr::String groupElem;
        if (poPath.find(L"MainMemberOf") != cdr::String::npos)
          groupElem = L"<CoopMember>Main</CoopMember>";
        else if (poPath.find(L"AffiliateMemberOf") != cdr::String::npos)
          groupElem = L"<CoopMember>Affiliate</CoopMember>";
        if (prevId)
          result << L"</ReportRow>";
        result << L"<ReportRow><DocId>"
               << cdr::stringDocId(poId)
               << L"</DocId><DocTitle>"
               << poName
               << L"</DocTitle>"
               << groupElem;
        prevId = poId;
      }
      if (!piName.isNull()) {
        result << L"<PI cdr:ref='"
               << cdr::stringDocId(piId)
               << L"#"
               << fragId
               << L"'>"
               << piName
               << L"</PI>";
      }
    }
    if (prevId)
      result << L"</ReportRow>";

    result << L"]]></ReportBody>\n";
    return result.str();
  }

  cdr::String DatedActions::execute(cdr::Session& session,
                                    cdr::db::Connection& dbConnection,
                                    cdr::Report::ReportParameters parm)
  {
    ReportParameters::iterator i = parm.find(L"DocType");
    if (i == parm.end())
      throw cdr::Exception(L"Must specify DocType");
    cdr::String docType = i->second;

    // Get the XML for the filter we'll use repeatedly.
    cdr::String filterXml = getNamedFilter(L"Dated Actions", dbConnection);

    // Get a list of documents of this type which have dated actions.
    string query = "SELECT DISTINCT q.doc_id,            "
                   "                d.title              "
                   "           FROM query_term q         "
                   "           JOIN document d           "
                   "             ON d.id = q.doc_id      "
                   "           JOIN doc_type t           "
                   "             ON t.id = d.doc_type    "
                   "          WHERE t.name = ?           "
                   "            AND q.path LIKE '%/DatedAction/ActionDate'";
      
    cdr::db::PreparedStatement select = dbConnection.prepareStatement(query);
    select.setString(1, docType);
    cdr::db::ResultSet rs = select.executeQuery();
    cdr::FilterParmVector parms;
    wostringstream result;
    result << L"<ReportBody><![CDATA[\n"
              L"<ReportName>" << getName() << L"</ReportName>\n";

    // Have to collect these and close the query so filter module can get the
    // documents.  Ideally, we'd like to put together a local struct
    // definition to package id+title pairs together, but the C++ language (or
    // at least Microsoft's implementation of it) does not allow for types
    // without external linkage as template arguments.
    std::vector<int> ids;
    std::vector<cdr::String> titles;
    while (rs.next())
    {
      ids.push_back(rs.getInt(1));
      titles.push_back(rs.getString(2));
    }
    select.close();
    std::vector<int>::iterator idIter = ids.begin();
    std::vector<cdr::String>::const_iterator titleIter = titles.begin();
    while (idIter != ids.end())
    {
      cdr::String docXml = getDocumentXml(*idIter, dbConnection);
      result << L"<ReportRow><DocId>"
             << cdr::stringDocId(*idIter++)
             << L"</DocId><DocTitle>"
             << *titleIter++
             << L"</DocTitle>"
             << cdr::filterDocument(docXml, filterXml, dbConnection, 0, &parms)
             << L"</ReportRow>";
    }

    result << L"]]></ReportBody>\n";
    return result.str();
  }

  cdr::String MenuTermTree::execute(cdr::Session& session,
                                    cdr::db::Connection& dbConnection,
                                    cdr::Report::ReportParameters parm)
  {
    // See if the client wants to restrict the report to one menu type.
    cdr::String menuType = "%";
    ReportParameters::iterator i = parm.find(L"MenuType");
    if (i != parm.end())
      menuType = i->second;

    // Pull the menu term information from the database.
    char* query =
        "SELECT DISTINCT a.doc_id  AS term_id,                                "
        "                a.value   AS term_name,                              "
        "                b.value   AS menu_type,                              "
        "                c.value   AS menu_status,                            "
        "                d.value   AS display_name,                           "
        "                e.int_val AS parent,                                 "
        "           CASE WHEN f.value is NULL THEN a.value                    "
        "                                     ELSE f.value                    "
        "            END           AS sort_string                             "
        "           FROM query_term a                                         "
        "           JOIN query_term b                                         "
        "             ON b.doc_id = a.doc_id                                  "
        "           JOIN query_term c                                         "
        "             ON c.doc_id = a.doc_id                                  "
        "            AND LEFT(c.node_loc, 8) = LEFT(b.node_loc, 8)            "
        "LEFT OUTER JOIN query_term d                                         "
        "             ON d.doc_id = a.doc_id                                  "
        "            AND d.path = '/Term/MenuInformation/MenuItem/DisplayName'"
        "            AND LEFT(d.node_loc, 8) = LEFT(b.node_loc, 8)            "
        "LEFT OUTER JOIN query_term e                                         "
        "             ON e.doc_id = a.doc_id                                  "
        "            AND e.path = '/Term/MenuInformation/MenuItem'            "
        "                       + '/MenuParent/@cdr:ref'                      "
        "            AND LEFT(e.node_loc, 8) = LEFT(b.node_loc, 8)            "
        "LEFT OUTER JOIN query_term f                                         "
        "             ON f.doc_id = a.doc_id                                  "
        "            AND f.path = '/Term/MenuInformation/MenuItem/@SortOrder' "
        "            AND LEFT(f.node_loc, 8) = LEFT(b.node_loc, 8)            "
        "          WHERE a.path = '/Term/PreferredName'                       "
        "            AND b.path = '/Term/MenuInformation/MenuItem/MenuType'   "
        "            AND c.path = '/Term/MenuInformation/MenuItem/MenuStatus' "
        "            AND b.value LIKE ?                                       "
        "            AND c.value != 'Offline'                                 "
        "       ORDER BY a.doc_id                                             "
        ;
      
    cdr::db::PreparedStatement ps = dbConnection.prepareStatement(query);
    ps.setString(1, menuType);
    cdr::db::ResultSet rs = ps.executeQuery();
    std::wostringstream result;
    result << L"<ReportBody><![CDATA[\n"
              L"<ReportName>" << getName() << L"</ReportName>\n";
    while (rs.next())
    {
      int         docId       = rs.getInt(1);
      cdr::String termName    = cdr::entConvert(rs.getString(2));
      cdr::String menuType    = cdr::entConvert(rs.getString(3));
      cdr::String menuStatus  = cdr::entConvert(rs.getString(4));
      cdr::String displayName = rs.getString(5);
      cdr::Int    parentId    = rs.getInt(6);
      cdr::String sortString   = cdr::entConvert(rs.getString(7));
      result << L"<MenuItem><TermId>"
             << docId
             << L"</TermId><TermName>"
             << termName
             << L"</TermName><MenuType>"
             << menuType
             << L"</MenuType><MenuStatus>"
             << menuStatus
             << L"</MenuStatus>";
      if (!displayName.isNull() && !displayName.empty())
          result << L"<DisplayName>"
                 << cdr::entConvert(displayName)
                 << L"</DisplayName>";
      if (!parentId.isNull())
          result << L"<ParentId>"
                 << parentId
                 << L"</ParentId>";
      result << L"<SortString>"
             << sortString
             << L"</SortString>";
      result << L"</MenuItem>\n";
    }
    result << L"]]></ReportBody>\n";
    return result.str();
  }

  cdr::String TermSets::execute(cdr::Session& session,
                                cdr::db::Connection& dbConnection,
                                cdr::Report::ReportParameters parm)
  {
    // See if the client wants to restrict the report to one set type.
    cdr::String setType = "%";
    ReportParameters::iterator parmIterator = parm.find(L"SetType");
    if (parmIterator != parm.end())
      setType = parmIterator->second;

    // Retrieve the filter used to extract set names and members.
    cdr::String filterXml = getNamedFilter(L"Get TermSet Name and Members",
                                           dbConnection);

    // Find the sets of the type requested.
    char* query =
        "   SELECT v.id, MAX(v.num)                 "
        "     FROM doc_version v                    "
        "     JOIN query_term t                     "
        "       ON t.doc_id = v.id                  "
        "    WHERE t.path = '/TermSet/TermSetType'  "
        "      AND t.value LIKE ?                   "
        "      AND v.publishable = 'Y'              "
        " GROUP BY v.id;                            ";
      
    cdr::db::PreparedStatement ps = dbConnection.prepareStatement(query);
    ps.setString(1, setType);
    cdr::db::ResultSet rs = ps.executeQuery();
    std::wostringstream result;
    result << L"<ReportBody><![CDATA[\n"
              L"<ReportName>" << getName() << L"</ReportName>\n";
    std::vector<int> docIds;
    std::vector<int> docVersions;
    while (rs.next())
    {
      docIds.push_back(rs.getInt(1));
      docVersions.push_back(rs.getInt(2));
    }
    for (size_t i = 0; i < docIds.size(); ++i)
    {
      int docId          = docIds[i];
      int docVersion     = docVersions[i];
      cdr::String docXml = getDocVersionXml(docId, docVersion, dbConnection);
      result << cdr::filterDocument(docXml, filterXml, dbConnection);
    }
    ps.close();
    result << L"]]></ReportBody>\n";
    return result.str();
  }

#if 0
struct TargetDoc { 
    int id; 
    cdr::String docType; 
    TargetDoc(int i, const cdr::String& dt) : id(i), docType(dt) {}
};

  cdr::String PublishLinkedDocs::execute(cdr::Session& session,
                                         cdr::db::Connection& dbConnection,
                                         cdr::Report::ReportParameters parm)
  {
    ReportParameters::iterator i = parm.find(L"DocType");
    if (i == parm.end())
      throw cdr::Exception(L"Must specify DocType");
    cdr::String docType = i->second;

    // Get the XML for the filter we'll use repeatedly.
    cdr::String filterXml = getNamedFilter(L"Dated Actions", dbConnection);

    // Get a list of documents of this type which have dated actions.
    char * query = "SELECT DISTINCT n.source_doc,        "
                   "                n.target_doc,        "
                   "                t.name               "
                   "           FROM link_net n           "
                   "           JOIN document d           "
                   "             ON d.id = n.target_doc  "
                   "           JOIN doc_type t           "
                   "             ON t.id = d.doc_type    "
                   "          WHERE t.name <> 'Citation' ";
      
    cdr::db::Statement select = dbConnection.createStatement();
    cdr::db::ResultSet rs = select.executeQuery(query);
    std::map<int, std::vector<TargetDoc> > links;
    while (rs.next())
    {
      int sourceDoc = rs.getInt(1);
      int targetDoc = rs.getInt(2);
      cdr::String docType = rs.getString(3);
      TargetDoc td(targetDoc, docType);
      if (links.find(sourceDoc) == links.end())
          links[sourceDoc] = std::vector<TargetDoc>();
      links[sourceDoc].push_back(td);
    }
#if 0
    cdr::FilterParmVector parms;
#endif
    wostringstream result;
    result << L"<ReportBody><![CDATA[\n"
              L"<ReportName>" << getName() << L"</ReportName>\n";

#if 1
    result << L"Built map for " << links.size() << L"linking docs";
#else
    // Have to collect these and close the query so filter module can get the
    // documents.  Ideally, we'd like to put together a local struct
    // definition to package id+title pairs together, but the C++ language (or
    // at least Microsoft's implementation of it) does not allow for types
    // without external linkage as template arguments.
    std::vector<int> ids;
    std::vector<cdr::String> titles;
    while (rs.next())
    {
      ids.push_back(rs.getInt(1));
      titles.push_back(rs.getString(2));
    }
    select.close();
    std::vector<int>::iterator idIter = ids.begin();
    std::vector<cdr::String>::const_iterator titleIter = titles.begin();
    while (idIter != ids.end())
    {
      cdr::String docXml = getDocumentXml(*idIter, dbConnection);
      result << L"<ReportRow><DocId>"
             << cdr::stringDocId(*idIter++)
             << L"</DocId><DocTitle>"
             << *titleIter++
             << L"</DocTitle>"
             << cdr::filterDocument(docXml, filterXml, dbConnection, 0, &parms)
             << L"</ReportRow>";
    }

#endif
    result << L"]]></ReportBody>\n";
    return result.str();
  }
#endif

  // Instantiations of report objects.
  Inactive                      inactive;
  LockedDocuments               lockedDocuments;
  LeadOrgPicklist               leadOrgPicklist;
  ProtocolUpdatePersonsPicklist protocolUpdatePersonsPicklist;
  PersonLocations               personLocations;
  PersonAddressFragment         personAddressFragment;
  ParticipatingOrgs             participatingOrgs;
  DatedActions                  datedActions;
  MenuTermTree                  menuTermTree;
  TranslatedSummary             translatedSummary;
  PatientSummary                patientSummary;
  BoardMember                   boardMember;
  GlossaryTermNames             glossaryTermNames;
  TermSets                      termSets;
#if 0
  PublishLinkedDocs             publishLinkedDocs;
#endif
}

  cdr::String TranslatedSummary::execute(cdr::Session& session,
                                         cdr::db::Connection& dbConnection,
                                         cdr::Report::ReportParameters parm)
  {
    ReportParameters::iterator i = parm.find(L"EnglishSummary");
    if (i == parm.end())
      throw cdr::Exception(L"Must specify EnglishSummary");
    cdr::String englishSummary = i->second;

    string query = "SELECT doc_id                                   "
                   "  FROM query_term                               "
                   " WHERE path = '/Summary/TranslationOf/@cdr:ref' "
                   "   AND int_val = ?                              ";
      
    cdr::db::PreparedStatement select = dbConnection.prepareStatement(query);
    select.setInt(1, englishSummary.extractDocId());
    cdr::db::ResultSet rs = select.executeQuery();

    wostringstream result;
    result << L"<ReportBody><![CDATA[\n"
              L"<ReportName>" << getName() << L"</ReportName>\n";
    if (!rs.next())
      throw cdr::Exception(L"No translated summary found for " +
                           englishSummary);
    int id = rs.getInt(1);
    result << L"<TranslatedSummary>"
           << cdr::stringDocId(id)
           << L"</TranslatedSummary>\n]]></ReportBody>\n";
    select.close();
    return result.str();
  }

  cdr::String PatientSummary::execute(cdr::Session& session,
                                      cdr::db::Connection& dbConnection,
                                      cdr::Report::ReportParameters parm)
  {
    ReportParameters::iterator i = parm.find(L"HPSummary");
    if (i == parm.end())
      throw cdr::Exception(L"Must specify HPSummary");
    cdr::String hpSummary = i->second;

    string query = "SELECT doc_id                                   "
                   "  FROM query_term                               "
                   " WHERE path = '/Summary/PatientVersionOf/@cdr:ref' "
                   "   AND int_val = ?                              ";
      
    cdr::db::PreparedStatement select = dbConnection.prepareStatement(query);
    select.setInt(1, hpSummary.extractDocId());
    cdr::db::ResultSet rs = select.executeQuery();

    wostringstream result;
    result << L"<ReportBody><![CDATA[\n"
              L"<ReportName>" << getName() << L"</ReportName>\n";
    if (!rs.next())
      throw cdr::Exception(L"No patient summary found for " + hpSummary);
    int id = rs.getInt(1);
    result << L"<PatientSummary>"
           << cdr::stringDocId(id)
           << L"</PatientSummary>\n]]></ReportBody>\n";
    select.close();
    return result.str();
  }

  cdr::String BoardMember::execute(cdr::Session& session,
                                   cdr::db::Connection& dbConnection,
                                   cdr::Report::ReportParameters parm)
  {
    ReportParameters::iterator i = parm.find(L"PersonId");
    if (i == parm.end())
      throw cdr::Exception(L"Must specify PersonId");
    cdr::String personId = i->second;

    string query = "SELECT doc_id                             "
                   "  FROM query_term                         "
                   " WHERE path = '/PDQBoardMemberInfo'       "
                   "            + '/BoardMemberName/@cdr:ref' "
                   "   AND int_val = ?                        ";
      
    cdr::db::PreparedStatement select = dbConnection.prepareStatement(query);
    select.setInt(1, personId.extractDocId());
    cdr::db::ResultSet rs = select.executeQuery();

    wostringstream result;
    result << L"<ReportBody><![CDATA[\n"
              L"<ReportName>" << getName() << L"</ReportName>\n";
    if (!rs.next())
      throw cdr::Exception(L"No board member found for " + personId);
    int id = rs.getInt(1);
    result << L"<BoardMember>"
           << cdr::stringDocId(id)
           << L"</BoardMember>\n]]></ReportBody>\n";
    select.close();
    return result.str();
  }

  cdr::String GlossaryTermNames::execute(cdr::Session& session,
                                         cdr::db::Connection& dbConnection,
                                         cdr::Report::ReportParameters parm)
  {
    ReportParameters::iterator i = parm.find(L"ConceptId");
    if (i == parm.end())
      throw cdr::Exception(L"Must specify document ID for glossary term "
                           L"concept document");
    cdr::String conceptId = i->second;

    string query = "SELECT DISTINCT d.id, d.title                            "
                   "           FROM document d                               "
                   "           JOIN query_term n                             "
                   "             ON n.doc_id = d.id                          "
                   "          WHERE n.path = '/GlossaryTermName'             "
                   "                       + '/GlossaryTermConcept/@cdr:ref' "
                   "            AND n.int_val = ?                            ";
      
    cdr::db::PreparedStatement select = dbConnection.prepareStatement(query);
    select.setInt(1, conceptId.extractDocId());
    cdr::db::ResultSet rs = select.executeQuery();

    wostringstream result;
    result << L"<ReportBody><![CDATA[\n"
              L"<ReportName>" << getName() << L"</ReportName>\n";
    while (rs.next()) {
        int id = rs.getInt(1);
        cdr::String name = rs.getString(2);
        result << L"<GlossaryTermName ref='"
               << cdr::stringDocId(id)
               << L"'>"
               << cdr::entConvert(name)
               << L"</GlossaryTermName>\n";
    }
    result << L"]]></ReportBody>\n";
    select.close();
    return result.str();
  }

/**
 * Get XML for a filter by name.
 */
cdr::String getNamedFilter(
        const cdr::String&           filterName,
              cdr::db::Connection&   dbConnection)
{
    string query = " SELECT xml               "
                   "   FROM document d        "
                   "   JOIN doc_type t        "
                   "     ON t.id = d.doc_type "
                   "  WHERE t.name = 'filter' "
                   "    AND d.title = ?       ";
    cdr::db::PreparedStatement ps = dbConnection.prepareStatement(query);
    ps.setString(1, filterName);
    cdr::db::ResultSet rs = ps.executeQuery();
    if (!rs.next())
      throw cdr::Exception(L"Cannot find filter", filterName);
    cdr::String filterXml = rs.getString(1);
    if (rs.next())
      throw cdr::Exception(L"Duplicate filter documents", filterName);
    ps.close();
    return filterXml;
}

/**
 * Get XML for a document by ID.
 */
cdr::String getDocumentXml(
              int                    docId,
              cdr::db::Connection&   dbConnection)
{
    string query = "SELECT xml FROM document WHERE id = ?";
    cdr::db::PreparedStatement ps = dbConnection.prepareStatement(query);
    ps.setInt(1, docId);
    cdr::db::ResultSet rs = ps.executeQuery();
    if (!rs.next()) {
        ps.close();
        throw cdr::Exception(L"Cannot find document", cdr::stringDocId(docId));
    }
    cdr::String docXml = rs.getString(1);
    ps.close();
    return docXml;
}

/**
 * Get XML for a document by ID and Version.
 */
cdr::String getDocVersionXml(
              int                    docId,
              int                    docVersion,
              cdr::db::Connection&   dbConnection)
{
    string query = "SELECT xml FROM doc_version WHERE id = ? AND num = ?";
    cdr::db::PreparedStatement ps = dbConnection.prepareStatement(query);
    ps.setInt(1, docId);
    ps.setInt(2, docVersion);
    cdr::db::ResultSet rs = ps.executeQuery();
    if (!rs.next()) {
        ps.close();
        throw cdr::Exception(L"Cannot find document", cdr::stringDocId(docId));
    }
    cdr::String docXml = rs.getString(1);
    ps.close();
    return docXml;
}

/**
 * Wrapper for Filter module's public filterDocument() function.
 */
cdr::String applyNamedFilter(
        const cdr::String&           filterName,
        const cdr::String&           docId,
              cdr::FilterParmVector* parms,
              cdr::db::Connection&   dbConnection)
{
    cdr::String filterXml = getNamedFilter(filterName, dbConnection);
    cdr::String docXml    = getDocumentXml(docId.extractDocId(), dbConnection);
    return cdr::filterDocument(docXml, filterXml, dbConnection, 0, parms);
}
