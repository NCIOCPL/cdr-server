/*
 * $Id: CdrCache.cpp,v 1.7 2005-03-04 02:41:06 ameyer Exp $
 *
 * Specialized cacheing for performance optimization, where useful.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.6  2004/07/02 03:21:21  ameyer
 * Changed set to map to insure that ordering is always the same, by term ID.
 * Before that I was just storing pointers to Term objects - which causes
 * the parents of a term to come out in different orders each time - which
 * would be a mess for publishing since docs would always appear to change.
 *
 * Revision 1.5  2004/07/02 02:13:46  ameyer
 * Fixed bug on cache start underflow.
 *
 * Revision 1.4  2004/07/02 01:26:36  ameyer
 * New concepts for starting and stopping cacheing using request counter.
 * New concept for timing out stale cache.
 * Bug fixes.
 *
 * Revision 1.3  2004/05/26 01:14:49  ameyer
 * New handling of PdqKey and cdr:ref attributes.
 *
 * Revision 1.2  2004/05/25 22:39:18  ameyer
 * This version gets PdqKey and cdr:ref for a parent from the terminology
 * link in the child record.  That works, and mirrors what used to be done,
 * but doesn't get the PdqKey and cdr:ref for the main term itself.
 * I'm going to change the way this is done in the next version.
 *
 * Revision 1.1  2004/05/14 02:04:34  ameyer
 * CdrCache serverside cacheing to speed publishing.
 *
 */

#include <iostream> // Debug
#include <sstream>
#include <time.h>

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

// Count of the number of times cacheing has been turned on.
// If process A turns on cacheing, then B turns it on, then either
//   A or B turns it off, we decrement the count and only turn
//   cacheing off when the count reaches 0.
// But see also S_termCacheStartTime.
static int S_cacheOnCount = 0;

// Time when the cache was initialized
// If it's too long, clear it
static time_t S_cacheStartTime = (time_t) 0;

// Name of the mutex preventing two separate threads from conflicting
//   reads or updates of the system-wide static S_pTermMap
static const char* const termCacheMutexName = "TermCacheMutex";

// Constructor
cdr::cache::Term::Term (
    int docId
) {
    id     = docId;
    typeOK = true;

    std::ostringstream os;
    os << " cdr:ref=\"" << cdr::stringDocId(id).toUtf8() << "\"";

    cdrRef = os.str();
    // std::cout << "Constructing Term, ref: " << cdrRef << "\n";
}

// Turn system-wide cacheing on or off.  Default start state is off
bool cdr::cache::Term::initTermCache(
    bool state
) {
    // Protect against multiple access conflicts
    HANDLE mutex = CreateMutex(0, false, termCacheMutexName);
    if (mutex != 0) {
        cdr::Lock lock(mutex, 10000);
        if (lock.m) {
            // Remember what it is before change
            bool prevState = S_cacheOnCount > 0 ? true : false;

            // If turning it on
            if (state) {
                // One more process wants it on
                ++S_cacheOnCount;

                // And turn it on if needed by creating a new
                //   static map of term doc ids to Term objects
                if (!prevState) {
                    S_pTermMap = new cdr::cache::TERM_MAP;
                    time(&S_cacheStartTime);
                }
            }

            // If turning it off
            else {
                // Decrement with sanity
                if (--S_cacheOnCount < 0)
                    S_cacheOnCount = 0;

                // Turn off by deleting the map if no process is
                //   still expecting the cache to be on
                if (S_cacheOnCount < 1 && S_pTermMap) {
                    cdr::cache::Term::clearMap(S_pTermMap);
                    S_cacheStartTime = (time_t) 0;
                }
            }

            // Release mutex
            CloseHandle(mutex);

            // Tell caller where we were
            return prevState;
        }
    }

    // If we got here we either couldn't create or find the
    //   mutex, or we couldn't acquire the lock
    // Neither should ever happen
    // Denormalization should take a fraction of a second
    std::wostringstream errMsg;
    errMsg << L"initTermCache: can't get mutex, GetLastError="
           << GetLastError();
    throw cdr::Exception (errMsg.str());
}

// Retrieve a utf-8 format string containing the denormalization
//  for a Term document and all its parent Term documents ...
std::string cdr::cache::Term::denormalizeTermId(
    const cdr::String&   xsltParms,
    cdr::db::Connection& conn
) {
    // Pointer to static system-wide map, or to local map on stack,
    //   depending on whether global caching is or is not enabled
    TERM_MAP *pMap;

    HANDLE mutex     = 0;       // Synchronize access to system-wide TERM_MAP
    bool   haveMutex = false;   // True = must release mutex
    cdr::String docIdStr;       // Document ID as a string
    bool   upcode;              // True=upcode as well as denormalize

    // String to return to caller
    std::string termsXml = "";

    // If static map, try to acquire it
    if (S_pTermMap) {

        // Make sure the map isn't too stale
        time_t now;
        time(&now);
        if (now - S_cacheStartTime > MAX_CACHE_DURATION) {
            // Clear it
            while (S_cacheOnCount > 0)
                initTermCache (false);

            // Alert somebody - it's a bug
            // XXX Send mail? XXX Throw exception?
            cdr::log::pThreadLog->Write("denormalizeTermId: ",
                    "Found and deleted stale Term denormalization map");
        }

        else {
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
    }

    // If no system-wide cacheing, or could not acquire mutex,
    //   use a local ID -> Term map just holding info for this one
    //   transaction
    if (!haveMutex)
        pMap = new TERM_MAP;

    // Parse the passed xslt parameters into docIdStr and optional
    //   suppress upcode indicator
    // Any slash delimited parameter after a docIdStr suppresses upcoding
    size_t parmPos = -1;
    if ((parmPos = xsltParms.find (L"/")) != -1) {
        docIdStr = xsltParms.substr (0, parmPos);
        upcode   = false;
    }
    else {
        docIdStr = xsltParms;
        upcode   = true;
    }
    // std::cout << "parms='" << xsltParms.toUtf8() << "'  docIdStr='" << docIdStr.toUtf8() << "'  upcode=" << upcode << "\n";

    // Denormalize the term, or find an existing denormalization
    Term *t = getTerm (conn, pMap, 0, docIdStr);

    // If it was a legitimate term, successfully denormalized
    if (t) {
        // If upcoding, get the entire family
        if (upcode)
            termsXml = t->getFamilyXml();
        // Else just the preferred name
        else
            termsXml = t->getNameXml();
    }

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
              pTerm->name = cdr::dom::getTextContent(node).toUtf8();

            // Save PdqKey to use as an attribute
            if (nodeName == L"PdqKey")
              pTerm->pdqKey = " PdqKey=\"Term:" +
                              cdr::dom::getTextContent(node).toUtf8() +
                              "\"";

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

            // If we're trying to upcode as well as denormalize the term
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
                          (cdr::dom::Element(gChildNode)).getAttribute(
                                                             "cdr:ref");
                      if (parentIdStr.size() == 0)
                          cdr::cache::fatal (
                              L"No cdr:ref for parent of Term=" + docIdStr);

                      // There can be more ParentTerm elements
                      // Process this one now by getting it and recursively
                      //   getting its parent terms
                      pParentTerm = cdr::cache::Term::getTerm (conn, pMap,
                                            depth + 1, parentIdStr);

                      // If we haven't already seen this one, process it
                      if (!pTerm->parentPtrs.count(pParentTerm->id)) {

                        // Install it in the parent term map for this Term
                        pTerm->parentPtrs[pParentTerm->id] = pParentTerm;

                        // Install all of its parents in the set for this Term
                        PARENT_MAP::iterator parIter =
                                  pParentTerm->parentPtrs.begin();
                        while (parIter != pParentTerm->parentPtrs.end()) {
                          if (!pTerm->parentPtrs.count(parIter->first))
                            pTerm->parentPtrs[parIter->first]=parIter->second;
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

    if (nameXml == "")
        // If not already seen, create the XML
        nameXml = makeTermStart() + "</Term\n>";

    return nameXml;
}

// Get full term family (term and parents) as XML string
std::string cdr::cache::Term::getFamilyXml() {

    if (familyXml == "") {
        // Main term, attributes, and preferred name
        familyXml = makeTermStart();

        // Get name of each parent
        PARENT_MAP::iterator parIter = parentPtrs.begin();
        while (parIter != parentPtrs.end()) {
            // Only include terms for which the type was ok and name stored
            Term *pParent = parIter->second;
            if (pParent->typeOK) {
                familyXml += " <Term";
                if (pParent->pdqKey.size() > 0)
                    familyXml += pParent->pdqKey;
                familyXml += pParent->cdrRef +
                             "><PreferredName>" + pParent->getName() +
                             "</PreferredName></Term>\n";
            }
            ++parIter;
        }

        familyXml += "</Term>\n";
        //std::cout << "familyXml = " << familyXml << "\n";
    }

    return familyXml;
}

// Create opening part of Term with namespace and attributes and pref name
std::string cdr::cache::Term::makeTermStart() {

    // XML namespace declaration required in order to use cdrRef
    //   which contains "cdr:ref='...'".  XSLT processor will
    //   complain if we don't declare cdr namespace prefix.
    std::string termStart = "<Term xmlns:cdr = 'cips.nci.nih.gov/cdr'";

    // Add PdqKey attribute
    if (pdqKey.size() > 0)
        termStart += pdqKey;

    // Add cdr:ref attribute + preferred name element
    termStart += cdrRef + ">\n <PreferredName>" + name + "</PreferredName>\n";

    // Return partially formed XML
    // Caller must add more data if desired plus Term end tag
    return termStart;
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
