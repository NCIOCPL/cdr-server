/*
 * $Id: CdrPublish.cpp,v 1.1 2002-04-04 01:04:41 bkline Exp $
 *
 * Commands to create a new publishing job and retrieve status for an 
 * existing publishing job.
 *
 * $Log: not supported by cvs2svn $
 */

// Eliminate annoying warnings about truncated debugging information.
#pragma warning(disable : 4786)

#include "CdrDom.h"
#include "CdrCommand.h"
#include "CdrException.h"
#include "CdrDbResultSet.h"
#include "CdrDbPreparedStatement.h"

// Local functions.
static cdr::dom::Element getControlDoc(
        const cdr::String& pubSystemName,
        const cdr::String& jobTime,
        cdr::db::Connection& conn,
        int* controlDocId);
static cdr::dom::Node getSubsetElem(
        const cdr::dom::Node&,
        const cdr::String& requestedName);
static void getSubsetParms(
        const cdr::dom::Node& subsetElem, 
        cdr::NamedValues* controlParms);
static void getSubsetOptions(
        const cdr::dom::Node& subsetElem, 
        cdr::NamedValues* options);
static void checkAuthorization(
        const cdr::dom::Node& subsetNode,
        cdr::Session& session,
        cdr::db::Connection& conn);
static int createJob(
        int controlDocId,
        const cdr::String& pubSubsetName, 
        int usrId,
        const cdr::String& baseDir,
        const cdr::String& jobTime,
        const cdr::String& email,
        const cdr::String& noOutput,
        cdr::db::Connection& conn);
static void insertParameterRow(
        int jobId, 
        int parmId, 
        const cdr::String& name, 
        const cdr::String& value,
        cdr::db::Connection& conn);
static void insertDocument(
        int jobId, 
        int docId,
        int userDocVersion,
        const cdr::String& jobTime,
        cdr::db::Connection& conn);

/**
 * Create a new publishing job.
 */
cdr::String cdr::publish(Session& session,
                         const dom::Node& commandNode,
                         db::Connection& conn)
{
    // Get date/time for job.
    String jobTime = conn.getDateTimeString();

    // Extract command arguments
    String              pubSystemName;
    String              pubSubsetName;
    String              email(true);
    String              noOutput = L"N";
    NamedValues         controlParms;
    NamedValues         requestParms;
    NamedValues         jobParms;
    NamedValues         options;
    std::map<int, int>  docList;
    //StringList          messages;

    dom::Node child = commandNode.getFirstChild();
    while (child != NULL) {
        if (child.getNodeType() == dom::Node::ELEMENT_NODE) {
            String name = child.getNodeName();
            if (name == L"PubSystem")
                pubSystemName = dom::getTextContent(child);
            else if (name == L"PubSubset")
                pubSubsetName = dom::getTextContent(child);
            else if (name == L"Parms") {
                dom::Node parm = child.getFirstChild();
                while (parm != NULL) {
                    if (parm.getNodeType() == dom::Node::ELEMENT_NODE) {
                        String name = parm.getNodeName();
                        if (name != L"Parm")
                            throw Exception(L"Unexpected child of Parms", name);
                        String parmName;
                        String parmValue;
                        dom::Node child = parm.getFirstChild();
                        while (child != NULL) {
                            if (child. getNodeType() == 
                                    dom::Node::ELEMENT_NODE) {
                                String name = child.getNodeName();
                                if (name == L"Name")
                                    parmName = dom::getTextContent(child);
                                else if (name == L"Value")
                                    parmValue = dom::getTextContent(child);
                            }
                            child = child.getNextSibling();
                        }
                        if (parmName.empty())
                            throw Exception(L"Missing parameter name");
                        if (requestParms.find(parmName) != requestParms.end())
                            throw Exception(L"Duplicate parameter name",
                                    parmName);
                        requestParms[parmName] = parmValue;
                    }
                    parm = parm.getNextSibling();
                }
            }
            else if (name == L"DocList") {
                dom::Node doc = child.getFirstChild();
                while (doc != NULL) {
                    if (doc.getNodeType() == dom::Node::ELEMENT_NODE) {
                        String name = doc.getNodeName();
                        if (name != L"Doc")
                            throw Exception(L"Unexpected child of DocList",
                                    name);
                        const dom::Element& elem =
                                static_cast<const dom::Element&>(doc);
                        String idAttr = elem.getAttribute(L"Id");
                        String verAttr = elem.getAttribute(L"Version");
                        if (idAttr.empty())
                            throw Exception(L"Missing Id attribute for Doc");
                        int id = idAttr.extractDocId();
                        int ver = verAttr.getInt();
                        if (id < 1)
                            throw Exception(L"Invalid doc ID", idAttr);
                        if (docList.find(id) != docList.end())
                            throw Exception(L"Duplicate doc ID", idAttr);
                        docList[id] = ver;
                    }
                    doc = doc.getNextSibling();
                }
            }
            else if (name == L"Email")
                email = dom::getTextContent(child);
            else if (name == L"NoOutput")
                noOutput = dom::getTextContent(child);
        }
        child = child.getNextSibling();
    }

    // Make sure we got the required elements.
    if (pubSystemName.empty() || pubSubsetName.empty())
        throw Exception(L"PubSystem and PubSubset elements are both required");

    // Normalize the noOutput value.
    if (noOutput != L"Y")
        noOutput = L"N";

    // Go get the control document for the specified publishing system.
    int controlDocId;
    dom::Element controlDocElem = getControlDoc(pubSystemName, jobTime, conn,
                                                &controlDocId);

    // Extract the DOM element for the requested subset.
    dom::Node subsetElem = getSubsetElem(controlDocElem, pubSubsetName);
    
    // Make sure the user is authorized to invoke the publishing job.
    checkAuthorization(subsetElem, session, conn);

    // Get the options specified for the subsystem.
    options[L"PublishIfWarnings"] = L"No";
    options[L"AbortOnError"]      = L"Yes";
    getSubsetOptions(subsetElem, &options);
    bool abortOnError = true;
    int errorsBeforeAbort = 0;
    if (options[L"AbortOnError"] == L"Yes")
        abortOnError = true;
    else if (options[L"AbortOnError"] == L"No")
        abortOnError = false;
    else
        errorsBeforeAbort = options[L"AbortOnError"].getInt();

    // Get the parameters specified for this subsytem.
    getSubsetParms(subsetElem, &controlParms);

    // Merge the defaults and the explicit parameters.
    NamedValues::const_iterator parmIter = requestParms.begin();
    while (parmIter != requestParms.end()) {
        String name  = parmIter->first;
        String value = parmIter->second;
        if (controlParms.find(name) == controlParms.end())
            throw Exception(L"Parameter not supported by subset", name);
        jobParms[name] = value;
        ++parmIter;
    }
    parmIter = controlParms.begin();
    while (parmIter != controlParms.end()) {
        String name  = parmIter->first;
        String value = parmIter->second;
        if (jobParms.find(name) == jobParms.end())
            jobParms[name] = value;
        ++parmIter;
    }

    // Get the base directory for the output.
    if (options.find(L"Destination") == options.end())
        throw Exception(L"Required Destination option not present in "
                        L"control document");
    String baseDir = options[L"Destination"];
    size_t baseDirLen = baseDir.size();
    while (baseDirLen-- > 0 && wcschr(L"/\\", baseDir[baseDirLen]))
        baseDir.erase(baseDirLen);

    // Everything we do from this point needs to be atomic.
    conn.setAutoCommit(false);

    // Create the row for the new job.
    int jobId = createJob(controlDocId, 
                          pubSubsetName, 
                          session.getUserId(), 
                          baseDir,
                          jobTime,
                          email,
                          noOutput, 
                          conn);

    // Create the pub_proc_parm rows.
    parmIter = jobParms.begin();
    int parmId = 1;
    while (parmIter != jobParms.end()) {
        String name  = parmIter->first;
        String value = parmIter->second;
        insertParameterRow(jobId, parmId++, name, value, conn);
        ++parmIter;
    }
    
    // Create the rows for the user-listed documents.
    std::map<int, int>::const_iterator docIter = docList.begin();
    while (docIter != docList.end()) {
        int docId  = docIter->first;
        int docVer = docIter->second;
        insertDocument(jobId, docId, docVer, jobTime, conn);
        docIter++;
    }

    // Report success.
    conn.commit();
    conn.setAutoCommit(true);
    return L"<CdrPublishResp><JobId>" + String::toString(jobId) +
           L"</JobId></CdrPublishResp>";
}

cdr::dom::Element getControlDoc(
        const cdr::String& pubSystemName,
        const cdr::String& jobTime,
        cdr::db::Connection& conn,
        int* controlDocId)
{
    // Retrieve the ID and XML for the control document.
    const char* query = " SELECT d.id,                       "
                        "        d.xml                       "
                        "   FROM doc_version d               "
                        "   JOIN doc_type t                  "
                        "     ON t.id = d.doc_type           "
                        "  WHERE d.title = ?                 "
                        "    AND t.name = 'PublishingSystem' "
                        "    AND d.dt <= ?                   ";
    cdr::db::PreparedStatement ps = conn.prepareStatement(query);
    ps.setString(1, pubSystemName);
    ps.setString(2, jobTime);
    cdr::db::ResultSet rs = ps.executeQuery();
    if (!rs.next())
        throw cdr::Exception(L"Unable to find control document", pubSystemName);
    *controlDocId   = rs.getInt(1);
    cdr::String xml = rs.getString(2);

    // Extract the DOM tree for the document.
    cdr::dom::Parser parser;
    try {
        parser.parse(xml);
    }
    catch (const cdr::dom::XMLException& e) {
        throw cdr::Exception (
                L"getControlDoc: Error parsing " + pubSystemName +
                L" control document: " + e.getMessage());
    }
    return parser.getDocument().getDocumentElement();
}

cdr::dom::Node getSubsetElem(
        const cdr::dom::Node& docElem,
        const cdr::String& requestedName)
{
    cdr::dom::Node subset = docElem.getFirstChild();
    while (subset != NULL) {
        if (subset.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = subset.getNodeName();
            if (name == L"SystemSubset") {
                cdr::dom::Node nameNode = subset.getFirstChild();
                while (nameNode != NULL) {
                    if (nameNode.getNodeType() == 
                            cdr::dom::Node::ELEMENT_NODE) {
                        cdr::String name = nameNode.getNodeName();
                        if (name == L"SubsetName") {
                            cdr::String subsetName = 
                                cdr::dom::getTextContent(nameNode);
                            if (subsetName == requestedName)
                                return subset;
                            break;
                        }
                    }
                    nameNode = nameNode.getNextSibling();
                }
            }
        }
        subset = subset.getNextSibling();
    }
    throw cdr::Exception(L"Unable to find publishing system subset",
                         requestedName);
}

void getSubsetParms(
        const cdr::dom::Node& subsetElem, 
        cdr::NamedValues* controlParms)
{
    cdr::dom::Node child = subsetElem.getFirstChild();
    while (child != NULL) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"SubsetParameters") {
                cdr::dom::Node parmNode = child.getFirstChild();
                while (parmNode != NULL) {
                    if (parmNode.getNodeType() == 
                            cdr::dom::Node::ELEMENT_NODE) {
                        cdr::String name = parmNode.getNodeName();
                        if (name != L"SubsetParameter")
                            throw cdr::Exception(L"Unexpected child element "
                                                 L"of SubsetParameters",
                                                 name);
                        cdr::dom::Node child = parmNode.getFirstChild();
                        cdr::String parmName;
                        cdr::String parmValue;
                        while (child != NULL) {
                            if (child.getNodeType() == 
                                    cdr::dom::Node::ELEMENT_NODE) {
                                cdr::String name = child.getNodeName();
                                if (name == L"ParmName")
                                    parmName = cdr::dom::getTextContent(child);
                                else if (name == L"ParmValue")
                                    parmValue =
                                        cdr::dom::getTextContent(child);
                                else
                                    throw cdr::Exception(L"Unexpected child "
                                                         L"element of "
                                                         L"SubsetParameter",
                                                         name);
                            }
                            child = child.getNextSibling();
                        }
                        if (parmName.empty())
                            throw cdr::Exception(L"Missing ParmName for "
                                                 L"SubsetParameter");
                        if (controlParms->find(parmName) != controlParms->end())
                            throw cdr::Exception(L"Duplicate subset parameter",
                                                 parmName);
                        (*controlParms)[parmName] = parmValue;
                    }
                    parmNode = parmNode.getNextSibling();
                }
            }
        }
        child = child.getNextSibling();
    }
}

void getSubsetOptions(
        const cdr::dom::Node& subsetElem, 
        cdr::NamedValues* options)
{
    cdr::dom::Node child = subsetElem.getFirstChild();
    while (child != NULL) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"SubsetOptions") {
                cdr::dom::Node option = child.getFirstChild();
                while (option != NULL) {
                    if (option.getNodeType() == 
                            cdr::dom::Node::ELEMENT_NODE) {
                        cdr::String name = option.getNodeName();
                        if (name != L"SubsetOption")
                            throw cdr::Exception(L"Unexpected child element "
                                                 L"of SubsetOptions",
                                                 name);
                        cdr::dom::Node child = option.getFirstChild();
                        cdr::String optionName;
                        cdr::String optionValue;
                        while (child != NULL) {
                            if (child.getNodeType() == 
                                    cdr::dom::Node::ELEMENT_NODE) {
                                cdr::String name = child.getNodeName();
                                if (name == L"OptionName")
                                    optionName = 
                                        cdr::dom::getTextContent(child);
                                else if (name == L"OptionValue")
                                    optionValue =
                                        cdr::dom::getTextContent(child);
                                else
                                    throw cdr::Exception(L"Unexpected child "
                                                         L"element of "
                                                         L"SubsetOption",
                                                         name);
                            }
                            child = child.getNextSibling();
                        }
                        if (optionName.empty())
                            throw cdr::Exception(L"Missing OptionName for "
                                                 L"SubsetOption");
                        (*options)[optionName] = optionValue;
                    }
                    option = option.getNextSibling();
                }
            }
        }
        child = child.getNextSibling();
    }
}

void checkAuthorization(
        const cdr::dom::Node& subsetNode,
        cdr::Session& session,
        cdr::db::Connection& conn)
{
    cdr::dom::Node child = subsetNode.getFirstChild();
    while (child != NULL) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"SubsetActionName") {
                cdr::String action = cdr::dom::getTextContent(child);
                if (!session.canDo(conn, action, L""))
                    throw cdr::Exception(L"User not authorized to perform "
                                         L"action", action);
                return;
            }
        }
        child = child.getNextSibling();
    }
}

int createJob(
        int controlDocId,
        const cdr::String& pubSubsetName, 
        int usrId,
        const cdr::String& baseDir,
        const cdr::String& jobTime,
        const cdr::String& email,
        const cdr::String& noOutput,
        cdr::db::Connection& conn)
{
    const char* query = " INSERT INTO pub_proc    "
                        " (                       "
                        "             pub_system, "
                        "             pub_subset, "
                        "             usr,        "
                        "             output_dir, "
                        "             started,    "
                        "             completed,  "
                        "             status,     "
                        "             messages,   "
                        "             email,      "
                        "             no_output   "
                        " )                       "
                        "      VALUES             "
                        " (                       "
                        "              ?,         "
                        "              ?,         "
                        "              ?,         "
                        "              '',        "
                        "              ?,         "
                        "              NULL,      "
                        "              'Init',    "
                        "              NULL,      "
                        "              ?,         "
                        "              ?          "
                        " )                       ";
    cdr::db::PreparedStatement ps = conn.prepareStatement(query);
    ps.setInt   (1, controlDocId);
    ps.setString(2, pubSubsetName);
    ps.setInt   (3, usrId);
    ps.setString(4, jobTime);
    ps.setString(5, email);
    ps.setString(6, noOutput);
    ps.executeUpdate();

    // Get the job ID and use it to modify the output directory.
    int jobId = conn.getLastIdent();
    cdr::db::PreparedStatement update = conn.prepareStatement(
            " UPDATE pub_proc       "
            "    SET output_dir = ? "
            "  WHERE id = ?         ");
    update.setString(1, baseDir + L"/Job" + cdr::String::toString(jobId));
    update.setInt   (2, jobId);
    update.executeUpdate();

    // The caller wants the job ID.
    return jobId;
}

void insertParameterRow(
        int jobId, 
        int parmId, 
        const cdr::String& name, 
        const cdr::String& value,
        cdr::db::Connection& conn)
{
    cdr::db::PreparedStatement ps = conn.prepareStatement(
            " INSERT INTO pub_proc_parm "
            " (                         "
            "             id,           "
            "             pub_proc,     "
            "             parm_name,    "
            "             parm_value    "
            " )                         "
            "      VALUES               "
            " (                         "
            "             ?,            "
            "             ?,            "
            "             ?,            "
            "             ?             "
            " )                         ");
    ps.setInt   (1, parmId);
    ps.setInt   (2, jobId);
    ps.setString(3, name);
    ps.setString(4, value);
    ps.executeUpdate();
}

void insertDocument(
        int jobId, 
        int docId,
        int userDocVersion,
        const cdr::String& jobTime,
        cdr::db::Connection& conn)
{
    // Verify or select the document version to be published.
    const char* query = userDocVersion < 1 ? " SELECT MAX(num)          "
                                             "   FROM doc_version       "
                                             "  WHERE id = ?            "
                                             "    AND publishable = 'Y' "
                                             "    AND val_status = 'V'  "
                                             "    AND dt <= ?           "
                                           : " SELECT num               "
                                             "   FROM doc_version       "
                                             "  WHERE id = ?            "
                                             "    AND publishable = 'Y' "
                                             "    AND val_status = 'V'  "
                                             "    AND num = ?           ";
    cdr::db::PreparedStatement select = conn.prepareStatement(query);
    select.setInt(1, docId);
    if (userDocVersion < 1)
        select.setString(2, jobTime);
    else
        select.setInt(2, userDocVersion);
    cdr::db::ResultSet rs = select.executeQuery();
    if (!rs.next()) {
        if (userDocVersion < 1)
            throw cdr::Exception(L"No publishable version found for document ",
                                 cdr::stringDocId(docId));
        else
            throw cdr::Exception(L"Version " + 
                                 cdr::String::toString(userDocVersion) +
                                 L" not publishable for document ",
                                 cdr::stringDocId(docId));
    }
    int docVersion = rs.getInt(1);

    // Insert the row into the pub_proc_doc table.
    cdr::db::PreparedStatement insert = conn.prepareStatement(
            " INSERT INTO pub_proc_doc "
            " (                        "
            "             pub_proc,    "
            "             doc_id,      "
            "             doc_version  "
            " )                        "
            "      VALUES              "
            " (                        "
            "             ?,           "
            "             ?,           "
            "             ?            "
            " )                        ");
    insert.setInt(1, jobId);
    insert.setInt(2, docId);
    insert.setInt(3, docVersion);
    insert.executeUpdate();
}
