/*
 * Reports supported natively by the CDR Server
 */

#ifndef CDR_REPORT_
#define CDR_REPORT_

#include <map>

#include "CdrDbConnection.h"
#include "CdrDom.h"
#include "CdrString.h"

/**@#-*/

namespace cdr {

/**@#+*/

  /** @pkg cdr */

  /**
   * Report object
   *
   * To create a new report object derive a class from cdr::Report and
   * create an instance.  Override the execute member function to produce
   * the report.  Optionally override the defaultParameters member function
   * to generate a map of default parameters.  If defaultParamters() is not
   * overridden, there will be no defaults.
   *
   * The execute member function must return the contents of the
   * <CdrReportResponse> element.
   *
   */
  class Report
  {
    public:
      Report(const String& name);
      virtual ~Report();

      static String doReport(cdr::Session& session,
                             const dom::Node& commandNode,
                             db::Connection& dbConnection);

      const String& getName() { return reportName; }

    protected:
      typedef std::map<String, String> ReportParameters;

    private:
      virtual String execute(cdr::Session& session,
                             db::Connection& dbConnection,
                             ReportParameters parm) = 0;
      virtual ReportParameters defaultParameters();

      const String reportName;
      static std::map<String, Report*> reportMap;
  };
}

#endif
