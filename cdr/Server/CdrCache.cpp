/*
 * $Id: CdrCache.cpp,v 1.2 2004-05-25 22:39:18 ameyer Exp $
 *
 * Specialized cacheing for performance optimization, where useful.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2004/05/14 02:04:34  ameyer
 * CdrCache serverside cacheing to speed publishing.
 *
 */

#include <iostream> // For debug

#include "CdrCommand.h"
#include "CdrVersion.h"
#include "CdrLock.h"
#include "CdrCache.h"

static bool getCacheTypes(cdr::String, bool *);

//--------------------------------------------------
// Terminology Cacheing
//--------------------------------------------------

// Map of Term document IDs to stored XML versions of the
//   denormalized term plus upcoded parents.
// When we're cacheing for the whole system, we use a singleton
//   class static map of id -> Term
// A new map is constructed and and populated
//   as Terms are retrieved.
// If no system-wide cacheing, the map pointer is null
//   - which indicates to the application that
//   system wide cacheing is not in effect.
static cdr::cache::TERM_MAP *S_pTermMap = 0;

// Name of the mutex preventing two separate threads from reading
//   or updating the system-wide static S_pTermMap
static const char* const termCacheMutexName = "TermCacheMutex";

// Turn system-wide cacheing on or off.  Default start state is off
bool cdr::cache::Term::initTermCache(
    bool state
) {
    // Remember what it is before change
    bool prevState = S_pTermMap ? true : false;

    // Whether stopping or starting, if it was already started,
    //   clear cache so we can reload terms from the database.
    if (prevState)
        cdr::cache::Term::clearMap(S_pTermMap);

    if (state)
        // Create a new map of term doc ids to Term objects
        S_pTermMap = new cdr::cache::TERM_MAP;

    return prevState;
}

// Retrieve a utf-8 format string containing the denormalization
//  for a Term document and all its parent Term documents ...
std::string cdr::cache::Term::denormalizeTermId(
    const cdr::String&   docIdStr,
    cdr::db::Connection& conn
) {
    // Pointer to static system-wide map, or to local map on stack,
    //   depending on whether global caching is or is not enabled
    TERM_MAP *pMap;

    HANDLE mutex     = 0;       // Synchronize access to system-wide TERM_MAP
    bool   haveMutex = false;   // True = must release mutex

    // String to return to caller
    std::string termsXml = "";

    // If static map, try to acquire it
    if (S_pTermMap) {

        HANDLE mutex = CreateMutex(0, false, termCacheMutexName);
        if (mutex != 0) {
            cdr::Lock lock(mutex, 3000);
            if (lock.m) {
                // Point to system wide cache map
                pMap      = S_pTermMap;
                haveMutex = true;
            }
        }
    }

    // If no system-wide cacheing, or could not acquire mutex,
    //   use a local ID -> Term map just holding info for this one
    //   transaction
    if (!haveMutex)
        pMap = new TERM_MAP;

    // Denormalize the term, or find an existing denormalization
    Term *t = getTerm (conn, pMap, 0, docIdStr);

    // If it was a legitimate term, successfully denormalized
    if (t)
        termsXml = t->getFamilyXml();

    // Release resources
    if (haveMutex)
        // Allow others to use the system-wide map
        CloseHandle(mutex);
    else
        // Or delete the local one
        clearMap (pMap);

    // Return XML or empty string
    return termsXml;
}

// Get a Term object corresponding to an ID.
cdr::cache::Term * cdr::cache::Term::getTerm(
    cdr::db::Connection& conn,
    cdr::cache::TERM_MAP *pMap,
    const int            depth,
    const cdr::String    &docIdStr
) {
    // Some string constants
    static const std::string s_SPACE = " ";
    static const std::string s_EQUOT = "=\"";
    static const std::string s_QUOTE = "\"";

    // Check for possible infinite recursion
    if (depth > MAX_HIERARCHY)
        cdr::cache::fatal (L"Exceeded maximum getTerm depth, id=" + docIdStr);

    // Convert id
    int docId = docIdStr.extractDocId();

    // Have we already seen this Term?
    if (pMap->count(docId) == 1)

        // No more to do.  This is where our really big savings occurs!
        return (*pMap)[docId];

    // Term object is null until proven otherwise
    cdr::cache::Term *pTerm = NULL;

    // Find last publishable version of the XML for this term
    int ver = cdr::getLatestPublishableVersion(docId, conn);
    if (ver < 0)
        // Should be one, if not, don't publish this term
        cdr::cache::error (L"No publishable version for termId=" + docIdStr);

    else {
        // Create a new Term object to load from the database
        pTerm = new Term (docId);

        // Get the XML itself
        cdr::CdrVerDoc docVer;
        if (!cdr::getVersion(docId, conn, ver, &docVer)) {
            // Serious error - can't happen if database okay
            wchar_t verStr[50];
            swprintf (verStr, L"%d", ver);
            cdr::cache::fatal (L"Error getting version=" + cdr::String(verStr)
                               + L" for Term doc=" + docIdStr);
        }

        // Look for elements we need
        // SAX would be more efficient, but we aren't using it
        //   anywhere else and don't have our own wrappers for it
        cdr::dom::Parser parser;
        cdr::dom::Node   node;
        cdr::dom::Node   childNode;
        cdr::dom::Node   gChildNode;
        cdr::String      nodeName;
        cdr::String      parentIdStr;
        cdr::cache::Term *pParentTerm;

        // Parse term doc and get first element
        parser.parse (docVer.xml);
        node = parser.getDocument().getDocumentElement().getFirstChild();

        // Search immediate children of top level element
        while (node != 0) {
          if (node.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            nodeName = node.getNodeName();

            // Preferred name of the Term
            if (nodeName == L"PreferredName")
              pTerm->name =
                 cdr::trimWhiteSpace(cdr::dom::getTextContent(node)).toUtf8();

            // Ensure this term isn't a type we don't look at
            else if (nodeName == L"TermType") {
              childNode = node.getFirstChild();
              while (childNode != 0) {
                if (childNode.getNodeType() == cdr::dom::Node::ELEMENT_NODE &&
                    cdr::String(childNode.getNodeName()) == L"TermTypeName") {

                  // These term types are not used, but we'll keep looking
                  //   for their parents
                  cdr::String typeName = cdr::dom::getTextContent (childNode);
                  if (typeName == L"Header term" ||
                      typeName == L"Obsolete term") {
                    pTerm->typeOK = false;
                    break;
                  }
                }
                childNode = childNode.getNextSibling();
              }
            }

            // Look in TermRelationship for ParentTerm's ID
            // ASSUMPTION: TermType is found before TermRelationship
            else if (nodeName == L"TermRelationship") {
              childNode = node.getFirstChild();
              while (childNode != 0) {

                // Looking for ParentTerm in TermRelationship
                if (childNode.getNodeType() == cdr::dom::Node::ELEMENT_NODE &&
                    cdr::String(childNode.getNodeName()) == L"ParentTerm") {

                  // Got a parent term, find its ID
                  gChildNode = childNode.getFirstChild();
                  while (gChildNode != 0) {
                    if (gChildNode.getNodeType() ==
                            cdr::dom::Node::ELEMENT_NODE &&
                        cdr::String(gChildNode.getNodeName()) == L"TermId") {

                      // Found ID, check and save it
                      parentIdStr =
                          (static_cast<cdr::dom::Element&>(gChildNode)).
                                    getAttribute("cdr:ref");
                      if (parentIdStr.size() == 0)
                          cdr::cache::fatal (
                              L"No cdr:ref for parent of Term=" + docIdStr);

                      // There can be more ParentTerm elements
                      // Process this one now by getting it and recursively
                      //   getting its parent terms
                      pParentTerm = cdr::cache::Term::getTerm (conn, pMap,
                                                    depth + 1, parentIdStr);

                      // If we haven't already seen this one, process it
                      if (!pTerm->parentPtrs.count(pParentTerm)) {

                        // Get all the attributes associated with parent and
                        //   save them too
                        // But we may have seen the term in another parent
                        //   tree, so check before building attributes
                        if (pParentTerm->attrXml.size() == 0) {
                          cdr::dom::NamedNodeMap attrs =
                                       gChildNode.getAttributes();
                          int nAttrs = attrs.getLength();
                          for (int i=0; i<nAttrs; i++) {
                              cdr::dom::Node attr = attrs.item(i);
                              cdr::String attrName = attr.getNodeName();
                              cdr::String attrVal  = attr.getNodeValue();
                              pParentTerm->attrXml += s_SPACE
                                  + attrName.toUtf8() + s_EQUOT
                                  + attrVal.toUtf8() + s_QUOTE;
                          }
                        }
std::cout << pParentTerm->name << pParentTerm->attrXml << "\n";

                        // Install it in the parent term set for this Term
                        pTerm->parentPtrs.insert(pParentTerm);

                        // Install all of its parents in the set for this Term
                        PARENT_SET::iterator parIter =
                                  pParentTerm->parentPtrs.begin();
                        while (parIter != pParentTerm->parentPtrs.end()) {
                          if (!pTerm->parentPtrs.count(*parIter))
                            pTerm->parentPtrs.insert(*parIter);
                          ++parIter;
                        }
                      }
                    }

                    // Else keep looking for ID
                    gChildNode = gChildNode.getNextSibling();
                  }
                }

                // Keep looking for parent relationship.  Can be more than 1
                childNode = childNode.getNextSibling();
              }
            }
          }

          // Next node under document element
          // Looking for PreferredName or TermRelationship
          node = node.getNextSibling();
        }
    }

    // Install this Term in the the id->term map
    // Do this even if Term is NULL to prevent redoing same one
    (*pMap)[docId] = pTerm;

    // And hand it back to caller
    return pTerm;
}

// Get preferred name as XML string
std::string cdr::cache::Term::getNameXml() {

    if (nameXml != "")
        nameXml = "<Term>\n"
                  " <PreferredName>" + name + "</PreferredName>\n"
                  "</Term>\n";

    return nameXml;
}

// Get full term family (term and parents) as XML string
std::string cdr::cache::Term::getFamilyXml() {

    if (familyXml == "") {
        familyXml = "<Term xmlns:cdr = 'cips.nci.nih.gov/cdr'>\n"
                    " <PreferredName>" + name + "</PreferredName>\n";

        // Get name of each parent
        PARENT_SET::iterator parIter = parentPtrs.begin();
        while (parIter != parentPtrs.end()) {
            // Only include terms for which the type was ok and name stored
            if ((*parIter)->typeOK)
                familyXml += " <Term" + (*parIter)->getAttrs() +
                             "><PreferredName>" + (*parIter)->getName() +
                             "</PreferredName></Term>\n";
            ++parIter;
        }

        familyXml += "</Term>\n";
std::cout << "familyXml = " << familyXml << "\n";
    }

    return familyXml;
}

// Empty and release a map
void cdr::cache::Term::clearMap(
    TERM_MAP *pMap
) {
    // Delete everything in the map
    // This hits every Term ever allocated, so there's no need to
    //   process individual Term parentPtrs.
    TERM_MAP::iterator mapIter = pMap->begin();
    while (mapIter != pMap->end()) {
        delete mapIter->second;
        ++mapIter;
    }

    // If pointing to the static map, have to reset pointer after deletion
    bool staticMap = (pMap == S_pTermMap) ? true : false;

    // Now the map itself
    delete pMap;

    // And prevent re-use if necessary
    if (staticMap)
        S_pTermMap = 0;
}

// Client capability to set cacheing on or off
// See CdrCommand.h
cdr::String cdr::cacheInit(
    cdr::Session& session,
    const cdr::dom::Node& node,
    cdr::db::Connection& conn
) {
    // XXX - NOTE: No authorization checking on this?
    bool startTermCacheing = false;     // true = start
    bool stopTermCacheing  = false;     // true = stop
    bool legalCacheingCmd  = false;     // true = found some legal command

    // Parse attributes of command
    cdr::dom::NamedNodeMap attributes = node.getAttributes();
    cdr::String onAttr;
    cdr::String offAttr;

    cdr::dom::Node attrNode = attributes.getNamedItem("on");
    if (attrNode != NULL)
        onAttr = attrNode.getNodeValue();
    attrNode = attributes.getNamedItem("off");
    if (attrNode != NULL)
        offAttr = attrNode.getNodeValue();

    // Did we get at least one?
    if (onAttr.size() == 0 && offAttr.size() == 0)
        throw cdr::Exception (L"Missing required 'on' or 'off' attribute"
                              L" in CdrCacheing command");

    // Convert and test for a valid value
    if (! (
            getCacheTypes (onAttr, &startTermCacheing) ||
            getCacheTypes (offAttr, &stopTermCacheing)
       )  )
        throw cdr::Exception (L"No legal value found for on/off attribute"
                              L" in CdrCacheing command");

    // This test is here just for if and when we have more cacheing types
    if (startTermCacheing || stopTermCacheing)
        cdr::cache::Term::initTermCache (startTermCacheing);

    return cdr::String(L"</CdrCacheingResp>");
}

/**
 * Parse an "on" or "off" attribute value to find out what is
 * requested.
 *
 * Legal values now are:
 *    "term"    Turn on/off term cacheing
 *    "pub"     Turn on/off all publication related cacheing
 *    "all"     Turn on/off all cacheing
 *
 * As of this writing, there is only one kind, so all are equivalent.
 *
 * @param cacheCmd      String value of attribute.
 * @param termCmd       Ptr to boolean to set true if term cacheing
 *                        referenced, else false.
 * @return              True=found a valid value, else false.
 *
 * May add more later.
 */
static bool getCacheTypes(
    cdr::String cacheCmd,
    bool        *termCmd
) {
    // Default value
    *termCmd = false;

    // Look for valid values
    // Add new ones if and where appropriate
    if (cacheCmd.find (L"term") != cacheCmd.npos)
        *termCmd = true;
    if (cacheCmd.find (L"pub") != cacheCmd.npos)
        *termCmd = true;
    if (cacheCmd.find (L"all") != cacheCmd.npos)
        *termCmd = true;

    // If more added, return *xCmd | *yCmd | *zCmd, etc.
    return *termCmd;
}

// Non-fatal error
void cdr::cache::error(
    cdr::String msg
) {
    // Log it and return
    cdr::log::pThreadLog->Write(cdr::ExcpLogSrc, msg);
}

// Fatal error, see caveats in CdrCache.h
void cdr::cache::fatal(
    cdr::String msg
) {
    // Logs and throws
    throw cdr::Exception("Fatal error in term denormalization", msg);
}
