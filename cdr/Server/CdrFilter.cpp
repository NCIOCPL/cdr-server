/*
 * $Id: CdrFilter.cpp,v 1.16 2002-01-31 21:35:09 mruben Exp $
 *
 * Applies XSLT scripts to a document
 *
 * $Log: not supported by cvs2svn $
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
 *
 */

#if defined _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <cstdio>
#include <sstream>
#include <ctime>
#include <string>
#include <utility>
#include <vector>

#include "sablot.h"

#include "CdrCommand.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"
#include "CdrException.h"
#include "CdrFilter.h"
#include "CdrGetDoc.h"
#include "CdrString.h"
#include "CdrVersion.h"

using std::pair;
using std::string;
using std::vector;
using std::wostringstream;

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
  
  // unique type used to get CdrDocCtl
  const wchar_t CdrDocType[] = L"";

  // unique type used to get filter by name
  const wchar_t CdrFilterType[] = L"";

  class ParmList
  {
    public:
      ~ParmList();
      void addparm(const cdr::String& name, const cdr::String& value);
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
  /* is owned by the caller and must be delete[]ed.                          */
  /***************************************************************************/
  char* parmstr(const cdr::String& s)
  {
    string us = s.toUtf8();
    char* p = new char[s.length() + 1];
    strcpy(p, us.c_str());
    return p;
  }

  void ParmList::addparm(const cdr::String& name, const cdr::String& value)
  {
    if (parms.size() == 0)
    {
      parms.push_back(parmstr(name));
      parms.push_back(parmstr(value));
    }
    else
    {
      parms[parms.size() - 2] = parmstr(name);
      parms[parms.size() - 1] = parmstr(value);
    }
    parms.push_back(NULL);
    parms.push_back(NULL);
  }

  /***************************************************************************/
  /* get document from the repository.  Error if type not NULL and document  */
  /* does not have this type                                                 */
  /***************************************************************************/
  cdr::String getDocument(cdr::String ui,
                          int version,
                          cdr::db::Connection& connection,
                          const wchar_t* type = NULL)
  {
    if (type == CdrDocType)
      return cdr::getDocString(ui, connection, version, true, false);

    cdr::String docstring(cdr::getDocString(ui, connection, version, true,
                                            type != CdrFilterType));

    cdr::dom::Parser parser;
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
                             + child.getNodeValue()
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
                              cdr::String* docui)
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

    string query = "SELECT id "
                   "FROM document "
                   "WHERE title=? "
                   "GROUP BY id";

    cdr::db::PreparedStatement select = connection.prepareStatement(query);
    select.setString(1, name);
    cdr::db::ResultSet rs = select.executeQuery();
    if (!rs.next())
      throw cdr::Exception(L"No document with title: " + name);

    int id = rs.getInt(1);

    if (rs.next())
      throw cdr::Exception(L"Multiple documents with title: " + name);

    return getDocument(cdr::stringDocId(id), 0, connection, CdrFilterType);
  }


  /***************************************************************************/
  /* get document from document spec (XML)                                   */
  /* If type is not null and document is retrieved from CDR, its DocType     */
  /* must match type.  If ui is not null, the ui will be returned in it.     */
  /* docui is used when getting a filter -- it contains the ui of the        */
  /* document to which the filter will be applied                            */
  /***************************************************************************/
  cdr::String getDocument(cdr::dom::Node docspec,
                          cdr::db::Connection& connection,
                          const wchar_t* type = NULL,
                          cdr::String* ui = NULL, cdr::String* docui = NULL)
  {
    cdr::dom::NamedNodeMap attributes = docspec.getAttributes();

    cdr::dom::Node name = attributes.getNamedItem("Name");
    if (name != NULL)
      return getFilterByName(name.getNodeValue(), connection, docui);

    cdr::dom::Node href = attributes.getNamedItem("href");
    if (href != NULL)
    {
      cdr::String u = href.getNodeValue();
      if (ui != NULL)
        *ui = u;

      return getDocument(u, 0, connection, type);
    }

    cdr::dom::Node f = docspec.getFirstChild();
    if (f.getNodeType() != cdr::dom::Node::CDATA_SECTION_NODE)
      throw cdr::Exception(L"Invalid filter specification");

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
      ThreadData(cdr::db::Connection& c, cdr::String id)
        : connection(c), DocId(id)
        {}
      string filter_messages;
      vector<UriInfo> uri_list;
      cdr::db::Connection& connection;
      cdr::String DocId;
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
        wostringstream msg;
        msg << L"XSLT error: code: " << code;
        if (fields != NULL)
          while (*fields != NULL)
            msg << cdr::String(*(fields++)) << L"\n";

        throw cdr::Exception(msg.str());
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

  /***************************************************************************/
  /* Sablotron error/message handling callbacks                              */
  /***************************************************************************/
  int uri_open(void* data, SablotHandle, const char* scheme,
               const char* rest, int* handle)
  {
    if (scheme == NULL)
      return SH_ERR_UNSUPPORTED_SCHEME;

    ThreadData* thread_data = static_cast<ThreadData*>(data);
    UriInfo u;

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
              && version_str != "last")
          {
            spec = version_str;
            version_str = "";
          }
        }
        type = spec;

        int version = 0;
        if (version_str == L"last")
          version = cdr::getVersionNumber(uid.extractDocId(), connection);
        else
          if (!version_str.empty())
            version = version_str.getInt();

        if (type == L"CdrCtl")
          u.doc = cdr::getDocCtlString(uid, version, connection,
                               cdr::DocCtlComponents::all).toUtf8();
        else
          u.doc = getDocument(uid, version, connection).toUtf8();
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
    char* s = NULL;
    char* d = NULL;
    char* r = NULL;

    try
    {
      // We have to go through some contortions here to avoid sending a const
      // char* to sablotron.  This probably is not necessary, but the function
      // is not declared as taking a const char* so it seems safer.

      s = new char[script.length() + 1];
      strcpy(s, script.c_str());
      d = new char[document.length() + 1];
      strcpy(d, document.c_str());

      ParmList p;
      if (parms != NULL)
        for (cdr::FilterParmVector::iterator ip = parms->begin();
             ip != parms->end();
             ++ip)
          p.addparm(ip->first, ip->second);

      char *arguments[] = { "/_stylesheet", s,
                            "/_xmlinput", d,
                            "/_output", NULL,
                            NULL
                          };
      void* proc;
      int rc;
      try
      {
        if ((rc = SablotCreateProcessor(&proc)))
          throw cdr::Exception(L"Cannot create XSLT processor");

        MessageHandler mh = { makeCode, messageHandler, messageHandler };
        if (SablotRegHandler(proc, HLR_MESSAGE, &mh, thread_data))
          throw cdr::Exception(L"cannot register Sablotron message handler");

        SchemeHandler sh = { uri_getall, uri_free, uri_open,
                             uri_get, uri_put,  uri_close
        };
        if (SablotRegHandler(proc, HLR_SCHEME, &sh, thread_data))
          throw cdr::Exception(L"cannot register Sablotron scheme handler");

        char** pparms = p.g_parms();
        if ((rc = SablotRunProcessor(proc, "arg:/_stylesheet",
                                     "arg:/_xmlinput", "arg:/_output",
                                     pparms, arguments)))
          throw cdr::Exception(L"XSLT error");

        if ((rc = SablotGetResultArg(proc, "arg:/_output", &r)))
          throw cdr::Exception(L"Cannot get XSLT result");
      }
      catch (...)
      {
        SablotDestroyProcessor(proc);
        throw;
      }

      if ((rc = SablotDestroyProcessor(proc)))
        throw cdr::Exception(L"Cannot destroy XSLT processor");

      result = r;
      delete[] s;
      delete[] d;
      SablotFree(r);
      return rc;
    }
    catch (...)
    {
      delete[] s;
      delete[] d;
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
  std::string qry = "SELECT xml FROM document WHERE id = ?";
  cdr::db::PreparedStatement stmt = connection.prepareStatement (qry);
  stmt.setInt (1, filterId);
  cdr::db::ResultSet rs = stmt.executeQuery();
  if (!rs.next())
      throw cdr::Exception (L"filterDocument: can't find filter for id="
                            + cdr::String::toString (filterId));
  cdr::String filterXml = rs.getString (1);

  // Invoke existing function, passing the script
  return filterDocument (document, filterXml, connection, messages,
                         parms, doc_id);
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
  if (processStrings(filter.toUtf8(), document.toUtf8(), result,
                     connection, parms, &thread_data))
      throw cdr::Exception(L"error in XSLT processing");

  if (messages != NULL)
    *messages = thread_data.filter_messages;

  return result;
}

cdr::String cdr::filter(cdr::Session& session,
                        const cdr::dom::Node& commandNode,
                        cdr::db::Connection& connection)
{
  vector<cdr::String> filters;
  cdr::String document;
  cdr::String ui;

  // check if we're to do output
  bool output = true;
  cdr::dom::NamedNodeMap cmdattr = commandNode.getAttributes();
  cdr::dom::Node attr = cmdattr.getNamedItem("Output");

  cdr::String foo;

  if (attr != NULL && cdr::String(attr.getNodeValue()) == L"N")
    output = false;

  // we need to get the document first so we'll have it's type if needed
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
          const wchar_t* type = NULL;
          cdr::dom::NamedNodeMap attributes = child.getAttributes();
          cdr::dom::Node ctl = attributes.getNamedItem("ctl");
          if (ctl != NULL)
          {
            cdr::String val = ctl.getNodeValue();
            if (val == L"y")
              type = CdrDocType;
          }

          document = getDocument(child, connection, type, &ui);

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
                                        NULL, &ui));
        else
        if (name == L"Parms")
        {
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

  string doc(document.toUtf8());
  string result(doc);
  ThreadData thread_data(connection, ui);
  for (std::vector<cdr::String>::iterator i = filters.begin();
       i != filters.end();
       ++i)
  {
    if (processStrings(i->toUtf8(), doc, result, connection, &pvector,
                       &thread_data))
      throw cdr::Exception(L"error in XSLT processing");

    doc = result;
  }

  result = output ? "<Document><![CDATA[" + result + "]]></Document>\n" : "";
  if (thread_data.filter_messages.length() != 0)
    result += "<Messages>" + thread_data.filter_messages + "</Messages>\n";

  return L"<CdrFilterResp>"
       + result
       + L"</CdrFilterResp>\n";
}
