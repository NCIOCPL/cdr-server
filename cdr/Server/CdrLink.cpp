#pragma warning(disable : 4786) // While including iostream
#include <iostream> // DEBUG

/* XXXX TO DO XXXX
    Optimize update by only processing changes?
    Maybe.  Have to see if it's worthwhile.
   XXXX TO DO XXXX */

/*
 * CdrLink.cpp
 *
 * Module for creating, maintaining, and searching the link network
 * stored in the link_net table.
 *
 *                                          Alan Meyer  July, 2000
 *
 * $Id: CdrLink.cpp,v 1.5 2001-04-17 23:11:24 ameyer Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.4  2000/12/13 01:50:59  ameyer
 * Implemented delLinks, added changes for docs deleted by marking them
 * deleted.
 *
 * Revision 1.3  2000/09/27 20:25:16  bkline
 * Fixed last argument to findTargetDocTypes().
 *
 * Revision 1.2  2000/09/27 11:28:44  bkline
 * Added CdrDelLinks() and findTargetDocTypes().
 *
 * Revision 1.1  2000/09/26 19:04:26  ameyer
 * Initial revision
 *
 *
 */

// Eliminate annoying warnings about truncated debugging information.
#pragma warning(disable : 4786)

#include "CdrSession.h"
#include "CdrDom.h"
#include "CdrString.h"
#include "CdrException.h"
#include "CdrDbConnection.h"
#include "CdrCommand.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbStatement.h"
#include "CdrDbResultSet.h"
#include "CdrValidateDoc.h"
#include "CdrLink.h"

// Prototypes for internal functions
static int linkTree (cdr::db::Connection&, cdr::dom::Node&,
                     int, int, cdr::String&, cdr::link::LnkList&,
                     cdr::StringSet&, cdr::StringSet&, cdr::StringList&);

static int checkMissedFrags (cdr::db::Connection&, int,
                     cdr::ValidRule, cdr::StringSet&,
                     cdr::StringList&);

static void updateFragList (cdr::db::Connection&, int, cdr::StringSet&);

static bool findDocType (cdr::db::Connection&, int, int *, cdr::String *);


/**
 * Constructor for a CdrLink object
 * See CdrLink.h for documentation
 */

cdr::link::CdrLink::CdrLink (
    cdr::db::Connection& conn,
    cdr::dom::Node&      elemDom,
    int                  docId,
    int                  docType,
    cdr::String          docTypeStr,
    cdr::String          refValue,
    LinkStyle            lnkStyle
) : fldNode (elemDom), srcId (docId), srcDocType (docType),
    srcDocTypeStr (docTypeStr), ref (refValue), style (lnkStyle)
{
    std::string qry;        // For database SQL string


    // Default values for uninitialized object
    type          = 0;
    typeStr       = L"";
    srcField      = L"";
    trgId         = 0;
    trgIdStr      = L"";
    trgDocType    = 0;
    trgDocTypeStr = L"";
    trgActiveStat = L"";
    trgFrag       = L"";
    hasData       = false;
    errCount      = 0;
    saveLink      = true;


    // Sanity check
    if (ref.size() == 0)
        throw cdr::Exception (L"Empty or missing ref value - Can't happen!");

    // Get info from the dom concerning this link
    const cdr::dom::Element& element
             = static_cast<const cdr::dom::Element&> (fldNode);
    srcField = fldNode.getNodeName ();

    // Is this a reference to a CDR document?
    if (style == cdr::link::ref || style == cdr::link::href) {

        // Get the fragment portion of the reference, if any
        wchar_t *fragptr = wcschr (ref.c_str(), '#');
        if (fragptr) {

            // Isolate base reference and fragment
            trgFrag  = cdr::String (fragptr + 1);
            trgIdStr = cdr::String (ref.c_str(), (fragptr - ref.c_str()));

            // If no base reference found, base is the current doc
            // I.e., reference is to an id inside this document
            if (trgIdStr.size() == 0)
                trgIdStr = cdr::stringDocId (srcId);
        }
        else {
            trgFrag  = L"";
            trgIdStr = ref;
        }

        // Get the id portion of the reference
        // This also tests for valid 'CDRnnn...' format
        try {
            trgId = trgIdStr.extractDocId ();
        }
        catch (const cdr::Exception& e) {
            // Probably an invalid format
            this->addLinkErr (e.what());
        }

        // If this is a self-link, we know things about the target
        if (trgId == srcId) {
            trgDocType    = srcDocType;
            trgDocTypeStr = srcDocTypeStr;
            trgActiveStat = L"A";
        }
    }

    // Find the link type expected for this doctype/fieldtag
    qry = "SELECT link_id FROM link_xml WHERE doc_type = ? AND element = ?";
    cdr::db::PreparedStatement select = conn.prepareStatement (qry);
    select.setInt (1, srcDocType);
    select.setString (2, srcField);
    cdr::db::ResultSet rs = select.executeQuery();

    if (!rs.next()) {
        // Error will be caught again and reported later
        this->saveLink = false;
    }
    else
        type = rs.getInt (1);

} // Constructor


/**
 * Validate one free standing link
 * See CdrLink.h for documentation
 */

int cdr::link::CdrLink::validateLink (
    cdr::db::Connection& dbConn,
    cdr::StringSet&      fragSet
) {
    std::string qry;        // For database SQL string


    // None of these checks can be made without a target id
    if (trgId != 0) {

        // Tests are done differently for links to self and links to other
        if (trgId != srcId) {

            // Find out if target exists and get its deletion status and type
            qry = "SELECT doc_type, active_status FROM document WHERE id = ?";
            cdr::db::PreparedStatement doc_sel = dbConn.prepareStatement (qry);
            doc_sel.setInt (1, trgId);
            cdr::db::ResultSet doc_rs = doc_sel.executeQuery();
            if (!doc_rs.next()) {
                this->addLinkErr (L"Target document not found in CDR");
            }
            else {
                trgDocType    = doc_rs.getInt (1);
                trgActiveStat = doc_rs.getString (2);
                if (trgActiveStat == L"D")
                    this->addLinkErr (L"Target link is to deleted document");
            }

            // If we found the target, can check the fragment
            // If a fragment is specified, it must exist in the target doc
            if (trgDocType != 0 && trgFrag.size() > 0) {

                qry = "SELECT fragment FROM link_fragment "
                      "WHERE doc_id = ? AND fragment = ?";
                cdr::db::PreparedStatement frag_sel =
                            dbConn.prepareStatement (qry);
                frag_sel.setInt (1, trgId);
                frag_sel.setString (2, trgFrag);
                cdr::db::ResultSet frag_rs = frag_sel.executeQuery();

                // Error if it doesn't
                if (!frag_rs.next())
                    this->addLinkErr (L"cdr:ref matching fragment '" +
                            trgFrag + L"' not found in target document");
            }
        }

        else {
            // Target doc is same as source doc
            // If this is a link to a fragment, check to be sure it's here
            if (trgFrag.size() > 0) {
                if (fragSet.find (trgFrag) == fragSet.end())
                    this->addLinkErr (L"cdr:ref matching fragment '" +
                            trgFrag + L"' not found in this document");
            }
        }

        // Validate target doctype for this link
        qry = "SELECT target_doc_type FROM link_target "
              "WHERE  source_link_type = ?";
        cdr::db::PreparedStatement targ_sel = dbConn.prepareStatement (qry);
        targ_sel.setInt (1, type);
        cdr::db::ResultSet targ_rs = targ_sel.executeQuery();

        // Need to find a target doc type for this link
        // And need to make sure at least one of them matches
        bool found_target_doc_type = false;
        bool match_target_doc_type = false;
        while (targ_rs.next()) {
            found_target_doc_type = true;
            if (trgDocType == targ_rs.getInt (1))
                match_target_doc_type = true;
        }

        // Nothing in the table is a system error
        // But we'll deal with it as an ordinary error for now
        if (!found_target_doc_type)
            this->addLinkErr (L"Field may not contain links");

        // Wrong target type is a link error
        else if (!match_target_doc_type)
            this->addLinkErr (L"Link from " + srcDocTypeStr + L"." + srcField +
                              L" to document type " + trgDocTypeStr +
                              L" is illegal");

        // Execute any custom link procedures (in CdrLinkProcs.cpp)
        customLinkCheck (dbConn, this);
    }

    return errCount;

} // validateLink


/**
 * Delete all links from a document.
 *
 * See CdrLink.h
 */
int cdr::link::CdrDelLinks (
    cdr::db::Connection& dbConn,
    int                  docId,
    cdr::ValidRule       vRule,
    cdr::StringList&     errList
) {

    // Find out if any other documents link to this one
    // Get the titles and fragment identifiers too for reporting purposes
    std::string breakQry =
            "SELECT DISTINCT source_doc, title, target_frag "
            "  FROM link_net, document "
            " WHERE target_doc = ? "
            "   AND source_doc = id";

    cdr::db::PreparedStatement breakSel = dbConn.prepareStatement (breakQry);
    breakSel.setInt (1, docId);
    cdr::db::ResultSet breakRs = breakSel.executeQuery();

    while (breakRs.next()) {
        // Retrieve info about link
        cdr::String idStr = cdr::String::toString (breakRs.getInt (1));
        cdr::String title = breakRs.getString (2);
        cdr::String frag  = breakRs.getString (3);

        // Construct informative error message
        cdr::String errMsg = L"Document " + idStr + L": (" + title +
              L") links to this document";
        if (frag.size() > 0)
            errMsg += L" Fragment(" + frag + L")";

        errList.push_back (errMsg);
    }

    // If only validating, or if only updating if valid, then
    // Stop here if there were links to this doc
    if (vRule == ValidateOnly || (vRule == UpdateIfValid && !errList.empty()))
        return errList.size();

    // Delete all links from this document to other documents
    std::string delLinkQry = "DELETE FROM link_net WHERE source_doc = ?";
    cdr::db::PreparedStatement delLinkSel =
                dbConn.prepareStatement (delLinkQry);
    delLinkSel.setInt (1, docId);
    cdr::db::ResultSet delLinkRs = delLinkSel.executeQuery();

    // Delete all fragment table entries for this document
    std::string delFragQry = "DELETE FROM link_fragment WHERE doc_id = ?";
    cdr::db::PreparedStatement delFragSel =
                dbConn.prepareStatement (delFragQry);
    delFragSel.setInt (1, docId);
    cdr::db::ResultSet delFragRs = delFragSel.executeQuery();

    return errList.size();
}


/**
 * getLinks
 * Retrieve information from the link tables.
 * Simple version only gets links to a document.
 *
 * See CdrLink.h
 */

cdr::String getLinks (
    cdr::Session&         session,
    const cdr::dom::Node& commandNode,
    cdr::db::Connection&  dbConnection
) {
    int         docId;      // Doc id from transaction
    cdr::String docIdStr,   // String form "CDR00..."
                name;       // Element name


    // Get document id from command
    cdr::dom::Node node = commandNode.getFirstChild();
    while (node != 0) {

        if (node.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            name = node.getNodeName();
            if (name != L"DocId")
                throw cdr::Exception (L"getLinks: Element '" + name
                                    + L"' in CdrGetLinks request is "
                                      L"currently unsupported");

            docIdStr = cdr::dom::getTextContent (node);
            docId    = docIdStr.extractDocId();
        }

        node = node.getNextSibling();
    }

    // In this simple getLinks we're just retrieving links that
    //   point to the current document
    // CdrDelLinks can already do that.
    cdr::StringList errList;
    int errCount = cdr::link::CdrDelLinks (dbConnection, docId,
                                           cdr::ValidateOnly, errList);

    // Pack it up the transaction and links
    cdr::String resp  = L"<CdrGetLinksRes>\n"
                        L"  <DocID>" + docIdStr + L"</DocID>\n"
                        L"  <LnkList>\n"
                        L"    <LnkCount>";
                resp += errCount + L"</LnkCount>\n";

    if (errCount) {
        cdr::StringList::const_iterator i = errList.begin();
        while (i != errList.end())
            resp += L"    " + cdr::tagWrap (*i++, L"LnkItem");
    }

    // Terminate and return it
    resp += L"  </LnkList>\n"
            L"</CdrGetLinksRes>\n";

    return resp;
}


/**
 * Dump link info in XML format
 * See CdrLink.h for documentation
 */

cdr::String cdr::link::CdrLink::dumpXML (
    cdr::db::Connection& dbConn
) {
    cdr::String xml;                    // String to return
    std::string qry;                    // SQL query for database access


    // Top level tag for XML
    xml = L"<LnkItem>";

    // Get link type name from database
    if (typeStr.size() == 0) {
        qry = "SELECT name FROM link_type WHERE id = ?";
        cdr::db::PreparedStatement stmt = dbConn.prepareStatement (qry);
        stmt.setInt (1, type);
        cdr::db::ResultSet rs = stmt.executeQuery();
        if (!rs.next())
            throw cdr::Exception (
                      L"CdrLink::dumpXML: Bad link type - Can't happen");
        typeStr = rs.getString (1);
    }
    xml += cdr::tagWrap (typeStr, L"LnkType");

    // Source document id
    xml += cdr::tagWrap (cdr::stringDocId (srcId), L"LnkSrcId");

    // Source document type
    xml += cdr::tagWrap (srcDocTypeStr, L"LnkSrcDocType");

    // Element tag in which link was found
    xml += cdr::tagWrap (srcField, L"LnkSrcField");

    // Target document id
    xml += cdr::tagWrap (trgIdStr, L"LnkTrgId");

    // Target document type, if known, may require lookup
    if (trgDocType > 0) {
        if (trgDocTypeStr.size() == 0) {
            qry = "SELECT name FROM doc_type WHERE id = ?";
            cdr::db::PreparedStatement stmt = dbConn.prepareStatement (qry);
            stmt.setInt (1, trgDocType);
            cdr::db::ResultSet rs = stmt.executeQuery();
            if (!rs.next())
                throw cdr::Exception (
                      L"CdrLink::dumpXML: Bad document type - Can't happen");
            trgDocTypeStr = rs.getString (1);
        }
        xml += cdr::tagWrap (trgDocTypeStr, L"LnkTrgDocType");
    }

    // Fragment identifier, if any
    if (trgFrag.size() > 0)
        xml += cdr::tagWrap (trgFrag, L"LnkTrgFrag");

    // Reference string
    if (ref.size() > 0)
        xml += cdr::tagWrap (ref, L"LnkHref");

    // List of errors
    if (errCount) {
        cdr::StringList::const_iterator i = errList.begin();
        while (i != errList.end())
            xml += tagWrap (*i++, L"LinkErr");
    }

    // Terminate item
    xml += L"</LnkItem>";

    return xml;

} // dumpXML


/**
 * Convert link info to string
 *
 * Produces strings like:
 *  "Link:Vocabulary Protocol/Diagnosis:1234=Terminology:2351"
 * See CdrLink.h for documentation
 */

cdr::String cdr::link::CdrLink::dumpString (
    cdr::db::Connection& dbConn
) {
    cdr::String str;                    // String to return
    std::string qry;                    // SQL query for database access


    // Get link type name from database, if not known
    if (type > 0) {
        if (typeStr.size() == 0) {
            qry = "SELECT name FROM link_type WHERE id = ?";
            cdr::db::PreparedStatement stmt = dbConn.prepareStatement (qry);
            stmt.setInt (1, type);
            cdr::db::ResultSet rs = stmt.executeQuery();
            if (!rs.next())
                throw cdr::Exception (
                      L"CdrLink::dumpString: Bad link type - Can't happen");
            typeStr = rs.getString (1);
        }
    }

    // Get target doc type from database, if not known
    if (trgDocType > 0) {
        if (trgDocTypeStr.size() == 0) {
            qry = "SELECT name FROM doc_type WHERE id = ?";
            cdr::db::PreparedStatement stmt = dbConn.prepareStatement (qry);
            stmt.setInt (1, trgDocType);
            cdr::db::ResultSet rs = stmt.executeQuery();
            if (!rs.next())
                throw cdr::Exception (
                      L"CdrLink::dumpString: Bad document type - Can't happen");
            trgDocTypeStr = rs.getString (1);
        }
    }

    // Doctype/Field=
    str = srcDocTypeStr + L"/" + srcField + L"=";

    // targettype:Id#Fragment
    if (trgDocType > 0) {
        str += trgDocTypeStr + L":" + cdr::String::toString (trgId);
        if (trgFrag.size() > 0)
            str += L"#" + trgFrag;
    }

    // Or hyperlink reference if link is to outside CDR
    else
        str += ref;

    // List of errors
    if (errCount) {
        cdr::StringList::const_iterator i = errList.begin();
        while (i != errList.end())
            str += L"//ERROR: " + *i++;
    }

    return str;

} // dumpString


/**
 * Add an error message to a link object, incrementing the count
 */

void cdr::link::CdrLink::addLinkErr (
    cdr::String msg
) {
    errList.push_back (msg);
    ++errCount;
}


/**
 * Validate links in a document and, optionally, update the link_net
 * database by calling updateLinkNet().
 *
 * See CdrLink.h
 */

int cdr::link::CdrSetLinks (
    cdr::dom::Node&         xmlNode,
    cdr::db::Connection&    dbConn,
    int                     ui,
    cdr::String             docTypeStr,
    cdr::ValidRule          validRule,
    cdr::StringList&        errList
) {
    cdr::link::LnkList lnkList;     // List of all link objects from this doc
    cdr::StringSet uniqSet;         // For finding duplicate link info
    cdr::StringSet fragSet;         // Set of all ref fragments found
    int docType,                    // Internal id of docType
        err_count;                  // Number of errors found


    // Get internal form of doctype
    std::string qry = "SELECT id FROM doc_type WHERE name = ?";
    cdr::db::PreparedStatement select = dbConn.prepareStatement (qry);
    select.setString (1, docTypeStr);
    cdr::db::ResultSet rs = select.executeQuery();

    if (!rs.next())
        throw cdr::Exception (L"CdrSetLinks: Unknown doctype");
    docType = rs.getInt (1);

    // Build list of links found in this doc
    err_count = linkTree (dbConn, xmlNode, ui, docType, docTypeStr,
                          lnkList, uniqSet, fragSet, errList);

    // Validate all links, appending to an error message return string
    cdr::link::LnkList::iterator i = lnkList.begin();
    err_count = 0;
    while (i != lnkList.end()) {
        err_count += i->validateLink (dbConn, fragSet);
        if (i->getErrCount() > 0)
            errList.push_back (i->dumpString (dbConn));
        ++i;
    }

    // Check for missing fragments
    // This is not part of validateLink because the errors are not
    //   found in a link, but rather in the absence of a fragment
    err_count += checkMissedFrags (dbConn, ui, validRule, fragSet, errList);

    // Update the link net if required
    if (validRule == cdr::UpdateUnconditionally ||
            (validRule == cdr::UpdateIfValid && err_count == 0)) {

        // All together now
        bool oldCommitted = dbConn.getAutoCommit();
        dbConn.setAutoCommit (false);

        // Update link net and fragment table
        cdr::link::updateLinkNet (dbConn, ui, lnkList);
        updateFragList (dbConn, ui, fragSet);

        dbConn.setAutoCommit (oldCommitted);
    }

    // XXXX If we are deleting text from nodes, we might do part
    //      or all of it here.  Might delete the nodes in validateLink()
    //      then reconstruct the XML string here
    //      But we only do it if we are actually updating something.
    //      If just validating a document on which editing may continue,
    //      we don't want to change anything under the eyes of the human
    //      editor.
    // XXXX Later!

    return err_count;

} // CdrSetLinks


/**
 * Traverse the tree of nodes, constructing a CdrLink object for each
 * node which contains a link attribute.
 *
 *  @param      conn        Reference to database connection.
 *  @param      node        Reference to the top node of the DOM parse
 *                           tree for the XML document.
 *  @param      docId       CDR document ID for XML document containing node.
 *  @param      docType     Document type for containing document.
 *  @param      docTypeStr  String name of document type.  As an
 *                           optimization, we retrieve this only once and
 *                           pass it in to linkTree.
 *  @param      lnkList     Reference to std::list of link objects.  We append
 *                           a new one each time we find a linking attribute
 *                           in a node.
 *  @param      uniqSet     Reference to a set of strings containing
 *                           source field + url, so we can avoid creating
 *                           useless duplicate link records.
 *  @param      fragSet     Reference to a list of cdr:id attribute
 *                           values which can be used as targets of links.
 *  @return                 Number of objects in list.
 */

static int linkTree (
    cdr::db::Connection&  conn,
    cdr::dom::Node&       node,
    int                   docId,
    int                   docType,
    cdr::String&          docTypeStr,
    cdr::link::LnkList&   lnkList,
    cdr::StringSet&       uniqSet,
    cdr::StringSet&       fragSet,
    cdr::StringList&      errList
) {
    int  link_count = 0;            // Number of links found
    cdr::String frag,               // Fragment identifier in node
                uniq,               // Enough info to find duplicates
                refAttr,            // Link attribute value
                refValue;           // Copy refAttr to here for one we want
    cdr::link::LinkStyle lnkStyle;  // ref, xref, etc.
    bool        multiLinkErr;       // True=Two or more links in one element


    // Return value from set insertion
    std::pair<cdr::StringSet::iterator, bool> ins_stat;


    // Process this node and it's siblings, while there are any
    while (node != 0) {

        // Only concerned with element nodes
        if (node.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {

            const cdr::dom::Element& element
                     = static_cast<const cdr::dom::Element&> (node);

            // Don't know what kind of link we have yet or how many
            lnkStyle     = cdr::link::not_set;
            multiLinkErr = false;

            // Does this node contain a link
            // Check for all three types
            // But only one is allowed
            refValue = element.getAttribute (L"cdr:ref");
            if (refValue.size() > 0)
                lnkStyle = cdr::link::ref;

            refAttr = element.getAttribute (L"cdr:href");
            if (refAttr.size() > 0) {
                if (lnkStyle == cdr::link::not_set) {
                    lnkStyle = cdr::link::href;
                    refValue = refAttr;
                }
                else
                    multiLinkErr = true;
            }

            refAttr = element.getAttribute (L"cdr:xref");
            if (refAttr.size() > 0) {
                if (lnkStyle == cdr::link::not_set) {
                    lnkStyle = cdr::link::xhref;
                    refValue = refAttr;
                }
                else
                    multiLinkErr = true;
            }

            // If we found any of the three types of link
            if (lnkStyle != cdr::link::not_set) {

                // Create a link object for this node
                cdr::link::CdrLink lnk (conn, node, docId, docType,
                                        docTypeStr, refValue, lnkStyle);

                // Is it a duplicate of a link object we've already seen?
                // Can happen if two occurrences of a field both link
                //   to the same object.
                // This might be legitimate, so we don't forbid it, but
                //   neither do we store duplicate link records or, if
                //   they are invalid, report duplicate errors.
                // Two links are duplicates if they come from the same
                //   field tag and point to the same url.
                uniq     = lnk.getSrcField() + L"+" + lnk.getRef();
                ins_stat = uniqSet.insert (uniq);

                // If it duplicates another link, we don't store it
                if (!ins_stat.second)
                    lnk.setSaveLink (false);

                // If there was more than one link attribute in the element
                // record the error
                if (multiLinkErr)
                    lnk.addLinkErr (
                         L"Can only have one link from a single element");

                // Add it to the list of links
                lnkList.push_back (lnk);
                ++link_count;
            }

            // Does the node have an id attribute which can be the
            //   target of an external link?
            frag = element.getAttribute (L"cdr:id");
            if (frag.size() > 0) {

                // Attempt insertion
                ins_stat = fragSet.insert (frag);

                // If it was already there, declare an error
                if (!ins_stat.second)
                    errList.push_back (L"cdr:ref \"" + frag +
                                       L"\" used more than once");
            }

            // Does the node have children?
            cdr::dom::Node child = node.getFirstChild();
            if (child != 0) {
                // Process subtree
                link_count += linkTree (conn, child, docId, docType,
                                        docTypeStr, lnkList, uniqSet,
                                        fragSet, errList);
            }
        }

        // Are there siblings?
        node = node.getNextSibling();
    }

    return link_count;

} // linkTree


/**
 * Update the link_net table in the database.
 *
 *  @param      conn        Reference to database connection.
 *  @param      docId       CDR document ID for XML document containing node.
 *  @param      lnkList     Reference to std::list of link objects.  We append
 *                           a new one each time we find a linking attribute
 *                           in a node.
 *  @return                 Void.
 */

void cdr::link::updateLinkNet (
    cdr::db::Connection& conn,
    int                  docId,
    cdr::link::LnkList&  lnkList
) {
    std::string qry;                    // SQL query for database access


    // This is a brute force version
    // We may be able to optimize this if required by deleting only those
    //   entries which are no longer accurate and adding only those which
    //   are new.
    // That might provide a significant performance improvement for edited
    //   records with large numbers of links.

    // Delete all existing records linking out from the current document
    qry = "DELETE FROM link_net WHERE source_doc = ?";
    cdr::db::PreparedStatement stmt = conn.prepareStatement (qry);
    stmt.setInt (1, docId);
    cdr::db::ResultSet rs = stmt.executeQuery();

    // Add the new ones, one at a time
    cdr::link::LnkList::iterator i = lnkList.begin();
    int err_count = 0;
    while (i != lnkList.end()) {

        // If this is a link to be saved (e.g., not a duplicate)
        if (i->getSaveLink()) {
            qry = "INSERT INTO link_net (link_type, source_doc, source_elem, "
                  "  target_doc, target_frag, url) "
                  "VALUES (?, ?, ?, ?, ?, ?)";

            cdr::db::PreparedStatement stmt = conn.prepareStatement (qry);
            stmt.setInt    (1, i->getType());
            stmt.setInt    (2, i->getSrcId());
            stmt.setString (3, i->getSrcField());
            stmt.setInt    (4, i->getTrgId());
            stmt.setString (5, i->getTrgFrag());
            stmt.setString (6, i->getRef());
            cdr::db::ResultSet rs = stmt.executeQuery();
        }

        // If it's a link for which any content is just denormalized data
        //   get rid of the data here
        // Requires that the XML string be rebuilt later
        // XXXX   MAY REQUIRE SETTING A FILEWIDE STATIC TO TELL
        //        CdrSetLinks TO REBUILD THE STRING.
        if (i->getStyle() == cdr::link::ref) {
            // XXXX - TO BE DONE
            ;
        }

        // Next link
        ++i;
    }
} // updateLinkNet


/**
 * Check whether there are any links to fragments that don't exist
 *
 *  @param      conn        Reference to database connection.
 *  @param      docId       CDR document ID for XML document containing node.
 *  @param      validRule   Relationship between validation and update.
 *                           If updating, we mark any links/docs as invalid
 *                           if they point to a non-existent fragment.
 *  @param      fragSet     Reference to cdr::StringSet of fragment texts.
 *                           These are identifiers stored in cdr:ref
 *                           attributes in a document.
 *  @return                 Void.
 */

static int checkMissedFrags (
    cdr::db::Connection& conn,
    int                  docId,
    cdr::ValidRule       validRule,
    cdr::StringSet&      fragSet,
    cdr::StringList&     errList
) {
    std::string qry;            // SQL query for database access
    cdr::String frag,           // Fragment id of link to our doc
                lastFrag;       // Last one seen before current one
    int         srcId,          // ID of source doc for fraglink
                frag_errs,      // Count of errors for one missing fragment
                total_errs;     // Count of all errors


    // Find all docs that link to any fragment in this doc
    qry = "SELECT target_frag, source_doc "
          "FROM   link_net "
          "WHERE  (target_doc = ? AND target_frag IS NOT NULL) "
          "ORDER BY target_frag";

    cdr::db::PreparedStatement stmt = conn.prepareStatement (qry);
    stmt.setInt (1, docId);
    cdr::db::ResultSet rs = stmt.executeQuery();

    // Start evaluating from scratch
    lastFrag   = L"";
    frag_errs  = 0;
    total_errs = 0;

    // Check each link fragment to this doc to be sure the frag exists
    while (rs.next()) {

        // Get data from the link
        rs.getString (1);
        rs.getInt    (2);

        // Is it pointing to a fragment actually in the current doc?
        if (fragSet.find (frag) == fragSet.end()) {

            // Not found, we have an error
            if (frag != lastFrag) {

                // Were there unreported errors?
                if (frag_errs > cdr::link::MaxFragMissErrors)
                    errList.push_back (cdr::String::toString (frag_errs) +
                        L" documents link to missing cdr:ref '" +
                        frag + L"'.  Stopped listing them after " +
                        cdr::String::toString (cdr::link::MaxFragMissErrors) +
                        L" errors reported");

                // Ready for next
                lastFrag  = frag;
                frag_errs = 0;
            }

            // Report this particular error
            // But don't report the same error more than some max
            //   number of times
            if (frag_errs++ < cdr::link::MaxFragMissErrors)
                errList.push_back (L"Document " +
                        cdr::String::toString (srcId) +
                        L" expects cdr:ref='" + frag +
                        L"' but no such ref found");

#if 0
THIS ISN'T A GOOD IDEA BECAUSE A USER CAN FIX THE SOURCE RECORD, LEAVING
THE TARGET MARKED BAD WHEN IT'S NOT.
THE ONLY RECORD MARKED SHOULD BE THE ONE WE'RE CURRENTLY VALIDATING.
            // Caller may be updating the database even if there are
            //   errors.  If so, must mark the remote source of the
            //   link as in error.
            if (validRule == cdr::UpdateUnconditionally) {
                qry = "UPDATE document "
                      "SET    val_status = ?, val_date GETDATE() "
                      "WHERE  id = ?";

                cdr::db::PreparedStatement stmt = conn.prepareStatement (qry);
                stmt.setInt (1, cdr::InvalidStatus);
                stmt.setInt (2, docId);
                stmt.executeQuery();
            }
#endif

            ++total_errs;
        }
    }

    return total_errs;

} // checkMissedFrags()


/**
 * Update link_fragment table in the database.
 *
 *  @param      conn        Reference to database connection.
 *  @param      docId       CDR document ID for XML document containing node.
 *  @param      fragSet     Reference to cdr::StringSet of fragment texts.
 *                           These are identifiers stored in cdr:ref
 *                           attributes in a document.
 *  @return                 Void.
 */

static void updateFragList (
    cdr::db::Connection& conn,
    int                  docId,
    cdr::StringSet&      fragSet
) {
    std::string qry;                    // SQL query for database access


    // Another brute force version, probably justified in this case
    //   because the average number of fragment ids for a record
    //   is expected to be quite small.
    qry = "DELETE FROM link_fragment WHERE doc_id = ?";
    cdr::db::PreparedStatement stmt = conn.prepareStatement (qry);
    stmt.setInt (1, docId);
    cdr::db::ResultSet rs = stmt.executeQuery();

    // Add each one
    cdr::StringSet::const_iterator i = fragSet.begin();
    while (i != fragSet.end()) {
        qry = "INSERT doc_id, fragment INTO link_fragment VALUES (?, ?)";
        cdr::db::PreparedStatement stmt = conn.prepareStatement (qry);
        stmt.setInt    (1, docId);
        stmt.setString (2, *i);
        cdr::db::ResultSet rs = stmt.executeQuery();
        ++i;
    }
} // updateFragList()


void cdr::link::findTargetDocTypes(
        cdr::db::Connection&    conn,
        const cdr::String&      srcElem,
        const cdr::String&      srcDocType,
        std::vector<int>&       typeList)
{
    std::string qry = "SELECT lt.target_doc_type"
                      "  FROM link_target lt,"
                      "       link_xml lx,"
                      "       doc_type dt"
                      " WHERE lx.doc_type = dt.id"
                      "   AND lx.element = ?"
                      "   AND dt.name = ?"
                      "   AND lt.source_link_type = lx.link_id";
    typeList.clear();
    cdr::db::PreparedStatement stmt = conn.prepareStatement(qry);
    stmt.setString(1, srcElem);
    stmt.setString(2, srcDocType);
    cdr::db::ResultSet rs = stmt.executeQuery();
    while (rs.next())
        typeList.push_back(rs.getInt(1));
}


/**
 * findDocType
 *
 * Find the document type id and name corresponding to a document id.
 *
 *  @param  dbConn      Database connection.
 *  @param  docId       Find doc type info for this document.
 *  @param  typeIdp     Pointer to int to receive type id.
 *  @param  typeStrp    Pointer to string receive type name.
 *
 *  @return true        Success, false=document or type not found.
 */

static bool findDocType (
    cdr::db::Connection& dbConn,
    int                  docId,
    int                  *typeIdp,
    cdr::String          *typeStrp
) {
    std::string qry = "SELECT t.id, t.name FROM doc_type t JOIN document d "
                      "    ON t.id = d.doc_type "
                      " WHERE d.id = ?";
    cdr::db::PreparedStatement stmt = dbConn.prepareStatement (qry);
    stmt.setInt (1, docId);
    cdr::db::ResultSet rs = stmt.executeQuery();
    if (!rs.next())
        return false;

    // Get info
    *typeIdp  = rs.getInt (1);
    *typeStrp = rs.getString (2);

    return true;
}
