/*
 * $Id: CdrFilter.cpp,v 1.59 2008-10-16 20:26:38 ameyer Exp $
 *
 * Applies XSLT scripts to a document
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.58  2008/08/27 02:38:31  ameyer
 * Added functionality to lookupExternalValue() to make values non-mappable
 * if they match anything in the external_map_nomap_pattern table.
 *
 * Revision 1.57  2008/08/01 02:31:04  ameyer
 * Fixed incorrectly escaped parenthesis chars in regex.
 *
 * Revision 1.56  2007/08/03 19:06:11  bkline
 * Fixed serious memory allocation bug in parmstr function().
 *
 * Revision 1.55  2007/03/14 23:58:23  ameyer
 * Changed slashes to tildes to delimit query arguments in execXsltSqlQuery.
 * We need to be able to pass strings with slashes into SQL Server.
 *
 * Revision 1.54  2007/03/14 23:10:38  ameyer
 * Added cdrutil:/sql-query, plus some additional documentation.
 *
 * Revision 1.53  2006/09/01 02:07:54  ameyer
 * Added minor safety check on mappability.
 *
 * Revision 1.52  2006/05/04 22:45:47  ameyer
 * Numerous changes were introduced to support date-time limited filtering.
 * This is an interim release of these changes.  It appears to work and,
 * as far as we know, can be put into production, but more changes will come.
 * Some debugging statements for date-time limits have been left in the
 * source, but commented out.
 *
 * Revision 1.51  2005/12/09 17:03:01  bkline
 * Fixed typo (was assigning the wrong value to verificationDate variable).
 *
 * Revision 1.50  2005/10/27 12:37:58  bkline
 * Support for new function to calculate an artificial "verification
 * date" added.
 *
 * Revision 1.49  2005/08/02 15:00:04  ameyer
 * Put more try/catch wrappers around Sablotron calls.
 * Purpose is to better identify any internal Sablotron exceptions for
 * which identity would be lost if they just bubbled up to the top.
 *
 * Revision 1.48  2005/06/21 13:06:20  bkline
 * Added support for new 'bogus' column in external_map table.
 *
 * Revision 1.47  2005/03/04 02:53:39  ameyer
 * Added new parameter to parser constructor to ensure that parser memory
 * for filters does not hang around beyond its need.  Part of conversion
 * from xml4c to Xerces.
 *
 * Revision 1.46  2004/05/14 02:20:54  ameyer
 * Added cdrutil protocol callback for function denormalizeTerm.
 *
 * Revision 1.45  2004/04/30 01:35:03  ameyer
 * Added code, normally dormant, for timing filter execution by filter id.
 *
 * Revision 1.44  2004/03/31 03:05:53  ameyer
 * Took toUtf8() out from inside function call parentheses.
 * Bob has seen an MSVC bug in the past that makes this safer.
 *
 * Revision 1.43  2004/03/12 00:30:30  bkline
 * Replaced calls to obsolete Sablotron APIs.
 *
 * Revision 1.42  2004/02/20 00:34:42  ameyer
 * Removed a debugging line inadvertently left in.
 *
 * Revision 1.41  2004/02/19 22:12:36  ameyer
 * Reorganized code in order to add new function filterDocumentByScriptSetName().
 * Took some code out of existing functions in order to centralize filter
 * set processing, whether being called from a C++ function or by the command
 * processor to process an XML transaction.
 *
 * Revision 1.40  2003/11/18 16:29:54  bkline
 * Added code to insert unmapped values into the external_map table
 * so the External Map Failure report would be able to let the users
 * know what still needs mapping.
 *
 * Revision 1.39  2003/11/05 22:28:32  bkline
 * Added support for new extern-map function.
 *
 * Revision 1.38  2003/09/09 19:25:01  bkline
 * Added new custom function for validating U.S. ZIP codes.
 *
 * Revision 1.37  2003/08/04 17:03:26  bkline
 * Fixed breakage caused by upgrade to latest version of Microsoft's
 * C++ compiler.
 *
 * Revision 1.36  2003/06/10 20:06:50  ameyer
 * Modified repFilterSet to return <CdrRepFilterSetResp> instead of
 * <...Add...>.
 *
 * Revision 1.35  2003/06/05 15:36:32  bkline
 * Added new command for determining the last publishable version of
 * a CDR document.
 *
 * Revision 1.34  2003/03/14 02:01:40  bkline
 * Fixed garbage returns from cdr::String::getInt().
 *
 * Revision 1.33  2002/11/19 22:43:32  bkline
 * Fixed code for last and lastp version specifications to throw an error
 * if the requested version is not found.
 *
 * Revision 1.32  2002/11/14 13:23:58  bkline
 * Changed CdrFilter command to use filter sets.  Added CdrDelFilterSet
 * command.
 *
 * Revision 1.31  2002/11/12 11:44:37  bkline
 * Added filter set support.
 *
 * Revision 1.30  2002/10/03 02:56:08  bkline
 * Added a custom function to map a Cancer.gov GUID to a pretty URL.
 *
 * Revision 1.29  2002/09/29 01:43:20  bkline
 * Fixed bug in code to extract CDATA section (wasn't checking for missing
 * node).
 *
 * Revision 1.28  2002/09/07 18:14:53  bkline
 * Turned on const char** casts for Sablotron again.
 *
 * Revision 1.27  2002/09/04 22:01:58  bkline
 * Backing out upgrade to Sablotron 0.95 to avoid bug.
 *
 * Revision 1.26  2002/09/04 18:58:17  bkline
 * Upgraded Sablotron to 0.95.
 *
 * Revision 1.25  2002/07/15 18:55:08  bkline
 * Backed out casts until next upgrade of Sablotron.
 *
 * Revision 1.24  2002/07/12 20:29:24  bkline
 * Added DocTitle direct access.
 *
 * Revision 1.23  2002/06/07 13:52:10  bkline
 * Added support for last publishable linked document retrieval.
 *
 * Revision 1.22  2002/05/21 19:08:55  bkline
 * Implemented support for finding documents by title in URI.
 *
 * Revision 1.21  2002/04/04 19:06:12  bkline
 * Eliminated unused variable.  Fixed typo in comment.
 *
 * Revision 1.20  2002/03/07 12:58:52  bkline
 * Added missing break statement in switch (message handler).
 *
 * Revision 1.19  2002/03/07 02:03:21  bkline
 * Delayed throwing an exception until we are no longer in a Sablotron
 * callback function (which was causing a memory leak).
 *
 * Revision 1.18  2002/02/19 22:44:59  bkline
 * Added support for version attribute on Document element.
 *
 * Revision 1.17  2002/02/01 22:08:01  bkline
 * Fixed whitespace in XSLT error messages.
 *
 * Revision 1.16  2002/01/31 21:35:09  mruben
 * changed default format for current date
 *
 * Revision 1.15  2002/01/23 18:23:13  mruben
 * Changed components of CdrDocCtl from uri
 *
 * Revision 1.14  2002/01/08 18:19:12  mruben
 * Modified for reentrance.
 * Added support for nonstandard scheme cdrutil:
 *
 * Revision 1.13  2001/09/21 03:45:53  ameyer
 * Added filterDocumentByScriptId and fitlerDocumentByScriptTitle
 *
 * Revision 1.12  2001/09/20 20:13:31  mruben
 * added code for accessing versions -- needs testing
 *
 * Revision 1.11  2001/07/12 19:35:06  mruben
 * fixed error in message
 *
 * Revision 1.10  2001/06/19 18:54:29  mruben
 * addes API call and support for parms
 *
 * Revision 1.9  2001/05/08 15:00:00  mruben
 * fixed bug in generating filter results
 *
 * Revision 1.8  2001/05/04 17:00:49  mruben
 * added attribute to skip output
 *
 * Revision 1.7  2001/04/05 23:10:02  mruben
 * added specifying filter by name
 *
 * Revision 1.6  2001/03/19 17:17:43  mruben
 * added support for xsl:message
 *
 * Revision 1.5  2001/03/13 22:15:09  mruben
 * added ability to use CdrDoc element for filter
 *
 * Revision 1.4  2001/02/26 16:09:27  mruben
 * fixed error in exception safety
 *
 * Revision 1.3  2000/09/25 14:00:14  mruben
 * added cdrx protocol for XSLT URIs
 *
 * Revision 1.2  2000/08/24 13:43:11  mruben
 * added support for cdr: URIs
 *
 * Revision 1.1  2000/08/23 14:19:03  mruben
 * Initial revision
 */

#if defined _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <iostream> // Debug
#include <cstdio>
#include <sstream>
#include <ctime>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <winsock.h>

#include "sablot.h"

#include "CdrCommand.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"
#include "CdrException.h"
#include "CdrFilter.h"
#include "CdrGetDoc.h"
#include "CdrString.h"
#include "CdrVersion.h"
#include "CdrRegEx.h"
#include "CdrTiming.h"
#include "CdrCache.h"

using std::pair;
using std::string;
using std::vector;
using std::wostringstream;

const int MAX_FILTER_SET_DEPTH = 20;

static int getDocIdFromTitle(const cdr::String& title,
                             cdr::db::Connection& connection);
static cdr::String getDocTitle(const cdr::String& id,
                               cdr::db::Connection& connection);
static cdr::String getPrettyUrl(const cdr::String& guid);

static void getFiltersInSet(const cdr::String& name,
                            std::vector<int>& filterSet,
                            cdr::db::Connection& conn);

static void getFiltersInSet(int,
                            std::vector<int>& filterSet,
                            int depth,
                            cdr::db::Connection& conn);
static string getPubVerNumber(const string&,
                              cdr::db::Connection&);
static string getValidZip(const string&,
                          cdr::db::Connection&);
static string lookupExternalValue(const string&,
                                  cdr::db::Connection&);
static void getFilterSetXml (cdr::db::Connection&, cdr::String, cdr::String,
                             vector<cdr::String>&  filters,
                             const cdr::String& maxFilterDate =
                                   cdr::DFT_VERSION_DATE);
static string execXsltSqlQuery(const string&, cdr::db::Connection&);
static std::string filterVector (
        cdr::String            document,
        cdr::db::Connection&   connection,
        vector<cdr::String>&   filterSet,
        std::string*           messages,
        cdr::FilterParmVector* parms,
        cdr::String            doc_id,
        const cdr::String&     maxDocDate = cdr::DFT_VERSION_DATE,
        const cdr::String&     maxFilterDate = cdr::DFT_VERSION_DATE);
static string getVerificationDate(const string& parms,
                                  cdr::db::Connection& conn,
                                  const cdr::String&);
static string execXsltDedupIDs(const string&);
static int parseIDs(char *, std::vector<char *>&);

static void makeNomapRegex (cdr::RegEx& re, cdr::db::Connection& conn);

// Map of filter strings to ids
// Used only if filter profiling is specified by environment variable
static std::map<std::string, int> *S_pFltrIdMap = 0;

// RegEx pattern for finding SQL commands that can modify the
//  database.  Searches case insensitively for keyword+whitespace
static cdr::RegEx S_unsafeWordsPat(
        ".*(insert\\s|update\\s|delete\\s|create\\s"
            "|alter\\s|exec[\\(\\s]|execute[\\(\\s]).*", true);

namespace
{
  // decode URI escapes; supports % escapes and '+' to ' ' conversion
  string uri_decode(string s)
  {
    string result;
    result.reserve(s.size());
    for (const char* p = s.c_str(); *p != '\0'; ++p)
    {
      switch (*p)
      {
        case '+':
          result += ' ';
          break;

        case '%':
        {
          unsigned c = 0;
          int n;
          sscanf(p + 1, "%2x%n", &c, &n);
          if (n == 0)
            c = '%';
          result += static_cast<char>(c);
          p += n;
          break;
        }

        default:
          result += *p;
      }
    }

    return result;
  }

  // What follows is Mike's humorous trick for creating two
  //   constants.  The values are the same, but the addresses
  //   are different.
  // CdrDocType != CdrFilterType because the names refer to addresses,
  //   not values.

  // unique type used to get CdrDocCtl
  const wchar_t CdrDocType[] = L"";

  // unique type used to get filter by name
  const wchar_t CdrFilterType[] = L"";

  // Class used as to hold a vector of pairs of parameter name + value.
  // Provides automatic cleanup.
  class ParmList
  {
    public:
      ~ParmList();
      void addparm(char* name, char* value);
      char** g_parms() { return parms.size() == 0 ? NULL : &parms[0]; }

    private:
      vector<char*> parms;
  };

  ParmList::~ParmList()
  {
    for (std::vector<char*>::iterator i = parms.begin();
         i != parms.end();
         ++i)
    {
      delete[] *i;
      *i = NULL;
    }
  }

  /***************************************************************************/
  /* returns pointer to UTF-8 string of data in s.  Note that this string    */
  /* is owned by the caller and must be delete[]ed.  (This happens           */
  /* automatically in the ParmList destructor - AHM.)                        */
  /***************************************************************************/
  char* parmstr(const cdr::String& s)
  {
    string us = s.toUtf8();
    char* p = new char[us.length() + 1];
    strcpy(p, us.c_str());
    return p;
  }

  void ParmList::addparm(char* name, char* value)
  {
      parms.push_back(name);
      parms.push_back(value);
  }

  /***************************************************************************/
  /* get document from the repository.  Error if type not NULL and document  */
  /* does not have this type                                                 */
  /***************************************************************************/
  cdr::String getDocument(cdr::String ui,
                          int version,
                          cdr::db::Connection& connection,
                          const wchar_t* type = NULL,
                          const cdr::String& maxDate = cdr::DFT_VERSION_DATE)
  {
    if (type == CdrDocType) {
      //std::cout << "Returning getDocString with maxDate\n"; // DEBUG
      return cdr::getDocString(ui, connection, version, true, false,
                               true, false, maxDate);
    }

    cdr::String docstring(cdr::getDocString(ui, connection, version, true,
                                            //type != CdrFilterType));
                                            false));

    // Force this parser to release its DOM tree when it goes out of scope
    // We could accumulate hundreds of these trees if we don't do this
    //   and we don't need them to hang around after getDocument returns
    cdr::dom::Parser parser(false);

    parser.parse(docstring);
    cdr::dom::Document doc = parser.getDocument();
    for (cdr::dom::Node node = doc.getFirstChild().getFirstChild();
         node != NULL;
         node = node.getNextSibling())
    {
      cdr::String name = node.getNodeName();
      if (type != NULL && name == L"CdrDocCtl")
      {
        for (cdr::dom::Node child = node.getFirstChild();
             child != NULL;
             child = child.getNextSibling())
          if (child.getNodeName() == L"DocType"
                  && child.getNodeValue() != type)
            throw cdr::Exception(cdr::String(
                          L"Invalid DocType in filter. Type is ")
                             + cdr::String(child.getNodeValue())
                             + L", expected " + type);
      }
      else
      if (name == L"CdrDocXml"
              && node.getNodeType() == cdr::dom::Node::ELEMENT_NODE)
      {
        cdr::dom::Node xml = node.getFirstChild();
        if (xml.getNodeType() != cdr::dom::Node::CDATA_SECTION_NODE)
          break;

        return xml.getNodeValue();
      }
    }
    throw cdr::Exception(L"dryrot: invalid return from getDocString\n");
  }

  /***************************************************************************/
  /* get filter using title.  Replace @@DOCTYPE@@ with document type of      */
  /* document to which it will be applied.  Error if more than one document  */
  /* with requested title                                                    */
  /***************************************************************************/
  cdr::String getFilterByName(cdr::String name,
                              cdr::db::Connection& connection,
                              cdr::String* docui,
                              int version = 0,
                              const cdr::String& maxDate=cdr::DFT_VERSION_DATE)
  {
    cdr::String::size_type idx = name.find(L"@@DOCTYPE@@");
    if (idx != cdr::String::npos)
    {
      cdr::String doctype;

      if (docui != NULL)
      {
        string tquery = "SELECT name "
                        "FROM doc_type dt "
                        "JOIN document d ON dt.id = d.doc_type "
                        "WHERE d.id=?";
        cdr::db::PreparedStatement tselect
              = connection.prepareStatement(tquery);
        tselect.setInt(1, docui->extractDocId());
        cdr::db::ResultSet trs = tselect.executeQuery();
        if (trs.next())
          doctype = trs.getString(1);
      }
      name.replace(idx, 11, doctype);
    }

    int id = getDocIdFromTitle(name, connection);

    return getDocument(cdr::stringDocId(id), version, connection,
                       CdrFilterType, maxDate);
  }


  /***********************************************************************
   * Get document from CdrFilter XML transaction.
   *
   * May be called to get either a document to be filter, or a filter itself.
   * Caller passes either a "Document" or a "Filter" node respectively.
   *
   *  @param docspec    The node to examine.
   *  @param connection Database access.
   *  @param type       Address of CdrFilterType or CdrDocType, or 0=any.
   *  @param ui         If non-zero, return CDR ID here.
   *  @param docui      If non-zero, ptr to doc id of doc to be filtered.
   *                    Used to find the doctype of the document to be
   *                    filtered, to replace an @@DOCTYPE@@ macro.
   *                    [Don't know why this is a pointer - AHM]
   *  @param maxDateBuf If non-zero, return the max date here.
   ***********************************************************************/
  cdr::String getDocument(cdr::dom::Node docspec,
                          cdr::db::Connection& connection,
                          const wchar_t* type = NULL,
                          cdr::String* ui = NULL,
                          cdr::String* docui = NULL,
                          cdr::String* maxDateBuf = NULL)
  {
// DEBUG
//std::cout << "In getDocument2\n";
    cdr::String docIdStr  = L"";
    cdr::String verString = L"";
    int version           = 0;
    cdr::String maxDate   = cdr::DFT_VERSION_DATE;

    // Get attributes of the Document or Filter node
    cdr::dom::NamedNodeMap attributes = docspec.getAttributes();

// DEBUG
/*
int alen = attributes.getLength();
std::cout<<"Attribute count="<<alen<<std::endl;
for (int i=0; i<alen; i++) {
 cdr::dom::Node attr=attributes.item(i);
 std::cout<<"Name="<<attr.getNodeName().toUtf8()<<" Value="<<attr.getNodeValue().toUtf8()<<std::endl;
}
*/
    // If there's an href, it's a request by doc (or filter) ui
    cdr::dom::Node href = attributes.getNamedItem("href");
    if (href != NULL)
    {
      docIdStr = href.getNodeValue();
      if (ui != NULL)
        *ui = docIdStr;
//std::cout<<"docIdStr="<<docIdStr.toUtf8()<<std::endl;
    }

    // Is there a limiting date for version retrieval?
    cdr::dom::Node dateNode = attributes.getNamedItem("maxDate");
    if (dateNode != NULL)
      maxDate = dateNode.getNodeValue();

    // Tell caller what maximum date we found or defaulted to
    if (maxDateBuf)
      *maxDateBuf = maxDate;

    // Is there a version number, or last/lastp?
    cdr::dom::Node verAttr = attributes.getNamedItem("version");
    if (verAttr != NULL)
    {
      verString = verAttr.getNodeValue();
      if (verString.size()) {
        // Version only makes sense if a docId given
        if (docIdStr == L"")
          throw cdr::Exception(L"CdrFilter:getDocument2: "
                  L"Version specified with no href=document ID");
        // Resolve last version
        if (verString == L"last")
          version = cdr::getVersionNumber(docIdStr.extractDocId(), connection,
                                          0, maxDate);
        // Resolve last publishable version
        else if (verString == L"lastp")
          version = cdr::getLatestPublishableVersion(docIdStr.extractDocId(),
                                  connection, 0, maxDate);

        // Else it should be an actual version number
        else
          version = verString.getInt();

//std::cout<<"In getDocument2, version num="<<version<<std::endl;
      }
    }

    // If there's a name, it's a request for a filter
    cdr::dom::Node name = attributes.getNamedItem("Name");
    if (name != NULL)
      return getFilterByName(name.getNodeValue(), connection, docui);

    // Else if there's a document id
    if (docIdStr != L"")
      return getDocument(docIdStr, version, connection, type, maxDate);

    // Else the xml was embedded in the transaction itself
    // There better be a CDATA section containing the data
    // Get it out of the transaction and return it to the caller
    cdr::dom::Node f = docspec.getFirstChild();
    if (f == NULL || f.getNodeType() != cdr::dom::Node::CDATA_SECTION_NODE)
      throw cdr::Exception(L"CdrFilter:getDocument2: "
                           L"Invalid filter specification");

    return f.getNodeValue();
  }

  /***************************************************************************/
  /* per thread data used in handlers                                        */
  /***************************************************************************/
  class UriInfo
  {
    public:
      UriInfo() : inuse(false), pos(0) {}

      bool inuse;
      string doc;
      int pos;
  };

  struct ThreadData
  {
      ThreadData(cdr::db::Connection& c, cdr::String id,
                 const cdr::String& maxDocDt=cdr::DFT_VERSION_DATE,
                 const cdr::String& maxFilterDt=cdr::DFT_VERSION_DATE)
        : connection(c), DocId(id), maxDocDate(maxDocDt),
          maxFilterDate(maxFilterDt), fatalError(false)
        {
            // If a max filter date is not specified, the default
            //   is to use the max document date, which may or may
            //   not be the default version date
            if (maxFilterDate == cdr::DFT_VERSION_DATE)
                maxFilterDate = maxDocDate;
        }

      string filter_messages;
      vector<UriInfo> uri_list;
      cdr::db::Connection& connection;
      cdr::String DocId;
      cdr::String maxDocDate;
      cdr::String maxFilterDate;

      // These two are added to avoid the exception Mike was throwing in
      // the message handler for fatal errors.  The exception was causing
      // a memory leak, because the invocation of the callback is not
      // wrapped in a try block inside Sablotron.
      bool fatalError;
      wostringstream errMsg;
  };

  /***************************************************************************/
  /* Sablotron error/message handling callbacks                              */
  /***************************************************************************/
  MH_ERROR makeCode(void*, SablotHandle, int,
                    unsigned short, unsigned short code)
  {
    return code;
  }

  MH_ERROR messageHandler(void* thread_data, SablotHandle, MH_ERROR code,
                          MH_LEVEL level, char** fields)
  {
    switch (level)
    {
      case MH_LEVEL_CRITICAL:
      case MH_LEVEL_ERROR:
      {
        ThreadData* td = static_cast<ThreadData*>(thread_data);
        td->fatalError = true;
        td->errMsg << L"XSLT error: code: " << code << L"\n";
        if (fields != NULL)
          while (*fields != NULL)
            td->errMsg << cdr::String(*(fields++)) << L"\n";
        break;
      }

      case MH_LEVEL_WARN:
        if (fields != NULL)
        {
          char** p;
          string msg;
          for (p = fields; p != NULL && strncmp(*p, "msg:", 4) != 0; ++p)
            ;
          if (p != NULL)
          {
            static const char xsl_msgheader[] = "xsl:message (";
            char* q = *p + 4;
            if (strncmp(q, xsl_msgheader, sizeof xsl_msgheader - 1) == 0)
            {
              msg = string(q + sizeof xsl_msgheader - 1);
              if (msg.length() != 0)
                msg = msg.substr(0, msg.length() - 1);

              static_cast<ThreadData*>(thread_data)->filter_messages
                += "<message>" + msg + "</message>\n";
            }
          }
        }
        break;

      case MH_LEVEL_INFO:
        break;
    }

    return 0;
  }

  /***************************************************************************
   * Sablotron error/message handling callbacks
   *
   * The "scheme" is a URI protocol identifier.  In theory, it could be
   * something like "http://".  For our purposes, the following schemes are
   * supported:
   *    cdr:/       Used for retrieving documents:
   *                cdr:/1234 - gets doc with id 1234.
   *                cdr:/1234/CdrCtl - gets it with CdrCtl wrapper.
   *                cdr:/\* - gets doc by id in ThreadData context.
   *    cdrx:/      Don't think this means anything different anymore?
   *    cdrutil:/   Get various things depending on what follows.
   *                Many different functions are supported.
   *
   * See Sablotron docs for callback interfaces.
   ***************************************************************************/
  int uri_open(void* data, SablotHandle, const char* scheme,
               const char* rest, int* handle)
  {
    if (scheme == NULL)
      return SH_ERR_UNSUPPORTED_SCHEME;

    ThreadData* thread_data = static_cast<ThreadData*>(data);
    UriInfo u;
//std::cout<<"ThreadData maxDocDate="<<thread_data->maxDocDate.toUtf8()<<" maxFilterDate="<<thread_data->maxFilterDate.toUtf8()<<std::endl;

    bool conditional = (strcmp(scheme, "cdrx") == 0);
    if (conditional || strcmp(scheme, "cdr") == 0)
    {
      cdr::db::Connection& connection
        = thread_data->connection;

      while (*rest == '/')
        ++rest;
      if (*rest == '\0')
        return 1;

      // we don't let an exception go back through Sablotron since we're not
      // sure if it's exception safe.  Instead we just return error if we can't
      // get the document
      try
      {
        // ***TODO***
        // for now we only implement simple UI to get the document XML and
        // ui/CdrCtl to get the entire doc control string
        cdr::String spec(rest);
        cdr::String uid;
        cdr::String version_str;
        cdr::String type;
        cdr::String::size_type sep = spec.find('/');
        cdr::String maxDate = thread_data->maxDocDate;
        if (sep != string::npos)
        {
          uid = spec.substr(0, sep);
          if (uid == L"*")
            uid = thread_data->DocId;
          spec = spec.substr(sep + 1);
        }
        else
        {
          uid = spec;
          spec = L"";
        }

        // 21May2002 RMK: Added capability to resolve URI with DocTitle.
        if (uid.find(L"name:") == 0)
        {
          // If we're here, we're getting a filter
          // XXX - FOR FILTER DATE LIMITING - XXX
          // Limit by filter date using the following technique:
          if (thread_data->maxFilterDate != cdr::DFT_VERSION_DATE) {
            version_str = "last"; // or lastp?
            maxDate = thread_data->maxFilterDate;
          }
          cdr::String::size_type pos = uid.find(L"@@SLASH@@");
          while (pos != uid.npos)
          {
            uid.replace(pos, 9, L"/");
            pos = uid.find(L"@@SLASH@@");
          }
          int id = getDocIdFromTitle(uid.substr(5), connection);
          uid = cdr::stringDocId(id);
        }

        if (!spec.empty())
        {
          sep = spec.find('/');
          if (sep != string::npos)
          {
            version_str = spec.substr(0, sep);
            spec = spec.substr(sep + 1);
          }
          else
          {
            version_str = spec;
            spec = L"";
          }
          if (!version_str.empty()
              && !isdigit(static_cast<unsigned char>(version_str[0]))
              && version_str != L"last" && version_str != L"lastp")
          {
            spec = version_str;
            version_str = L"";
          }
        }
        type = spec;

        int version = 0;
        if (version_str == L"last")
        {
//std::cout<<"uri_open last id="<<uid.extractDocId()<<" date="<<thread_data->maxDocDate.toUtf8()<<"\n";
          version = cdr::getVersionNumber(uid.extractDocId(), connection,
                                          0, maxDate);
          if (version < 1)
            throw cdr::Exception(L"No version found for document", uid);
        }
        else if (version_str == L"lastp")
        {
//std::cout<<"uri_open lastP id="<<uid.extractDocId()<<" date="<<thread_data->maxDocDate.toUtf8()<<"\n";
          version = cdr::getLatestPublishableVersion(uid.extractDocId(),
                                                     connection, 0, maxDate);
          if (version < 1)
            throw cdr::Exception(L"No publishable version found for document",
                                 uid);
        }
        else if (!version_str.empty())
          version = version_str.getInt();

        if (type == L"CdrCtl") {
          //std::wcout << L"getDocCtlString for " << uid << L"\n";
          u.doc = cdr::getDocCtlString(uid, version, connection,
                               cdr::DocCtlComponents::all).toUtf8();
        }
        else if (type == L"DocTitle") {
          //std::wcout << L"getDocTitle for " << uid << L"\n";
          u.doc = "<CdrDocTitle>"
                + getDocTitle(uid, connection).toUtf8()
                + "</CdrDocTitle>";
        }
        else
{ //std::cout<<"Fetching document="<<uid.toUtf8()<<" verNum="<<version<<std::endl;
          u.doc = getDocument(uid, version, connection).toUtf8();
}
      }
      catch (...)
      {
        if (!conditional)
          return 1;

        u.doc = "<CdrDocCtl><NotFound>Y</NotFound></CdrDocCtl>";
      }
    }
    else
    if (strcmp(scheme, "cdrutil") == 0)
    {
      while (*rest == '/')
        ++rest;

      string function(rest);
      string parms;
      string::size_type idx = function.find('/');
      if (idx != string::npos)
      {
        parms = function.substr(idx + 1);
        function = function.substr(0, idx);

      }

      if (function == "docid")
        u.doc = "<DocId>" + thread_data->DocId.toUtf8() + "</DocId>";
      else
      if (function == "date")
      {
        char buf[1024];

        const char* format = "%Y-%m-%dT%H:%M:%S.000";
        if (!parms.empty())
        {
          parms = uri_decode(parms).c_str();
          format = parms.c_str();
        }

        time_t tod = time(NULL);
        strftime(buf, sizeof buf, format, localtime(&tod));
        u.doc = string("<Date>") + buf + "</Date>";
      }
      // RMK 2002-09-02: added function to get a pretty URL from Cancer.gov.
      else if (function == "pretty-url")
      {
        u.doc = string("<PrettyUrl>") + getPrettyUrl(parms).toUtf8() +
                      "</PrettyUrl>";
      }
      // RMK 2003-05-29: added function to get number of last pub. version.
      else if (function == "get-pv-num")
      {
        u.doc = getPubVerNumber(parms, thread_data->connection);
      }
      // AHM 2004-05-04: added function to denormalize a Term reference
      else if (function == "denormalizeTerm")
      {
         try {
            u.doc = cdr::cache::Term::denormalizeTermId(parms,
                                              thread_data->connection);
         }
         catch (...) {
             // Should already be logged
             return SH_ERR_NOT_OK;
         }
      }
      else if (function == "valid-zip")
      {
        u.doc = getValidZip(parms, thread_data->connection);
      }
      else if (function == "extern-map")
      {
        u.doc = lookupExternalValue(parms, thread_data->connection);
      }
      else if (function == "verification-date")
      {
        u.doc = getVerificationDate(parms, thread_data->connection,
                                    thread_data->DocId);
      }
      else if (function == "sql-query")
      {
        u.doc = execXsltSqlQuery(parms, thread_data->connection);
      }
      else if (function == "dedup-ids")
      {
std::cout << "Recognized function 'dedup-ids'\n";
        u.doc = execXsltDedupIDs(parms);
      }
      else
        return 1;
    }
    else
      return SH_ERR_UNSUPPORTED_SCHEME;

    u.inuse = true;
    int i;
    int n = thread_data->uri_list.size();
    for (i = 0; i < n; ++i)
      if (thread_data->uri_list[i].inuse)
      {
        thread_data->uri_list[i] = u;
        *handle = i;
        return 0;
      }

    *handle = thread_data->uri_list.size();
    thread_data->uri_list.push_back(u);
    return 0;
  }

  int uri_close(void* data, SablotHandle, int handle)
  {
    static_cast<ThreadData*>(data)->uri_list[handle].inuse = false;
    return 0;
  }

  int uri_getall(void* data, SablotHandle processor, const char* scheme,
                 const char* rest, char** buffer, int* count)
  {
    int handle = -1;
    *buffer = NULL;
    ThreadData* thread_data = static_cast<ThreadData*>(data);

    try
    {
      int rc = uri_open(data, processor, scheme, rest, &handle);
      if (rc)
        return 1;

      *count = thread_data->uri_list[handle].doc.size();
      *buffer = new char[*count + 1];
      strcpy(*buffer, thread_data->uri_list[handle].doc.c_str());
      uri_close(data, processor, handle);
    }
    catch (...)
    {
      if (handle >= 0)
        uri_close(data, processor, handle);
      delete[] *buffer;
      throw;
    }

    return 0;
  }

  int uri_free(void*, SablotHandle, char* buffer)
  {
    delete[] buffer;
    return 0;
  }

  int uri_get(void* data, SablotHandle, int handle, char* buffer, int* count)
  {
    ThreadData* thread_data = static_cast<ThreadData*>(data);
    int n = thread_data->uri_list[handle].doc.size()
          - thread_data->uri_list[handle].pos;
    if (*count > n)
      *count = n;

    if (*count < 0)
      *count = 0;

    if (*count > 0)
      memcpy(buffer,
             thread_data->uri_list[handle].doc.data()
                 + thread_data->uri_list[handle].pos,
             *count);

    thread_data->uri_list[handle].pos += *count;

    return 0;
  }

  int uri_put(void*, SablotHandle, int, const char*, int*)
  {
    return 1;  // unsupported
  }

  /***************************************************************************/
  /* Process filter on document                                              */
  /***************************************************************************/
  int processStrings(string script, string document,
                     string& result, cdr::db::Connection& connection,
                     cdr::FilterParmVector* parms,
                     ThreadData* thread_data)
  {
      int         rc = 0;
      const char* s  = script.c_str();
      const char* d  = document.c_str();
      char*       r  = NULL;
      DWORD       startTime;

      try {
          // Prepare the control structures for Sablotron processing.
          SablotSituation situation = NULL;
          SablotHandle    processor = NULL;

          // If we're profiling filter performance, get
          //   the current millisecond time before filtering
          if (S_pFltrIdMap)
              startTime = GetTickCount();

          try {
              try {
                  SablotCreateSituation(&situation);
                  SablotCreateProcessorForSituation(situation, &processor);
                  SablotAddArgBuffer(situation, processor, "sheet", s);
                  SablotAddArgBuffer(situation, processor, "data", d);
              }
              catch (...) {
                  cdr::log::pThreadLog->Write(L"CdrFilter::processStrings",
                  L"Exception thrown in Sablotron initialization");
                  throw;
              }

              // Plug in (and remember for cleanup) the parameter strings.
              ParmList parmList;
              if (parms != NULL) {
                  for (cdr::FilterParmVector::iterator ip = parms->begin();
                       ip != parms->end();
                       ++ip) {
                      char* name = parmstr(ip->first);
                      char* value = parmstr(ip->second);
                      parmList.addparm(name, value);
                      SablotAddParam(situation, processor, name, value);
                  }
              }

              // Register our custom handlers.
              MessageHandler mh = { makeCode, messageHandler, messageHandler };
              if (SablotRegHandler(processor, HLR_MESSAGE, &mh, thread_data))
                  throw cdr::Exception(L"cannot register Sablotron "
                                       L"message handler");

              SchemeHandler sh = { uri_getall, uri_free, uri_open,
                                   uri_get, uri_put,  uri_close
              };
              if (SablotRegHandler(processor, HLR_SCHEME, &sh, thread_data))
                  throw cdr::Exception(L"cannot register Sablotron "
                                       L"scheme handler");

              // Perform the XSL/T transformation.
              try {
                  rc = SablotRunProcessorGen(situation, processor, "arg:/sheet",
                                         "arg:/data", "arg:/out");
              }
              catch (...) {
                  cdr::log::pThreadLog->Write(L"CdrFilter::processStrings",
                          L"Exception thrown in SablotRunProcessorGen");
                  throw;
              }

              if (thread_data->fatalError)
                  throw cdr::Exception(thread_data->errMsg.str());
              if (rc)
                  throw cdr::Exception(L"XSLT error");
              if ((rc = SablotGetResultArg(processor, "arg:/out", &r)) != 0)
                  throw cdr::Exception(L"Cannot get XSLT result");
          }
          catch (...) {
              cdr::log::pThreadLog->Write(L"CdrFilter::processStrings",
                      L"Exception thrown, Destroying proc and sit");
              SablotDestroyProcessor(processor);
              SablotDestroySituation(situation);
              cdr::log::pThreadLog->Write(L"CdrFilter::processStrings",
                      L"Destroying proc and sit successful");
              throw;
          }
          // Clean up and save the output.
          try {
              if ((rc = SablotDestroyProcessor(processor)) != 0)
                  throw cdr::Exception(L"Cannot destroy XSLT processor");
              if ((rc = SablotDestroySituation(situation)) != 0)
                  throw cdr::Exception(L"Cannot destroy Sablotron situation");
          }
          catch (...) {
              cdr::log::pThreadLog->Write(L"CdrFilter::processStrings",
                      L"Exception thrown in normal proc/sit cleanup");
              throw;
          }

          result = r;
          try {
              SablotFree(r);
          }
          catch (...) {
              cdr::log::pThreadLog->Write(L"CdrFilter::processStrings",
                      L"Exception thrown in normal SablotFree cleanup");
              throw;
          }


          // If profiling filter performance
          if (S_pFltrIdMap) {
              // Get time taken to filter
              DWORD endTime = GetTickCount();

              // Which filter are we using
              int fltrId = (*S_pFltrIdMap)[s];

              // Adjust for infrequent (every 47 days) timer wraparound
              DWORD amount;
              if (endTime < startTime)
                  // Adjust for clock tick wraparound every 47 days.
                  amount = ((DWORD) 0xFFFFFFFFFFFFFFFF - startTime) + endTime;
              else
                  amount = (endTime - startTime);

              // Store it in the database
              try {
                  cdr::db::Connection dbConn =
                      cdr::db::DriverManager::getConnection (cdr::db::url,
                                                             cdr::db::uid,
                                                             cdr::db::pwd);
                  std::string insQry =
                      "INSERT INTO filter_profile (id, millis, dt) "
                      "     VALUES (?, ?, GETDATE())";
                  cdr::db::PreparedStatement pstmt =
                      dbConn.prepareStatement(insQry);
                  pstmt.setInt(1, fltrId);
                  pstmt.setInt(2, (int) amount);
                  cdr::db::ResultSet rs = pstmt.executeQuery();
              }
              catch (cdr::Exception &e) {
                  cdr::log::WriteFile (L"CdrFilter profiling: db failure: ",
                                       e.what());
              }
          }

          return rc;
      }
      catch (...) {
          cdr::log::pThreadLog->Write(L"CdrFilter::processStrings",
                  L"An exception was thrown here, or in Sablotron");
          SablotFree(r);
          throw;
      }
  }
}

// Version that accepts filter document id
cdr::String cdr::filterDocumentByScriptId (
        const cdr::String& document,
        int filterId,
        cdr::db::Connection& connection,
        cdr::String* messages,
        cdr::FilterParmVector* parms,
        cdr::String doc_id)

{
  // Convert filterId to filter itself
  // This is NOT maxDate limit capable!
  // Currently used only by CdrDoc routines
  std::string qry = "SELECT xml FROM document WHERE id = ?";
  cdr::db::PreparedStatement stmt = connection.prepareStatement (qry);
  stmt.setInt (1, filterId);
  cdr::db::ResultSet rs = stmt.executeQuery();
  if (!rs.next())
      throw cdr::Exception (
              L"filterDocumentByScriptId: can't find filter for id="
              + cdr::String::toString (filterId));
  cdr::String filterXml = rs.getString (1);

  // Invoke existing function, passing the script
  return filterDocument (document, filterXml, connection, messages,
                         parms, doc_id);
}

int getDocIdFromTitle(const cdr::String& title,
                      cdr::db::Connection& connection)
{
  const char* query = " SELECT id        "
                      "   FROM document  "
                      "  WHERE title = ? ";

  cdr::db::PreparedStatement select = connection.prepareStatement(query);
  select.setString(1, title);
  cdr::db::ResultSet rs = select.executeQuery();
  if (!rs.next())
    throw cdr::Exception(L"No document with title", title);
  int id = rs.getInt(1);
  if (rs.next())
    throw cdr::Exception(L"Multiple documents with title", title);
  return id;
}

cdr::String getDocTitle(const cdr::String& id, cdr::db::Connection& conn)
{
  int docId = id.extractDocId();
  const char* const query = " SELECT title    "
                            "   FROM document "
                            "  WHERE id = ?   ";
  cdr::db::PreparedStatement ps = conn.prepareStatement(query);
  ps.setInt(1, id.extractDocId());
  cdr::db::ResultSet rs = ps.executeQuery();
  if (!rs.next())
    throw cdr::Exception(L"Document not found", id);
  cdr::String title = rs.getString(1);
  return cdr::entConvert(title);
}

// Version that accepts filter document title
cdr::String cdr::filterDocumentByScriptTitle (
        const cdr::String& document,
        const cdr::String& filterTitle,
        cdr::db::Connection& connection,
        cdr::String* messages,
        cdr::FilterParmVector* parms,
        cdr::String doc_id)
{
  // Convert filterId to filter itself
  std::string qry = "SELECT xml FROM document WHERE title = ?";
  cdr::db::PreparedStatement stmt = connection.prepareStatement (qry);
  stmt.setString (1, filterTitle);
  cdr::db::ResultSet rs = stmt.executeQuery();
  if (!rs.next())
      throw cdr::Exception (L"filterDocument: can't find filter for id="
                            + filterTitle);
  cdr::String filterXml = rs.getString (1);

  // Title had better be unique
  if (rs.next())
      throw cdr::Exception (L"filterDocument: filter title not unique: "
                            + filterTitle);

  // Invoke existing function, passing the script
  return filterDocument (document, filterXml, connection, messages,
                         parms, doc_id);
}

// Version that accepts name of filter set
cdr::String cdr::filterDocumentByScriptSetName (
    const cdr::String&     document,
    const cdr::String&     setName,
    cdr::db::Connection&   connection,
    cdr::String*           messages,
    const cdr::String&     version,
    cdr::FilterParmVector* parms,
    cdr::String            doc_id
) {
    // Resolve setName into vector of filter XML strings
    vector<cdr::String> filters;
    getFilterSetXml (connection, setName, version, filters);

    // Messages need to be std::string
    std::string msgs;

    // Execute the filters
    std::string result = filterVector (document, connection, filters,
                                       &msgs, parms, doc_id);

    // Convert messages
    *messages = msgs;

    // Convert and return filtered doc
    return cdr::String (result);
}

// Primary version, using string
cdr::String cdr::filterDocument(const cdr::String& document,
                           const cdr::String& filter,
                           cdr::db::Connection& connection,
                           cdr::String* messages,
                           cdr::FilterParmVector* parms,
                           cdr::String doc_id)
{
  string result;
  ThreadData thread_data(connection, doc_id);
  string fString = filter.toUtf8();
  string dString = document.toUtf8();
  if (processStrings(fString, dString, result,
                     connection, parms, &thread_data))
      throw cdr::Exception(L"error in XSLT processing");

  if (messages != NULL)
    *messages = thread_data.filter_messages;

  return result;
}

// Entry point for CdrServer clients.
cdr::String cdr::filter(cdr::Session& session,
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& connection)
{
  vector<cdr::String> filters;
  cdr::String document;
  cdr::String ui;
  cdr::String maxDocDate;
  cdr::String maxFilterDate;

  // check if we're to do output
  bool output = true;
  cdr::dom::NamedNodeMap cmdattr = commandNode.getAttributes();
  cdr::dom::Node attr = cmdattr.getNamedItem("Output");

  if (attr != NULL && cdr::String(attr.getNodeValue()) == L"N")
    output = false;

  // we need to get the document first so we'll have its type if needed
  // to determine the filter name

  // extract document from the command
  {
    for (cdr::dom::Node child = commandNode.getFirstChild();
         child != NULL;
         child = child.getNextSibling())
    {
      if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE)
      {
        cdr::String name = child.getNodeName();
        if (name == L"Document")
        {
          const cdr::dom::Element& e =
              static_cast<const cdr::dom::Element&>(child);

          const wchar_t* type = NULL;
          cdr::dom::NamedNodeMap attributes = child.getAttributes();
          cdr::dom::Node ctl = attributes.getNamedItem("ctl");
          if (ctl != NULL)
          {
            cdr::String val = ctl.getNodeValue();
            if (val == L"y")
              type = CdrDocType;
          }

          // Document may be in the command itself, or referred to
          //   in the command href.
          // If in the href, it may or may not have a numeric or
          //   symbolic version number and possibly a maxDate limit.
          // getDocument() parses the "Document" node to find out and
          //   get what we need.
          document = getDocument(child, connection, type, &ui, 0,
                                 &maxDocDate);
          break; // only one document allowed
        }
      }
    }
  }

  cdr::FilterParmVector pvector;

  // extract filters and parms from the command
  {
    for (cdr::dom::Node child = commandNode.getFirstChild();
         child != NULL;
         child = child.getNextSibling())
    {
      if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE)
      {
        cdr::String name = child.getNodeName();
        if (name == L"Filter")
          filters.push_back(getDocument(child, connection, CdrFilterType,
                                        NULL, &ui, &maxFilterDate));
        else
        if (name == L"FilterSet")
        {
          // Extract name and version of filter set
          const cdr::dom::Element& e =
              static_cast<const cdr::dom::Element&>(child);
          String setName    = e.getAttribute("Name");
          String versionStr = e.getAttribute("Version");

          // Limiting date on filters
          maxFilterDate = e.getAttribute("maxDate");
//std::cout<<"In cdr::filter, maxFilterDate="<<maxFilterDate.toUtf8()<<std::endl;
          if (maxFilterDate == L"")
              // If filter date not specified, use document date
              // If that's not explicitly specified, both are defaulted
              maxFilterDate = maxDocDate;

          // Get XML strings for each filter into the filterSet vector
          getFilterSetXml (connection, setName, versionStr, filters,
                           maxFilterDate);
        }
        else
        if (name == L"Parms")
        {
          // Assemble parameters into a vector of name/value pairs
          for (cdr::dom::Node pchild = child.getFirstChild();
               pchild != NULL;
               pchild = pchild.getNextSibling())
          {
            cdr::String name = pchild.getNodeName();
            if (name == L"Parm")
            {
              cdr::String name;
              cdr::String value;
              for (cdr::dom::Node ppchild = pchild.getFirstChild();
                   ppchild != NULL;
                   ppchild = ppchild.getNextSibling())
              {
                cdr::String nname = ppchild.getNodeName();
                cdr::String nvalue;
                cdr::dom::Node  ppc = ppchild.getFirstChild();
                if (ppc != NULL
                      && ppc.getNodeType() == cdr::dom::Node::TEXT_NODE)
                  nvalue = ppc.getNodeValue();

                if (nname == L"Name")
                  name = nvalue;
                else
                if (nname == L"Value")
                  value = nvalue;
              }
              if (name.length() != 0)
                pvector.push_back(pair<cdr::String, cdr::String>(name, value));
            }
          }
        }
      }
    }
  }

  // Create string to receive any messages
  std::string messages;

  // We now have the XML strings for document and all filters
  // Process doc through all filters
//std::cout<<"CdrFilter maxDocDate="<<maxDocDate.toUtf8()<<std::endl;
//std::cout<<"CdrFilter maxFilterDate="<<maxFilterDate.toUtf8()<<std::endl;
  std::string result = filterVector (document, connection, filters,
                                     &messages, &pvector, ui,
                                     maxDocDate, maxFilterDate);

  // Package results into XML transacton response
  result = output ? "<Document><![CDATA[" + result + "]]></Document>\n" : "";
  if (messages.length() != 0)
    result += "<Messages>" + messages + "</Messages>\n";

  return L"<CdrFilterResp>"
       + cdr::String(result)
       + L"</CdrFilterResp>\n";
}

/**
 * Retrieve the XML text for each filter in a set of filters.
 * Return them to the caller in a vector of strings
 *
 *  @param  connection  Reference to active db connection to CDR.
 *  @param  setName     Name of the filter set.
 *  @param  version     Version number or null, or symbolic "last" or "lastp".
 *  @param  filters     Pointer to vector to receive filters.
 *
 *  @return             Void.
 *  @throws             Exceptions from lower level db access.
 */
static void getFilterSetXml (
    cdr::db::Connection&  connection,
    cdr::String           setName,
    cdr::String           version,
    vector<cdr::String>&  filters,
    const cdr::String&    maxFilterDate
) {
//std::cout<<"In getFilterSetXml, setName="<<setName.toUtf8()<<" version="<<version.toUtf8()<<" maxFilterDate="<<maxFilterDate.toUtf8()<<std::endl;
    // Vector of doc ids for each filter in the set
    std::vector<int> filterSet;

    // Fill in filterSet with filter document IDs
    getFiltersInSet(setName, filterSet, connection);

    // For each one, find the document string
    for (size_t i = 0; i < filterSet.size(); ++i) {

        int id = filterSet[i];
        int ver = 0;
        cdr::String idStr = cdr::stringDocId(id);

        // XXX Possibly controversial decision?
        // If a maxFilterDate is specified, get the last version from
        //   the version table, not the CWD from the document table
        if (maxFilterDate != cdr::DFT_VERSION_DATE && version.empty())
{
//std::cout<<"Setting version='last'"<<std::endl;
            version = L"last";
}

        // Resolve version
        if (version == L"last")
          ver = cdr::getVersionNumber(id, connection, 0, maxFilterDate);
        else if (version == L"lastp")
          ver = cdr::getLatestPublishableVersion(id, connection, 0,
                                                 maxFilterDate);
        else if (!version.empty())
          throw cdr::Exception(L"Invalid value for Version attribute",
                               version);

//std::cout<<"About to fetch filter, id="<<idStr.toUtf8()<<" verNum="<<ver<<std::endl;
        // Fetch from the database
        cdr::String doc = getDocument (idStr, ver, connection, CdrFilterType);

        // Add it to caller's vector of filter document strings
        filters.push_back(doc);
    }
}

/**
 * Filter a document through a set of filters.
 *
 *  @param  document    cdr::String document to be filtered
 *  @param  filterSet   Vector of filters as XML strings.
 *  @param  connection  Reference to active database connection to CDR.
 *  @param  messages    cdr::String*.  If not null, nonerror messages
 *                      from the filter are placed (as XML) in this string
 *  @param  parms       Pointer to vector of pairs of param name + value
 *                       All params are passed to all filters.
 *  @param  doc_id      CDR identifier of document
 *
 *  @return             Filtered document, in utf-8.
 *
 *  @throws             Database or filter exceptions from lower down.
 */
static std::string filterVector (
    cdr::String           document,
    cdr::db::Connection&  connection,
    vector<cdr::String>&  filterSet,
    std::string           *messages,
    cdr::FilterParmVector *parms,
    cdr::String           doc_id,
    const cdr::String&    maxDocDate,
    const cdr::String&    maxFilterDate
) {
    // Convert input doc to utf8 for filtering
    string dString = document.toUtf8();
    std::string doc(dString);
    std::string result(doc);

    // Create struct for Sablotron callbacks
    ThreadData threadData (connection, doc_id, maxDocDate, maxFilterDate);

    // Apply each filter in the vector
    for (std::vector<cdr::String>::iterator i = filterSet.begin();
         i != filterSet.end(); ++i) {

        // Execute one filter
        string fString = i->toUtf8();
        if (processStrings (fString, doc, result, connection, parms,
                            &threadData))
            throw cdr::Exception (L"error in XSLT processing");

        // Output becomes input of next filter
        doc = result;
    }

    // Make messages available if requested
    if (messages != NULL)
        *messages = threadData.filter_messages;

    // Return last output as utf8 std::string
    return doc;
}


/**
 * Open a socket on the specified port to the specified host.
 *
 *  @param  host        string for the host; can be dotted-decimal or
 *                      DNS name.
 *  @param  port        integer for the port to which we are to connect.
 *  @returns            port number if successful; -1 on failure.
 */
int openSocket(const char* host, int port)
{
    long                address;
    struct sockaddr_in  addr;
    struct hostent *    ph;
    int                 sock;

    // Handle dotted-decimal IP address.
    if (isdigit(host[0])) {
        if ((address = inet_addr(host)) == -1)
            return -1;
        addr.sin_addr.s_addr = address;
        addr.sin_family = AF_INET;
    }
    else {
        // Lookup the host by its DNS name.
        ph = gethostbyname(host);
        if (!ph)
            return -1;
        addr.sin_family = ph->h_addrtype;
        memcpy(&addr.sin_addr, ph->h_addr, ph->h_length);
    }

    // Plug in the port.
    addr.sin_port = htons(port);

    // Create a TCP/IP socket.
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return -1;

    // Connect to the host.
    if (connect(sock, (struct sockaddr *)&addr, sizeof addr) < 0) {
        closesocket(sock);
        return -1;
    }
    return sock;
}

/*
 * Object representing the headers for an HTTP response.
 */
struct HttpHeaders {
    int code;
    typedef std::map<std::string, std::string> HeaderMap;
    HeaderMap headerMap;

    /**
     * Extract the code representing the status of the response,
     * and each of the header lines.
     */
    HttpHeaders(const std::string& lines) : code(-1) {

        // Extract the status code from the first line.
        size_t eol = lines.find("\r\n");
        if (eol == lines.npos)
            return;
        size_t i = 0;
        while (i < eol && !isspace(lines[i]))
            ++i;
        while (i < eol && isspace(lines[i]))
            ++i;
        code = atoi(lines.c_str() + i);
        size_t pos = eol + 2;

        // Loop through each of the remaining lines.
        eol = lines.find("\r\n", pos);
        while (eol != lines.npos) {

            // Find the name of the header.
            size_t nameStart = pos;
            while (nameStart < eol && isspace(lines[nameStart]))
                ++nameStart;
            size_t nameEnd = nameStart;
            while (nameEnd < eol && !strchr(" \t:", lines[nameEnd]))
                ++nameEnd;
            std::string name = lines.substr(nameStart, nameEnd - nameStart);

            // Lowercase the name (they're case-insensitive).
            for (size_t i = 0; i < name.size(); ++i)
                name[i] = tolower(name[i]);

            // Move to the beginning of the value.
            size_t valStart = nameEnd;
            while (valStart < eol && strchr(" \t:", lines[valStart]))
                ++valStart;
            std::string value = lines.substr(valStart, eol - valStart);

            // Store the header in the map and skip to the next line.
            headerMap[name] = value;
            pos = eol + 2;
            eol = lines.find("\r\n", pos);
        }
    }

    /**
     * Find the content-length header and return the integer form of its
     * value.
     *
     *  @returns        number of bytes in the body of the response,
     *                  or 0 if the content-length header can't be
     *                  found.
     */
    int getContentLength() const {
        std::string contentLength = "content-length";
        if (headerMap.find(contentLength) != headerMap.end()) {
            std::string headerValue = headerMap.find(contentLength)->second;
            return atoi(headerValue.c_str());
        }
        return 0;
    }
};

/**
 * Send a soap request to the specified host, and return the response XML.
 *
 *  @param  host            c-style string naming the host.
 *  @param  action          value for the HTTP SOAPAction header.
 *  @param  request         XML document containing the SOAP request.
 *  @param  path            portion of the URL following the host; e.g.:
 *                          "/PrettyURL/GetPrettyURL.asmx".
 *  @param  port            integer for the port on which the socket
 *                          should be opened; default is 80 (http).
 *  @return                 string for XML document containing the SOAP
 *                          server's response.
 */
cdr::String sendSoapRequest(
        const char* host,
        const cdr::String& action,
        const cdr::String& request,
        const cdr::String& path,
        int port = 80)
{
    // Build the request.
    std::string r = request.toUtf8();
    int len = r.size();
    int sock = openSocket(host, port);
    std::string headers =
        "POST " + path.toUtf8() + " HTTP/1.1\r\n"
        "Host: " + std::string(host) + "\r\n"
        "Accept-Encoding: identity\r\n"
        "Content-Length: " + cdr::String::toString(len).toUtf8() + "\r\n"
        "SOAPAction: " + action.toUtf8() + "\r\n"
        "Content-Type: text/xml; charset=utf-8\r\n\r\n";
    std::string message = headers + r;

    // Send the request to the server.
	//std::cout << "*** REQUEST ***\n" << message;
    if (send(sock, message.c_str(), message.size(), 0) < 0) {
        closesocket(sock);
        return L"";
    }

    // Collect the response.
    std::string response;
    char buf[8192];
    int n = recv(sock, buf, sizeof buf, 0);
    while (n > 0) {
        response += std::string(buf, n);

        // See if we have a complete set of headers.
        size_t hdrEnd = response.find("\r\n\r\n");
        while (hdrEnd != response.npos) {
            HttpHeaders headers = HttpHeaders(response.substr(0, hdrEnd + 2));
            response = response.substr(hdrEnd + 4);

            // Code 200 means we have the successful response.
            if (headers.code == 200) {
                int contentLength = headers.getContentLength();
                while (response.size() < (size_t)contentLength) {
                    n = recv(sock, buf, sizeof buf, 0);
                    if (n > 0)
                        response += std::string(buf, n);
                    if (n < 1) {
                        closesocket(sock);
                        return L"";
                    }
                }
                closesocket(sock);
                return response;
            }

            // Code 100 means continue.
            else if (headers.code == 100)
                hdrEnd = response.find("\r\n\r\n");

            // Everything else if a failure code.
            else {
                closesocket(sock);
                return L"";
            }
        }
		n = recv(sock, buf, sizeof buf, 0);
    }

    // Didn't get the response we're were looking for.
    closesocket(sock);
    return L"";
}

/**
 * Asks Cancer.gov for the URL which corresponds to a GUID which they
 * gave the user as a more stable identifier (but too ugly to use in
 * a URL).
 *
 *  @param      guid    reference to string we want to trade in for the url;
 *                      e.g., 2af98ad7-51eb-46a4-8c74-111b6215c496
 *  @returns            an http url if successful, or an empty string;
 *                      Lakshmi doesn't want the filter module to stop
 *                      if we fail, so unfortunately the user won't know
 *                      why a failure occurred if it did.
 */
cdr::String getPrettyUrl(const cdr::String& guid)
{
    try {
        const char* host    = "stage.cancer.gov";
        cdr::String path    = "/PrettyURL/GetPrettyURL.asmx";
        cdr::String action  = "http://cancer.gov/GetPrettyURL/ReturnPrettyURL";
        cdr::String request =
    L"<?xml version='1.0' encoding='utf-8'?>"
    L"<soap:Envelope xmlns:soap='http://schemas.xmlsoap.org/soap/envelope/'"
    L"               xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'"
    L"               xmlns:xsd='http://www.w3.org/2001/XMLSchema'>"
    L" <soap:Body>"
    L"  <ReturnPrettyURL xmlns='http://cancer.gov/GetPrettyURL/'>"
    L"   <viewID>" + guid + L"</viewID>"
    L"   <objectID/>"
    L"  </ReturnPrettyURL>"
    L" </soap:Body>"
    L"</soap:Envelope>";
        cdr::String response = sendSoapRequest(host, action, request, path);
        cdr::String openTag = L"&lt;url&gt;";
        cdr::String closeTag = L"&lt;/url&gt;";
        size_t pos = response.find(openTag);
        if (pos == response.npos) return L"";
        pos += openTag.size();
        size_t end = response.find(closeTag, pos);
        if (end == response.npos) return L"";
        return response.substr(pos, end - pos);

    }
    catch (...) {
        return L"";
    }
}

struct FilterSetMember {
    int foreignKey;
    bool nested;
    FilterSetMember(int fk, bool flag) : foreignKey(fk), nested(flag) {}
};
typedef std::vector<FilterSetMember> FilterSetMembers;

static void addFilterSetMembers(cdr::db::Connection& connection,
                                int id,
                                const FilterSetMembers& filterSetMembers)
{
    cdr::db::PreparedStatement ps = connection.prepareStatement(
            "INSERT INTO filter_set_member (filter_set, "
            "                               position,   "
            "                               filter,     "
            "                               subset)     "
            "     VALUES (?, ?, ?, ?)                   ");
    cdr::Int nullInt(true);
    for (int i = 0; i < (int)filterSetMembers.size(); ++i) {
        ps.setInt(1, id);
        ps.setInt(2, i + 1);
        if (filterSetMembers[i].nested) {
            ps.setInt(3, nullInt);
            ps.setInt(4, filterSetMembers[i].foreignKey);
        }
        else {
            ps.setInt(3, filterSetMembers[i].foreignKey);
            ps.setInt(4, nullInt);
        }
        ps.executeUpdate();
    }
}

static void extractFilterSetParams(const cdr::dom::Node& commandNode,
                                   cdr::String& filterSetName,
                                   cdr::String& filterSetDescription,
                                   cdr::String& filterSetNotes,
                                   FilterSetMembers& filterSetMembers)
{
    cdr::dom::Node child = commandNode.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"FilterSetName")
                filterSetName = cdr::dom::getTextContent(child);
            else if (name == L"FilterSetDescription")
                filterSetDescription = cdr::dom::getTextContent(child);
            else if (name == L"FilterSetNotes")
                filterSetNotes = cdr::dom::getTextContent(child);
            else if (name == L"Filter") {
                const cdr::dom::Element& e =
                    static_cast<const cdr::dom::Element&>(child);
                cdr::String idString = e.getAttribute("DocId");
                if (idString.empty())
                    throw cdr::Exception(
                            L"Missing DocId attribute for Filter");
                int id = idString.extractDocId();
                filterSetMembers.push_back(FilterSetMember(id, false));
            }
            else if (name == L"FilterSet") {
                const cdr::dom::Element& e =
                    static_cast<const cdr::dom::Element&>(child);
                cdr::String idString = e.getAttribute("SetId");
                if (idString.empty())
                    throw cdr::Exception(
                            L"Missing SetId attribute for FilterSet");
                int id = idString.getInt();
                filterSetMembers.push_back(FilterSetMember(id, true));
            }
        }
        child = child.getNextSibling();
    }
    if (filterSetName.empty())
        throw cdr::Exception(L"FilterSetName element is required.");
    if (filterSetDescription.empty())
        throw cdr::Exception(L"FilterSetDescription element is required.");
}

/**
 * Creates a new named filter set, establishing its attributes and
 * composition.
 *
 *  @param      session     contains information about the current user.
 *  @param      node        contains the XML for the command.
 *  @param      conn        reference to the connection object for the
 *                          CDR database.
 *  @return                 String object containing the XML for the
 *                          command response.
 *  @exception  cdr::Exception if a database or processing error occurs.
 */
cdr::String cdr::addFilterSet(cdr::Session& session,
                              const cdr::dom::Node& commandNode,
                              cdr::db::Connection& connection)
{
    // Check user authorization.
    if (!session.canDo(connection, L"ADD FILTER SET", L""))
        throw Exception (L"addFilterSet: User '" + session.getUserName() +
                         L"' not authorized to create new filter sets.");

    // Extract the parameters from the command.
    String filterSetName;
    String filterSetDescription;
    String filterSetNotes(true); // initiated as NULL.
    FilterSetMembers filterSetMembers;
    extractFilterSetParams(commandNode, filterSetName, filterSetDescription,
                           filterSetNotes, filterSetMembers);

    // Add the new set.
    connection.setAutoCommit(false);
    db::PreparedStatement stmt = connection.prepareStatement(
            "INSERT INTO filter_set (name, description, notes)"
            "     VALUES (?, ?, ?)");
    stmt.setString(1, filterSetName);
    stmt.setString(2, filterSetDescription);
    stmt.setString(3, filterSetNotes);
    stmt.executeUpdate();
    int id = connection.getLastIdent();

    // Populate the set.
    addFilterSetMembers(connection, id, filterSetMembers);

    // Do this mostly to detect infinitely recursive set.
    std::vector<int> filters;
    getFiltersInSet(id, filters, 0, connection);
    connection.commit();

    // Report success.
    return L"<CdrAddFilterSetResp TotalFilters='"
        + String::toString(filters.size())
        + L"'/>";
}

/**
 * Replaces the attributes and composition of a named filter set.
 *
 *  @param      session     contains information about the current user.
 *  @param      node        contains the XML for the command.
 *  @param      conn        reference to the connection object for the
 *                          CDR database.
 *  @return                 String object containing the XML for the
 *                          command response.
 *  @exception  cdr::Exception if a database or processing error occurs.
 */
cdr::String cdr::repFilterSet(cdr::Session& session,
                              const cdr::dom::Node& commandNode,
                              cdr::db::Connection& connection)
{
    // Check user authorization.
    if (!session.canDo(connection, L"MODIFY FILTER SET", L""))
        throw Exception (L"repFilterSet: User '" + session.getUserName() +
                         L"' not authorized to modify existing filter sets.");

    // Extract the parameters from the command.
    String filterSetName;
    String filterSetDescription;
    String filterSetNotes(true); // initiated as NULL.
    FilterSetMembers filterSetMembers;
    extractFilterSetParams(commandNode, filterSetName, filterSetDescription,
                           filterSetNotes, filterSetMembers);

    // Find the filter set.
    db::PreparedStatement query = connection.prepareStatement(
            "SELECT id FROM filter_set WHERE name = ?");
    query.setString(1, filterSetName);
    db::ResultSet rs = query.executeQuery();
    if (!rs.next())
        throw Exception(L"Filter set not found", filterSetName);
    int id = rs.getInt(1);
    if (!id)
        throw Exception(L"Filter set not found", filterSetName);

    // Replace the set's attributes.
    connection.setAutoCommit(false);
    db::PreparedStatement stmt = connection.prepareStatement(
            "UPDATE filter_set       "
            "   SET name = ?,        "
            "       description = ?, "
            "       notes = ?        "
            " WHERE id = ?           ");
    stmt.setString(1, filterSetName);
    stmt.setString(2, filterSetDescription);
    stmt.setString(3, filterSetNotes);
    stmt.setInt   (4, id);
    stmt.executeUpdate();

    // Clear out the set's membership.
    db::PreparedStatement del = connection.prepareStatement(
            "DELETE FROM filter_set_member WHERE filter_set = ?");
    del.setInt(1, id);
    del.executeUpdate();

    // Populate the set.
    addFilterSetMembers(connection, id, filterSetMembers);

    // Do this mostly to detect infinitely recursive set.
    std::vector<int> filters;
    getFiltersInSet(id, filters, 0, connection);
    connection.commit();

    // Report success.
    return L"<CdrRepFilterSetResp TotalFilters='"
        + String::toString(filters.size())
        + L"'/>";
}

/**
 * Returns the attributes and contents of a named filter set.
 *
 *  @param      session     contains information about the current user.
 *  @param      node        contains the XML for the command.
 *  @param      conn        reference to the connection object for the
 *                          CDR database.
 *  @return                 String object containing the XML for the
 *                          command response.
 *  @exception  cdr::Exception if a database or processing error occurs.
 */
cdr::String cdr::getFilterSet(Session& session,
                              const dom::Node& commandNode,
                              db::Connection& connection)
{
    String filterSetName;
    dom::Node child = commandNode.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"FilterSetName")
                filterSetName = cdr::dom::getTextContent(child);
        }
        child = child.getNextSibling();
    }
    if (filterSetName.empty())
        throw Exception(L"FilterSetName element required");

    // Find the filter set.
    db::PreparedStatement q1 = connection.prepareStatement(
            "SELECT id,          "
            "       name,        "
            "       description, "
            "       notes        "
            "  FROM filter_set   "
            " WHERE name = ?     ");
    q1.setString(1, filterSetName);
    db::ResultSet rs1 = q1.executeQuery();
    if (!rs1.next())
        throw Exception(L"Filter set not found", filterSetName);
    int    id    = rs1.getInt(1);
    String name  = rs1.getString(2);
    String desc  = rs1.getString(3);
    String notes = rs1.getString(4);

    // Start the response.
    std::wostringstream response;
    response << L"<CdrGetFilterSetResp><FilterSetName>"
             << name
             << L"</FilterSetName><FilterSetDescription>"
             << desc
             << L"</FilterSetDescription>";
    if (!notes.isNull())
        response << L"<FilterSetNotes>" << notes << L"</FilterSetNotes>";

    // Find the filter set members.
    db::PreparedStatement q2 = connection.prepareStatement(
            "  SELECT m.filter,             "
            "         d.title,              "
            "         m.position,           "
            "         'F'                   "
            "    FROM filter_set_member m   "
            "    JOIN document d            "
            "      ON d.id = m.filter       "
            "   WHERE m.filter_set = ?      "
            "     AND m.filter IS NOT NULL  "
            "   UNION                       "
            "  SELECT m.subset,             "
            "         s.name,               "
            "         m.position,           "
            "         'S'                   "
            "    FROM filter_set_member m   "
            "    JOIN filter_set s          "
            "      ON s.id = m.subset       "
            "   WHERE filter_set = ?        "
            "     AND m.subset IS NOT NULL  "
            "ORDER BY 3                     ");
    q2.setInt(1, id);
    q2.setInt(2, id);
    db::ResultSet rs2 = q2.executeQuery();
    while (rs2.next()) {
        int foreignKey = rs2.getInt(1);
        String name    = rs2.getString(2);
        int position   = rs2.getInt(3);
        String type    = rs2.getString(4);
        if (type == L"F") {
            response << L"<Filter DocId='"
                     << stringDocId(foreignKey)
                     << L"'>"
                     << name
                     << L"</Filter>";
        }
        else {
            response << L"<FilterSet SetId='"
                     << foreignKey
                     << L"'>"
                     << name
                     << L"</FilterSet>";
        }
    }
    response << L"</CdrGetFilterSetResp>";
    return response.str();
}

/**
 * Deletes a named CDR filter set.  Blocked if the set is used as a
 * nested member of another filter set.
 *
 *  @param      session     contains information about the current user.
 *  @param      node        contains the XML for the command.
 *  @param      conn        reference to the connection object for the
 *                          CDR database.
 *  @return                 String object containing the XML for the
 *                          command response.
 *  @exception  cdr::Exception if a database or processing error occurs.
 */
cdr::String cdr::delFilterSet(Session& session,
                              const dom::Node& commandNode,
                              db::Connection& connection)
{
    // Check user authorization.
    if (!session.canDo(connection, L"DELETE FILTER SET", L""))
        throw Exception (L"delFilterSet: User '" + session.getUserName() +
                         L"' not authorized to delete filter sets.");

    String filterSetName;
    dom::Node child = commandNode.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"FilterSetName")
                filterSetName = cdr::dom::getTextContent(child);
        }
        child = child.getNextSibling();
    }
    if (filterSetName.empty())
        throw Exception(L"Required FilterSetName element missing");

    // Find the filter set.
    db::PreparedStatement q1 = connection.prepareStatement(
            "SELECT id         "
            "  FROM filter_set "
            " WHERE name = ?   ");
    q1.setString(1, filterSetName);
    db::ResultSet rs1 = q1.executeQuery();
    if (!rs1.next())
        throw Exception(L"Filter set not found", filterSetName);
    int    id    = rs1.getInt(1);

    // Make sure the set isn't a nested member of another set.
    db::PreparedStatement q2 = connection.prepareStatement(
            "SELECT COUNT(*)          "
            "  FROM filter_set_member "
            " WHERE subset = ?        ");
    q2.setInt(1, id);
    db::ResultSet rs2 = q2.executeQuery();
    if (!rs2.next())
        throw Exception(L"Internal error checking for nested set membership");
    if (rs2.getInt(1) > 0)
        throw Exception(L"Set cannot be deleted",
                        L"Set is used as a member of other sets");

    // Open a transaction and start deleting rows.
    connection.setAutoCommit(false);
    db::PreparedStatement q3 = connection.prepareStatement(
            "DELETE FROM filter_set_member "
            "      WHERE filter_set = ?    ");
    q3.setInt(1, id);
    q3.executeUpdate();
    db::PreparedStatement q4 = connection.prepareStatement(
            "DELETE FROM filter_set "
            "      WHERE id = ?     ");
    q4.setInt(1, id);
    q4.executeUpdate();
    connection.commit();

    // Report success.
    return L"<CdrDelFilterSetResp/>";
}

/**
 * Returns a list of the named filter sets in the CDR.
 *
 *  @param      session     contains information about the current user.
 *  @param      node        contains the XML for the command.
 *  @param      conn        reference to the connection object for the
 *                          CDR database.
 *  @return                 String object containing the XML for the
 *                          command response.
 *  @exception  cdr::Exception if a database or processing error occurs.
 */
cdr::String cdr::getFilterSets(cdr::Session& session,
                               const cdr::dom::Node& commandNode,
                               cdr::db::Connection& connection)
{
    // Find the filter sets.
    std::wostringstream response;
    response << L"<CdrGetFilterSetsResp>";
    db::Statement stmt = connection.createStatement();
    db::ResultSet rs = stmt.executeQuery(
            "  SELECT id,        "
            "         name       "
            "    FROM filter_set "
            "ORDER BY name       ");
    while (rs.next()) {
        int    id   = rs.getInt(1);
        String name = rs.getString(2);
        response << L"<FilterSet SetId='"
                 << id
                 << L"'>"
                 << name
                 << L"</FilterSet>";
    }
    response << L"</CdrGetFilterSetsResp>";
    return response.str();
}

/**
 * Returns a list of the filter documents in the CDR repository.
 *
 *  @param      session     contains information about the current user.
 *  @param      node        contains the XML for the command.
 *  @param      conn        reference to the connection object for the
 *                          CDR database.
 *  @return                 String object containing the XML for the
 *                          command response.
 *  @exception  cdr::Exception if a database or processing error occurs.
 */
cdr::String cdr::getFilters(cdr::Session& session,
                            const cdr::dom::Node& commandNode,
                            cdr::db::Connection& connection)
{
    // Find the filter documents.
    std::wostringstream response;
    response << L"<CdrGetFiltersResp>";
    db::Statement stmt = connection.createStatement();
    db::ResultSet rs = stmt.executeQuery(
            "  SELECT d.id,             "
            "         d.title           "
            "    FROM document d        "
            "    JOIN doc_type t        "
            "      ON t.id = d.doc_type "
            "   WHERE t.name = 'Filter' "
            "ORDER BY d.title           ");
    while (rs.next()) {
        int    id    = rs.getInt(1);
        String title = rs.getString(2);
        response << L"<Filter DocId='"
                 << stringDocId(id)
                 << L"'>"
                 << title
                 << L"</Filter>";
    }
    response << L"</CdrGetFiltersResp>";
    return response.str();
}

void getFiltersInSet(const cdr::String& name,
                     std::vector<int>& filterSet,
                     cdr::db::Connection& conn)
{
    cdr::db::PreparedStatement stmt = conn.prepareStatement(
            "SELECT id         "
            "  FROM filter_set "
            " WHERE name = ?   ");
    stmt.setString(1, name);
    cdr::db::ResultSet rs = stmt.executeQuery();
    if (!rs.next())
        throw cdr::Exception(L"Unable to find filter set", name);
    int id = rs.getInt(1);
    getFiltersInSet(id, filterSet, 0, conn);
}

void getFiltersInSet(int id,
                     std::vector<int>& filterSet,
                     int depth,
                     cdr::db::Connection& conn)
{
    FilterSetMembers setMembers;
    if (depth > MAX_FILTER_SET_DEPTH)
        throw cdr::Exception(L"getFiltersInSet(): infinite recursion");
    cdr::db::PreparedStatement stmt = conn.prepareStatement(
            "  SELECT filter,           "
            "         subset,           "
            "         position          "
            "    FROM filter_set_member "
            "   WHERE filter_set = ?    "
            "ORDER BY position          ");
    stmt.setInt(1, id);
    cdr::db::ResultSet rs = stmt.executeQuery();
    while (rs.next()) {
        cdr::Int filterId = rs.getInt(1);
        cdr::Int subsetId = rs.getInt(2);
        int      position = rs.getInt(3);
        if (!filterId.isNull())
            setMembers.push_back(FilterSetMember(filterId, false));
        else if (!subsetId.isNull())
            setMembers.push_back(FilterSetMember(subsetId, true));
        else
            throw cdr::Exception(L"filter and subset foreign keys both "
                                 L"null at position " +
                                 cdr::String::toString(position) +
                                 L" in filter set " +
                                 cdr::String::toString(id));
    }
    for (size_t i = 0; i < setMembers.size(); ++i) {
        if (setMembers[i].nested)
            getFiltersInSet(setMembers[i].foreignKey, filterSet, depth + 1,
                            conn);
        else
            filterSet.push_back(setMembers[i].foreignKey);
    }
}

string getPubVerNumber(const string& parms,
                       cdr::db::Connection& conn)
{
    cdr::String idString = parms;
    int id = 0;
    try {
        id = idString.extractDocId();
    }
    catch (...) {
        return "<PubVerNumber>0</PubVerNumber>";
    }
    if (!id)
        return "<PubVerNumber>0</PubVerNumber>";
    cdr::db::PreparedStatement stmt = conn.prepareStatement(
            "  SELECT MAX(v.num)            "
            "    FROM doc_version v         "
            "    JOIN document d            "
            "      ON d.id = v.id           "
            "   WHERE v.id = ?              "
            "     AND d.active_status = 'A' "
            "     AND v.publishable = 'Y'   ");
    stmt.setInt(1, id);
    cdr::db::ResultSet rs = stmt.executeQuery();
    if (!rs.next())
        return "<PubVerNumber>0</PubVerNumber>";
    int versionNum = rs.getInt(1);
    std::ostringstream os;
    os << "<PubVerNumber>" << versionNum << "</PubVerNumber>";
    return os.str();
}

string getValidZip(const string& parms,
                   cdr::db::Connection& conn)
{
    cdr::String zipData = parms;
    if (zipData.size() < 5)
        return "<ValidZip/>";
    zipData = zipData.substr(0, 5);
    cdr::db::PreparedStatement stmt = conn.prepareStatement(
            "SELECT zip FROM zipcode WHERE zip = ?");
    stmt.setString(1, zipData);
    cdr::db::ResultSet rs = stmt.executeQuery();
    if (!rs.next())
        return "<ValidZip/>";
    std::ostringstream os;
    os << "<ValidZip>" << parms.substr(0, 5) << "</ValidZip>";
    return os.str();
}

string getVerificationDate(const string& parms, cdr::db::Connection& conn,
                           const cdr::String& docIdString) {
    //std::cerr << "getVerificationDate(parms=" << parms.c_str() << ")\n";
    int docId = docIdString.extractDocId();
    string latestDate;
    string verificationDate;
    string importDate;
    string mailerResponseDate;
    string lastModDate;
    string::size_type idx = parms.find('/');
    if (idx != string::npos) {
        string temp = parms.substr(idx + 1);
        if (!temp.empty())
            lastModDate = temp;
        verificationDate = parms.substr(0, idx);
    }
    else if (!parms.empty())
        verificationDate = parms;
    //std::cerr << "lastModDate=" << lastModDate.c_str() << "\n";
    //std::cerr << "verificationDate=" << verificationDate.c_str() << "\n";
    cdr::db::PreparedStatement stmt1 = conn.prepareStatement(
             " SELECT MAX(j.dt)      "
             "   FROM import_job j   "
             "   JOIN import_event e "
             "     ON e.job = j.id   "
             "   JOIN import_doc d   "
             "     ON d.id = e.doc   "
             "  WHERE d.cdr_id = ?   ");
    stmt1.setInt(1, docId);
    cdr::db::ResultSet rs1 = stmt1.executeQuery();
    if (rs1.next())
        importDate = rs1.getString(1).toUtf8();
    stmt1.close();
    //std::cerr << "importDate=" << importDate.c_str() << "\n";
    cdr::db::PreparedStatement stmt2 = conn.prepareStatement(
             " SELECT MAX(r.value)                         "
             "   FROM query_term r                         "
             "   JOIN query_term d                         "
             "     ON d.doc_id = r.doc_id                  "
             "  WHERE r.path = '/Mailer/Response/Received' "
             "    AND d.path = '/Mailer/Document/@cdr:ref' "
             "    AND d.int_val = ?                        ");
    stmt2.setInt(1, docId);
    cdr::db::ResultSet rs2 = stmt2.executeQuery();
    if (rs2.next())
        mailerResponseDate = rs2.getString(1).toUtf8();
    stmt2.close();
    //std::cerr << "mailerResponseDate=" << mailerResponseDate.c_str()
    //          << "\n";
    if (!verificationDate.empty())
        latestDate = verificationDate;
    if (!importDate.empty() && (latestDate.empty() || importDate > latestDate))
        latestDate = importDate;
    if (!mailerResponseDate.empty())
        if (latestDate.empty() || mailerResponseDate > latestDate)
            latestDate = mailerResponseDate;
    //std::cerr << "latestDate=" << latestDate.c_str() << "\n";
    if (!latestDate.empty())
        return "<VerificationDate>" + latestDate.substr(0, 10)
             + "</VerificationDate>";
    if (!lastModDate.empty())
        return "<VerificationDate>" + lastModDate.substr(0, 10)
             + "</VerificationDate>";
    cdr::String dateFirstPub = cdr::getDateFirstPublished(docId, conn);
    //std::wcerr << L"dateFirstPub=" << dateFirstPub.c_str() << L"\n";
    if (!dateFirstPub.empty())
        return "<VerificationDate>" + dateFirstPub.toUtf8().substr(0, 10)
             + "</VerificationDate>";
    return "<VerificationDate/>";
}

/**
 * Execute an SQL query on behalf of an XSLT filter request.
 *
 * Supports "cdrutil:sql-query" XSLT extension function callback.
 *
 * Example parms in call:
 *   "cdrutil:/sql-query/SELECT id, title FROM document WHERE id=?~12345".
 *
 * Example return:
 *   <?xml version='1.0' encoding='UTF-8'?>
 *   <SqlResult>
 *    <row id='1'>
 *     <col id='1' name='id'>12345</col>
 *     <col id='2' name='title'>Title of this particular doc</col>
 *    </row>
 *   </SqlResult>
 *
 * Nulls are handled by returning an empty element with an attribute
 * "null" = "Y", for example:
 *     <col id='2' name='title' null='Y'/>
 *
 * @params parms    Tilde delimited string.
 *                  We use tildes rather than slashes because slashes
 *                  may be needed in SQL queries (e.g., in query_term
 *                  path selections.)
 *                  The first piece of the string is the SQL query itself.
 *                  Optional subsequent tildes delimit question mark
 *                  subsitution parameters.
 *
 * @params conn     Database connection.
 *
 * @return          Serial SqlResult document, as described above.
 *
 * @throw           cdr::Exception from lower level database calls.
 */
static string execXsltSqlQuery(const string& parms,
                               cdr::db::Connection& conn) {

// DEBUG
// std::cout << "SqlQuery parms=\"" << parms << "\"" << std::endl;

    // Hold the query's optional parms
    vector<string> qryParms;

    // Output goes here
    string result("<?xml version='1.0' encoding='UTF-8'?>\n<SqlResult>");

    string qry;             // Query, isolated from its parameters
    int qmarkCount   = 0;   // Number of question mark parameters in query
    int qryParmCount = 0;   // Number of corresponding parms, must match

    // First string piece is always the query string, parm count is
    //   count of string pieces after that
    int pos;
    if ((pos = parms.find("~")) == string::npos)
        qry = parms;
    else
        qry = parms.substr(0, pos);

    // Empty query?
    if (qry.size() < 1)
        throw cdr::Exception(L"CdrFilter execXsltSqlQuery: Empty query");

    // Refuse query containing possible SQL update commands
    if (S_unsafeWordsPat.match(qry.c_str()))
        throw cdr::Exception(L"CdrFilter execXsltSqlQuery: "
            L" Query contains disallowed SQL keywords, sorry.");

    // Make individual strings from parms
    if (pos != string::npos) {
        int start = pos + 1;
        while ((pos = parms.find_first_of("~", start)) != string::npos) {
            qryParms.push_back(parms.substr(start, pos-start));
            start = pos + 1;
        }
        // Parm found after last '~'
        qryParms.push_back(parms.substr(start, string::npos));

        qryParmCount = qryParms.size();
    }

    // Count question mark placeholders in the query
    pos = 0;
    while ((pos = qry.find_first_of("?", pos)) != string.npos) {
        ++qmarkCount;
        ++pos;
    }

    // Counts need to match
    if (qmarkCount != qryParmCount)
          throw cdr::Exception(L"CdrFilter::execXsltSqlQuery:"
                L" qmark, qryParm count mismatch in query: " +
                cdr::String(parms));

    // Prepare and execute the query
    cdr::db::PreparedStatement qryStmt = conn.prepareStatement(qry);
    for (int i=0; i<qmarkCount; i++)
        qryStmt.setString(i+1, qryParms[i]);
    cdr::db::ResultSet rs = qryStmt.executeQuery();

    // Analyze results
    cdr::db::ResultSetMetaData metaData = rs.getMetaData();
    int colCount = metaData.getColumnCount();
    vector<string> colNames;
    for (int i=1; i<=colCount; i++) {
        string colName(metaData.getColumnName(i).toUtf8());
        colNames.push_back(colName);
    }

    // Put the results into XML
    int rowId = 1;
    while (rs.next()) {
        // New row
        char buf[1024];
        sprintf(buf, "\n <row id='%d'>", rowId);
        result += buf;

        // All columns
        for (int i=1; i<= colCount; i++) {
            cdr::String wValue = cdr::entConvert(rs.getString(i));
            sprintf(buf, "\n  <col id='%d' name='%s'", i,
                    colNames[i-1].c_str());
            // sprintf(buf, "\n  <col id='%d'", i);
            result += buf;
            if (wValue.isNull())
                // Show difference between empty string and null
                result += " null='Y'/>";
            else
                // Copy in full value
                result += ">" + wValue.toUtf8() + "</col>";
        }

        // Row termination
        result += "\n </row>";
        ++ rowId;
    }

    // Final XML terminator
    result += "\n</SqlResult>";

// DEBUG
// std::cout << "End result=\n" << result << std::endl;

    return result;
}

/**
 * Examine a list of protocol IDs to find secondary IDs that are
 * not included in a list of primary IDs, using a normalized string
 * comparison.
 *
 * This is a highly specialized custom function designed to support
 * addition of PDQProtocolIDs to CTGovProtocolIDs without duplicating
 * any of them.
 *
 * Example parms in call:
 *   "cdrutil:/dedup-ids/swog1234~ecog-123~~NCI-442~SWOG 1234~ecog-123~Pf-99"
 *
 * Example return for above:
 *   <?xml version='1.0' encode='UTF-8'?>
 *   <result>
 *    <id>NCI-442</id>
 *    <id>Pf-99</id>
 *   </result>
 *
 * @params parms    Tilde delimited string.
 *                  One tilde between each pair of IDs.
 *                  Two tildes (i.e., empty ID) between primary IDs and
 *                  secondary IDs.  Only the unique secondary IDs will be
 *                  returned.
 *
 * @params conn     Database connection.
 *
 * @return          Serial result document, as illustrated above.
 */
static string execXsltDedupIDs(const string& parms) {

    if (parms.size() == 0)
        throw cdr::Exception("Empty parameter string to execXsltDedupIDs()");

    // All the input IDs are parsed here
    int buflen = parms.size() + 1;
    char *buf;         // Plain IDs copied here for tokenizing
    char *normBuf;     // Normalized here

    // Pointers to the two buffers
    char *bufp;
    char *normp;

    std::vector<char *> inIDs;      // Input ID pointers
    std::vector<char *> normIDs;    // Ptrs to normalized IDs

    // Output XML string
    std::string result;

    // Use try wrapper to avoid memory leak on allocations
    try {
        buf     = new char[buflen];
        normBuf = new char[buflen];
        bufp    = buf;
        normp   = normBuf;

        strcpy(buf, parms.c_str());

        // Parse the input converting ~ -> 0 and storing pointers in inIDs
        int primeCount = parseIDs(buf, inIDs);
        int idCount    = inIDs.size();

        // Normalize all of the IDs in a separate copy
        for (int i=0; i<idCount; i++) {
            // Copy from buf to normBuf
            bufp = inIDs[i];
            normIDs.push_back(normp);
            while (*bufp) {
                char c = *bufp++;
                // Accept only normalized case alphanumerics
                if (isupper(c))
                    *normp++ = tolower(c);
                else if (isalpha(c) || isdigit(c))
                    *normp++ = c;
            }
            // Terminate the normalized string
            *normp++ = (char) 0;
        }

        // Compare them, starting with the first secondary ID
        // We compare each string to all previous ones.
        // If they are the same:
        //   Keep the previous one and null out the later one.
        for (int i=primeCount; i<idCount; i++) {
            for (int j=0; j<i; j++) {
                if (normIDs[j] && !strcmp(normIDs[j], normIDs[i])) {
                    normIDs[i] = NULL;
                    break;
                }
            }
        }

        // Construct output as an xml string
        result = "<?xml version='1.0' encoding='UTF-8'?>\n<result>\n";
        for (int i=primeCount; i<idCount; i++) {
            if (normIDs[i]) {
                result += " <id>";
                result += inIDs[i];
                result += "</id>\n";
            }
        }

        result += "</result>\n";
    } catch (...) {
        if (normBuf)
            delete normBuf;
        if (buf)
            delete buf;
        throw;
    }

    return result;
}


/**
 * Parse a tilde delimited list of primary and secondary IDs.
 * Helper function for execXsltDedupIDs.
 *
 * This function is separated from the rest of the logic in case the
 * input format needs to change.
 *
 * @params inStr    Input tilde delimited C style char string, null terminated.
 *                  See execXsltDedupIDs.
 *
 * @params inIDs    Empty vector of pointers to chars to receive
 *                  parsed IDs as null delimited C strings.
 *
 * @return          Index of first pointer in inIDs that is a secondary ID.
 *
 * @throws          cdr::Exception if memory overflow (should never happen).
 */
static int parseIDs(char *inStr, std::vector<char *>& inIDs) {

    int tokenCount = 0;         // Number of IDs found
    int primeCount = 0;         // Count that are primary IDs (before ~~)

std::cout << "Entering parseIDs\n";
    char *inp = inStr;

    // Parse till end of inStr
    while (*inp) {

        // Install pointer to token
        inIDs.push_back(inp);
        while (*inp) {
            if (*inp == '~') {

                // Terminate token in the original string
                *inp = (char) 0;
                ++tokenCount;

                // Two separators in a row separate primary/secondary IDs
                if (*(inp+1) == '~') {
                    primeCount = tokenCount;
                    ++inp;
                }
                ++inp;
                break;
            }
            ++inp;
        }
    }

std::cout << "Leaving parseIDs, tokenCount=" << tokenCount
          << " primeCount=" << primeCount << "\n";
    return primeCount;
}


/**
 * Search the external_map table for a value in a specific usage.
 * If found, tell caller how to map the value.
 * If not found, create an entry in the table for it.
 *
 * @param parms     Usage/Value, slash separated string.
 *
 * @return          DocID to which value should map.
 *                  Empty if data not mapped.
 *                  "DROP" if data is "bogus".
 */
string lookupExternalValue(const string& parms,
                           cdr::db::Connection& conn)
{
    // Switch from utf-8 to 16-bit encoding.
    cdr::String parmString = parms;

    // Split the parameters into two pieces.
    size_t sep = parmString.find(L"/");
    if (sep == parmString.npos || sep == 0)
        throw cdr::Exception(L"Invalid parm for extern-map function",
                             parmString);
    cdr::String usageName = parmString.substr(0, sep++);
    cdr::String externName = parmString.substr(sep);
    if (externName.empty())
        throw cdr::Exception(L"External name missing from extern-map "
                             L"function");

    // Look up the external value.
    cdr::db::PreparedStatement stmt = conn.prepareStatement(
            "SELECT m.doc_id, bogus, mappable "
            "  FROM external_map m       "
            "  JOIN external_map_usage u "
            "    ON u.id = m.usage       "
            " WHERE u.name = ?           "
            "   AND m.value = ?          ");
    stmt.setString(1, usageName);
    stmt.setString(2, externName);
    cdr::db::ResultSet rs = stmt.executeQuery();
    if (!rs.next()) {

        // Value was not found.  Is it a mappable pattern?
        // Find out by getting a regex of non-mappable patterns
        // RegEx constructed for case insensitive searching = true
        cdr::RegEx re(true);
        makeNomapRegex(re, conn);

        // By default it will be mappable
        cdr::String mappable = L"Y";
        if (re.match(externName) && usageName == L"CT.gov Facilities")
            // It matched one of the patterns for non-mappable agency values
            mappable = L"N";

        // Insert the new value
        cdr::db::PreparedStatement s = conn.prepareStatement(
            "INSERT INTO external_map (usage, value, last_mod, mappable) "
            "SELECT id, ?, GETDATE(), ?  "
            "  FROM external_map_usage   "
            " WHERE name = ?             ");
        s.setString(1, externName);
        s.setString(2, mappable);
        s.setString(3, usageName);
        s.executeUpdate();
        return "<DocId/>";
    }

    int docId = rs.getInt(1);
    cdr::String bogus = rs.getString(2);
    cdr::String mappable = rs.getString(3);

    // Bogus records are known false values sometimes seen in docs
    if (bogus == L"Y")
        return "<DocId>DROP</DocId>";

    // If not mapped, or non-mappable, return an empty doc id.
    // Not mapped are values encountered but not yet mapped.
    // Non-mappable are values such as notes or comments entered by
    //   an external source in lieu of a real value, e.g., "call ...
    //   for contact information"
    if (mappable == L"N" || !docId)
        return "<DocId/>";

    cdr::String idString = cdr::stringDocId(docId);
    std::ostringstream os;
    os << "<DocId>" << idString.toUtf8() << "</DocId>";
    return os.str();
}

//  Map filter strings to ids
void cdr::buildFilterString2IdMap() {

    // To enable filter profiling, a programmer must set an
    //   environment variable and restart the server.
    // If the variable is not set, no filter name to ID map
    //   is constructed and no profiling takes place.
    if (!getenv("CDR_FILTER_PROFILING"))
        return;

    // Build map of filter names to IDs on the heap
    std::map<std::string, int> *pMap = new std::map<std::string, int>;

    // Don't bother to try/catch.  Just let exceptions go up the stack
    cdr::db::Connection dbConn =
        cdr::db::DriverManager::getConnection (cdr::db::url,
                                               cdr::db::uid,
                                               cdr::db::pwd);

    // Get all docs of type Filter
    cdr::db::Statement stmt = dbConn.createStatement();
    cdr::db::ResultSet rs = stmt.executeQuery (
        "SELECT d.id, d.xml "
        "  FROM document d, doc_type t "
        " WHERE d.doc_type = t.id "
        "   AND t.name = 'Filter'");

    // Put each in map
    while (rs.next()) {
        int id = rs.getInt(1);
        cdr::String xml = rs.getString(2);

        // Insert as utf-8, the format used in processStrings
        std::string utf = xml.toUtf8();
        (*pMap)[utf] = id;
    }

    // Store this in static memory
    S_pFltrIdMap = pMap;
}

/**
 * Create a big regular expression from the non-mappable patterns
 * stored in the external_map_nomap_pattern table.
 *
 *  @param re       Reference to regular expression object to hold pattern.
 *                  Caller should create it with ignore case set.
 *  @param conn     Database connection.
 *
 *  @return         Void, caller's re is set to pattern.
 *  @throw          cdr::Exception if pattern error or memory blowout
 */
// Pattern buf size, big enough for 1-2,000+ patterns of expected size.
const int MAX_PAT_BUF = 100000;

// Map of ASCII chars that must be escaped
typedef unsigned short PATCHAR;
const int MAX_ESC = 128;
static PATCHAR escape[MAX_ESC];
static unsigned char escapeChars[] = "^$()[]*+.|\\";

// If true, escape list not yet initialized
static bool noEscape = true;

static void makeNomapRegex (
    cdr::RegEx& re,
    cdr::db::Connection& conn
) {
    // Build pattern here.  Allocate some overflow area just in case
    wchar_t patBuf[MAX_PAT_BUF + 8];

    // Point to start and end
    wchar_t *nextp = patBuf;
    wchar_t *endp  = patBuf + MAX_PAT_BUF;

    // Seed table of escape chars if not yet done
    // Static array initialized by compiler to all 0
    if (noEscape) {
        for (int i=0; i<sizeof(escapeChars); i++) {
            PATCHAR c = escapeChars[i];
            escape[c] = 1;
        }
        noEscape = false;
    }

    // Select all of the values in the table
    cdr::db::PreparedStatement selStmt = conn.prepareStatement(
                "SELECT pattern FROM external_map_nomap_pattern");
    cdr::db::ResultSet rs = selStmt.executeQuery();

    // Need OR symbol before all but the first pattern
    bool firstPat = true;

    // Put each pattern into the big pattern string
    while (rs.next()) {
        cdr::String nomapPattern = rs.getString(1);
        const wchar_t *nPatp = nomapPattern.c_str();

        // Set alternation if needed
        if (!firstPat)
            *nextp++ = (PATCHAR) '|';
        firstPat = false;

        // Each subpattern must match starting at the
        //   beginning of a value string
        *nextp++ = (PATCHAR) '(';
        *nextp++ = (PATCHAR) '^';

        // Process each char until done or buffer full
        while (*nPatp && nextp < endp) {
            PATCHAR c = *nPatp++;

            // Translate SQL wildcard to regex
            if (c == '%') {
                *nextp++ = (PATCHAR) '.';
                *nextp++ = (PATCHAR) '*';
                continue;
            }
            // Check for chars needing backslash escapes
            // All escape chars are in 7 bit ASCII set
            if (c < MAX_ESC && escape[c])
                *nextp++ = (PATCHAR) '\\';

            // Save actual char
            *nextp++ = c;
        }
        *nextp++ = (PATCHAR) ')';

        // Did we run out of memory?
        if (nextp >= endp)
            // Somebody has to recompile with higher value
            throw cdr::Exception(L"makeNomapRegex ran out of memory.  "
                                 L"Please inform support staff");
    }

    // Terminate the whole thing
    *nextp++ = (PATCHAR) 0;

    // DEBUG - Log the complete regex
    // cdr::log::WriteFile("makeNomapRegex pattern: ", patBuf);

    // Initialize the regular expression
    re.set(patBuf);
}
