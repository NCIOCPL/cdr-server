/*
 * $Id: CdrReport.cpp,v 1.2 2000-10-26 15:03:17 mruben Exp $
 *
 * Reporting functions
 *
 * $Log: not supported by cvs2svn $
 *
 */

#if defined _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <cstdio>
#include <map>
#include <sstream>
#include <string>

#include "CdrCommand.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"
#include "CdrException.h"
#include "CdrReport.h"
#include "CdrString.h"

using std::map;
using std::string;
using std::wistringstream;
using std::wostringstream;

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
          throw cdr::Exception(L"Invalid document ID");
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
  
  return L"<CdrReportResponse>"
       + report->execute(session, dbConnection, parm)
       + L"</CdrReportResponse>";
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

/*****************************************************************************/
/* Inactive Checked Out Documents report                                     */
/*****************************************************************************/

namespace
{
  class Inactive : public cdr::Report
  {
    public:
      Inactive() : cdr::Report("Inactive Checked Out Documents") {}

    private:
      virtual cdr::String execute(cdr::Session& session,
                                  cdr::db::Connection& dbConnection,
                                  cdr::Report::ReportParameters parm);
  };

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

    string query = "SELECT c.id, c.dt_out, u.name, a.dt, ac.name "
                   "FROM checkout c "
                   "INNER JOIN usr u "
                   "ON c.usr = u.id "
                   "INNER JOIN audit_trail a "
                   "   ON c.id = a.document "
                   "INNER JOIN action ac "
                   "   ON a.action = ac.id "
                   "WHERE c.dt_in IS NULL "
                   "  AND a.dt < DATEADD(day, ?, "
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
      result << L"<ReportRow>\n"
                L"<DocId>" << id << L"</DocId>\n"
                L"<CheckedOutTo>" << name << L"</CheckedOutTo>\n"
                L"<WhenCheckedOut>" << dt << L"</WhenCheckedOut>\n"
                L"<LastActivity>\n"
                L"<ActionType>" << aname << L"</Actiontype>\n"
                L"<ActionWhen>" << dt << L"</ActionWhen>\n"
                L"</LastActivity>\n"
                L"</ReportRow>\n";
    }

    result << L"]]</ReportBody>\n";
    return result.str();
  }

  Inactive inactive;
}
