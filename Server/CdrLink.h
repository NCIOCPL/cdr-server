/*
 * $Id$
 *
 * Header for Link Module software - to maintain the link_net
 * table describing the link relationships among CDR documents.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.11  2006/11/28 22:42:19  ameyer
 * Defines for link_chk types.
 *
 * Revision 1.10  2006/10/06 02:41:27  ameyer
 * Minor modifications to support link target version type distinctions.
 *
 * Revision 1.9  2002/08/29 21:53:07  ameyer
 * Added constants for some initial values that can be set or read in different
 * places for CdrLink members.
 *
 * Revision 1.8  2002/08/14 01:36:51  ameyer
 * Added constants for max sizes in link references.
 *
 * Revision 1.7  2002/05/08 20:30:26  pzhang
 * Added getSearchLinksResp and getSearchLinksRespWithProp.
 *
 * Revision 1.6  2002/01/31 16:28:21  ameyer
 * Added trgFound to indicate target of link found in database.
 *
 * Revision 1.5  2001/05/17 17:39:50  ameyer
 * Minor changes to comments.
 *
 * Revision 1.4  2001/04/17 23:14:03  ameyer
 * Added customLinkCheck()
 * Small changes in validation.
 *
 * Revision 1.3  2000/09/27 20:25:39  bkline
 * Fixed last argument to findTargetDocTypes().
 *
 * Revision 1.2  2000/09/27 11:29:23  bkline
 * Added CdrDelLinks() and findTargetDocTypes().
 *
 * Revision 1.1  2000/09/26 19:05:00  ameyer
 * Initial revision
 *
 */

#ifndef CDR_LINK_
#define CDR_LINK_

#include "CdrString.h"
#include "CdrValidateDoc.h"
#include "CdrDbConnection.h"
#include <vector>

/**@#-*/

namespace cdr {
  namespace link {

/**@#+*/

    /** @pkg cdr */

    /**
     * Max number of errors we report for one missing fragment id.
     * This is so that if a fragment identifier is deleted from a
     *   document and 1000 other documents reference it, we don't
     *   report 1000 errors.
     */
    const int MaxFragMissErrors = 5;

    /**
     * Max sizes of objects stored in the database.
     * Larger would be rejected by DBMS.
     */
    const int MAX_FRAG_SIZE = 32;
    const int MAX_URL_SIZE  = 256;

    /**
     * Values used in a CdrLink object when we have no values
     * provided in the cdr:ref, cdr:href or cdr:xref.
     *
     * These values are only used in memory.  The database uses
     * NULL for these values.
     */
    const int         NO_DOC_ID   = 0;
    const cdr::String NO_FRAGMENT = L"";
    const cdr::String NO_REF_TEXT = L"";

    /**
     * Possible linking styles
     */
    typedef enum LinkStyle {
        not_set,        // Don't yet know what it is
        ref,            // Link via cdr:ref attribute
        href,           // cdr:href - retains element content
        xhref           // xlink:href - no validation
    } LinkStyle;

    /**
     * Three different kinds of versions and link can point to.
     *
     * These guide indexing in query_term_def
     */
    typedef enum LinkChkVersion {
        CHK_CWD = 'C',  // Check against current working document
        CHK_PUB = 'P',  // Check against last published version
        CHK_VER = 'V'   // Check against last any version
    } LinkChkVersion;

    /**
     * List of CdrLink objects.
     * Requires forward declaration of CdrLink class.
     */
    class CdrLink;
    typedef std::list<CdrLink> LnkList;

    /**
     * Class for manipulating link_net entries.
     */

    class CdrLink {
        public:
            /**
             * Constructor.
             *
             * The main purpose of the constructor is to capture
             * a description of the link.  However some validation
             * is also done here.  Errors generated by the link
             * capture itself are produced here instead of delayed
             * for later re-checking.
             *
             * @param   conn        Connection to the database.
             * @param   elemDom     DOM node for element containing link.
             * @param   docId       ID of source document containing link.
             * @param   docType     Document type identifier for source doc.
             * @param   docTypeStr  String form, to save getting it again.
             * @param   refValue    The reference itself, e.g., "CDR1234..."
             * @param   lnkStyle    Type of attribute containing link.
             */
            CdrLink (cdr::db::Connection&, cdr::dom::Node&, int, int,
                     cdr::String, cdr::String, LinkStyle);

            /**
             * Default copy constructor from compiler is fine.
             * Default destructor also fine.
             */

            /**
             * validateLink
             *
             * Performs all validations which can be performed on a
             * free-standing link.  Other routines than this one
             * perform any link validations which must be performed
             * on the document as a whole (e.g., does it have any
             * duplicate fragment ids, and contain any fragment ids
             * referred to by other documents.)
             *
             * @param  dbConn       Reference to database connection.
             * @param  fragSet      Reference to set of frag ids in src doc
             * @return              Count of errors for this link so far.
             */
            int validateLink (cdr::db::Connection&, cdr::StringSet &);

            /**
             * dumpXML
             *
             * Retrieves all of the information we have about a link
             * and serializes it into a string of xml.
             *
             * @param  dbConn       Reference to database connection.
             * @return              XML string containing link info,
             *                      including any errors.
             */
            cdr::String CdrLink::dumpXML (cdr::db::Connection&);

            /**
             * dumpString
             *
             * Retrieves all of the information we have about a link
             * and serializes it into a human readable string.
             *
             * This is like dumpXML(), but does not contain XML tags,
             * using human readable formatting instead - more suitable
             * for human readable error messages.
             *
             * @param  dbConn       Reference to database connection.
             * @return              XML string containing link info,
             *                      including any errors.
             */
            cdr::String CdrLink::dumpString (cdr::db::Connection&);

            /**
             * Add an error message to a link object
             *
             * @param  msg          Error message to add.
             */
            void CdrLink::addLinkErr (cdr::String);

            /**
             * Get the count of errors for one link.
             *
             * @return              Count for this one link.
             */
            int CdrLink::getErrorCount ();

            /**
             * Set the link to not be saved in the event of certain errors
             *
             * @param  s            New saveLink value.
             */
            void CdrLink::setSaveLink (bool s) { saveLink = s; };

            /**
             * Accessors.
             * XXXX - Could modify a few of these to generate data
             *        when called, if it doesn't exist.
             */
            int         getType          (void) { return type; }
            cdr::String getTypeStr       (void) { return typeStr; }
            int         getSrcId         (void) { return srcId; }
            cdr::String getSrcField      (void) { return srcField; }
            int         getSrcDocType    (void) { return srcDocType; }
            cdr::String getSrcDocTypeStr (void) { return srcDocTypeStr; }
            int         getTrgId         (void) { return trgId; }
            cdr::String getTrgIdStr      (void) { return trgIdStr; }
            int         getTrgDocType    (void) { return trgDocType; }
            cdr::String getTrgActiveStat (void) { return trgActiveStat; }
            cdr::String getTrgDocTypeStr (void) { return trgDocTypeStr; }
            cdr::String getRef           (void) { return ref; }
            cdr::String getTrgFrag       (void) { return trgFrag; }
            cdr::String getChkType       (void) { return chkType; }
            const cdr::dom::Node&
                        getFldNode       (void) { return fldNode; }
            bool        getHasData       (void) { return hasData; }
            LinkStyle   getStyle         (void) { return style; }
            bool        getSaveLink      (void) { return saveLink; }
            cdr::ValidationControl getErrCtl (void) { return errCtl; }


        private:
            // CdrLink objects are shallow copied via push_back to a LnkList.
            // Beware using references here.
            // The objects are small enough that I haven't bothered to
            //   try to optimize things with pointers, but that could
            //   be done to save memory and copy time.
            int         type;           // From link_type table
            cdr::String typeStr;        // Human readable form of same
            int         srcId;          // Doc ID of source of link
            cdr::String srcField;       // Tag of element containing link
            int         srcDocType;     // From doc_type table
            cdr::String srcDocTypeStr;  // Human readable form
            bool        trgFound;       // True=Target found in database
            int         trgId;          // Doc ID of target doc
            cdr::String trgIdStr;       // String format of target id
            int         trgDocType;     // Doc_type of target doc
            cdr::String trgDocTypeStr;  // String format
            cdr::String trgActiveStat;  // 'A'=active, 'D'=deleted target doc
            cdr::String ref;            // Full reference as string
            cdr::String chkType;        // Check for P)ub V)er or C)WD target
            cdr::String trgFrag;        // #Fragment part of ref, if any
            const cdr::dom::Node&
                        fldNode;        // DOM node in parsed xml
            bool        hasData;        // True=denormalized data in node
            LinkStyle   style;          // cdr:ref, cdr:href, xlink:href
            bool        saveLink;       // True=Save to link_net
            ValidationControl errCtl;   // Holds error messages
    };

    /**
     * Update the link net from a list of link objects
     */
    extern void updateLinkNet (cdr::db::Connection&, int, cdr::link::LnkList&);

    /**
     * Validate links in a document and, optionally, update the link_net
     * database by calling updateLinkNet().
     *
     * Called by execValidateDoc() for validation of new or modified docs.
     *
     *  @param      docObj      Pointer to the CdrDoc object to validate.
     *  @param      node        Reference to a DOM parse tree for the XML
     *                           of the document to be written to the database.
     *  @param      validRule   Relationship between validation and link_net
     *                           update.  Values are:
     *                             ValidateOnly
     *                             UpdateIfValid
     *                             UpdateUnconditionally
     *  @return                 Count of errors.
     *                           0 = complete success.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern int cdrSetLinks (cdr::CdrDoc*, cdr::dom::Node&, cdr::ValidRule);

    /**
     * Delete all link table entries for which a document is the source.
     * Do this when deleting a document.
     *
     * We also check to see if the document is a target of links.  The
     * calling program can control whether to delete links from a document
     * based on whether there are any links to it.
     *
     * Called by cdrDelDoc() when deleting a document.
     *
     *  @param      dbConn      Reference to the connection object for the
     *                           CDR database.
     *  @param      docId       Document UI, as an integer.
     *  @param      vRule       Relationship between validation and link_net
     *                           update.  Values are:
     *                             ValidateOnly
     *                             UpdateIfValid
     *                             UpdateUnconditionally
     *  @param      errlist     Reference to string to receive errors.
     *  @return                 Count of errors.
     *                           0 = complete success.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern int cdrDelLinks (cdr::db::Connection&, int, cdr::ValidRule,
                            cdr::StringList&);

    /**
     * Return the CdrSearchLinksResp element that represents target links
     * made from a particular element type in a given source document type.
     * It contains only the documents satisfying the link_properties. It
     * is designed for task: picklists with server-side filtering, and
     * hence it emphasizes on speed by not using other funtions in cdr::link.
     * This is assumed to be a replacement for findTargetDocTypes().
     *
     *  @param      conn         Reference to the connection object for the
     *                            CDR database.
     *  @param      srcElem      Reference to a string containing the source
     *                            element name.
     *  @param      srcDocType   Reference to a string containing the source
     *                            document type.
     *  @param      titlePattern Reference to a string containing the target
     *                            document title pattern.
     *  @param      maxRows      Maximum number of (id, title) pairs returned.
     *
     *  @return                  String of serial XML containing qualifying
     *                            doc IDs and titles, usually for a picklist.
     *
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern cdr::String getSearchLinksResp (cdr::db::Connection& conn,
                                    const cdr::String&   srcElem,
                                    const cdr::String&   srcDocType,
                                    const cdr::String&   titlePattern,
                                    int                  maxRows);

    /**
     * Handles the link proprty specific code for CdrSearchLinksResp.
     *
     * Parses the rule values for each property ID, builds a parse
     * tree, and generates and executes SQL for the rule.
     *
     *  @param      conn         Reference to the connection object for the
     *                            CDR database.
     *  @param      link_id      Link type unique ID in link_type table.
     *  @param      prop_ids     Reference to vector of unique IDs in
     *                            link_properties table.
     *  @param      prop_values  Reference to vector of string property
     *                            values, one for each prop_id.  These are
     *                            rules to compile.  If rule changed since
     *                            last compile, a value compare will detect
     *                            that and recompile it.
     *  @param      titlePattern Reference to a string containing the target
     *                            document title pattern, e.g. "antibody%" to
     *                            generate a picklist of qualifying docs
     *                            with titles LIKE 'antibody%'
     *  @param      maxRows      Maximum number of (id, title) pairs returned.
     *
     *  @return                  String of serial XML containing qualifying
     *                            doc IDs and titles, usually for a picklist.
     *
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern cdr::String getSearchLinksRespWithProp (
                                    cdr::db::Connection&      conn,
                                    int                       link_id,
                                    std::vector<int>&         prop_ids,
                                    std::vector<cdr::String>& prop_values,
                                    const cdr::String&        titlePattern,
                                    int                       maxRows);

    /**
     * Check for and execute any custom link procedures.
     * Checks a DBMS table to find out what procedures are required for
     * this type of link and then invokes the appropriate code for them.
     *
     *  @param      conn        Reference to the connection object for the
     *                           CDR database.
     *  @param      link        Reference CdrLink object for link to check.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern int customLinkCheck (cdr::db::Connection& conn,
                                 cdr::link::CdrLink* link);
  }
}

#endif // CDR_LINK_
