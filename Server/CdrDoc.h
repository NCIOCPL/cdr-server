/*
 * CdrDoc.h
 *
 * Header file for adding/replacing/deleting documents in the database.
 *
 *                                          Alan Meyer  May, 2000
 *
 * $Id$
 *
 * BZIssue:: 4767
 */

#ifndef CDR_DOC_
#define CDR_DOC_

#include "CdrSession.h"
#include "CdrDom.h"
#include "CdrString.h"
#include "CdrDbConnection.h"
#include "CdrBlob.h"
#include "CdrValidationCtl.h"

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

    /**
     * The default revision markup filter level.  All documents
     * are filtered to only include approved and publishable
     * revsions for default validation.
     */
    const int DEFAULT_REVISION_LEVEL = 3;

    /**
     * Maximum number of hierarchical levels for which we are willing
     * to index fields in a document.
     *
     * If we reach this level something is seriously wrong.
     */
    const int MAX_INDEX_ELEMENT_DEPTH = 40;

    /**
     * Width of the hex representation of the ordinal position of an
     * element among it's siblings.  For example, for width 4:
     *   Element  0 = "0000"
     *   Element 17 = "0011"
     *     etc.
     */
    const int INDEX_POS_WIDTH = 4;

    /**
     * Maximum number of ordinal positions representable at one level.
     * The number is so high that it's inconceivable that any actual
     * document could reach it.
     */
    const int MAX_INDEX_ELEMENT_POS = 0x7FFFFFFF;

    /**
     * Indication of whether the document should be processed as
     * a content document (one with user produced information in it)
     * or a control document (one with control information, such
     * as a schema or stylesheet produced by programmers.)
     */
    typedef enum ContentOrControl {
        not_set,        // Haven't figured it out yet
        content,        // Doc contains user produced content
        control         // Doc contains programmer produced control data
    } ContentOrControl;

    /**
     * Indications of the status of a CdrDoc object with respect to
     * publishability.  Does this object represent a publishable
     * version or not.
     */
    typedef enum VerPubStatus {
        publishable,        // Version is known to be publishable
        nonPublishable,     // Version known to be non-publishable
        unknown             // Don't currently know the status
    } VerPubStatus;

    /**
     * Two types of operation are allowed on PermaTargIds during link
     * processing.  See ckPermaTargNode() in CdrLink.cpp.
     */
    typedef enum {
        Insert,         // Inserting a new permaTargId
        Delete          // Delete a permaTargId
    } ptargOp;

    /**
     * Pairs of operation + permaTargId.  ID always 0 when operation==Insert
     */
    typedef std::pair<ptargOp, cdr::String> ptargPair;

    /**
     * 0 or more PermaTargIds can be processed in one doc.
     * A vector stores up all of the IDs to process.
     */
    typedef std::vector<ptargPair> ptargPairVector;

    // Object to represent one row in the query_term table.
    struct QueryTerm {
        int         doc_id;
        cdr::String path;
        cdr::String value;
        cdr::Int    int_val;
        cdr::String node_loc;
        QueryTerm(int id, const cdr::String& p, const cdr::String& v,
                  const cdr::Int& iv, const cdr::String& nl) :
            doc_id(id), path(p), value(v), int_val(iv), node_loc(nl) {}
        bool operator==(const QueryTerm& qt) const {
            return doc_id   == qt.doc_id &&
                   path     == qt.path &&
                   value    == qt.value &&
                   node_loc == qt.node_loc;
        }
        bool operator<(const QueryTerm& qt) const {
            if (doc_id < qt.doc_id) return true;
            if (doc_id > qt.doc_id) return false;
            if (path   < qt.path)   return true;
            if (path   > qt.path)   return false;
            if (value  < qt.value)  return true;
            if (value  > qt.value)  return false;
            return node_loc < qt.node_loc;
        }
    };

    typedef std::set<QueryTerm> QueryTermSet;

    /**
     * Class for manipulating CDR documents - adding, replacing and deleting.
     */

    class CdrDoc {
        public:
            /**
             * Create a CdrDoc object using a passed dom reference.
             * The document is present in memory.
             *
             *  @param  dbConn      Reference to database connection allowing
             *                      access to doc in database.
             *
             *  @param  docDom      Reference to a dom parse of a CdrDoc
             *                      as would be passed in a CdrCommandSet.
             *
             *  @param session      reference to CDR session object
             *                      for this context
             *
             *  @param withLocators True = use cdr-eid attributes in
             *                      validation.
             */
            CdrDoc (cdr::db::Connection& dbConn, cdr::dom::Node& docDom,
                    cdr::Session& session, bool withLocators=false);

            /**
             * Create a CdrDoc object using a passed document ID.
             * The document is present in the database.
             *
             *  @param  dbConn      Reference to database connection allowing
             *                      access to doc in database.
             *
             *  @param  docId       CDR document ID for a document currently
             *                      in the database.
             *
             *  @param withLocators True = use cdr-eid attributes in
             *                      validation.
             */
            CdrDoc (cdr::db::Connection& dbConn, const int docId,
                    bool withLocators=false);

            /**
             * Create a CdrDoc object using a passed document ID and
             * version number.  The document is present in the database.
             *
             * NOTE: Partly limited version for use in query_term_pub
             *    updating.  Beef it up if we need it for other purposes.
             *
             *  @param  dbConn      Reference to database connection allowing
             *                      access to doc in database.
             *
             *  @param  docId       CDR document ID for a document currently
             *                      in the database.
             *
             *  @param  verNum      Version number, 0 = CWD.
             *
             *  @param withLocators True = use cdr-eid attributes in
             *                      validation.
             */
            CdrDoc (cdr::db::Connection& dbConn, const int docId,
                    const int verNum, bool withLocators=false);

            /**
             * Delete a CdrDoc object
             */
            ~CdrDoc () {}

            /**
             * Store a CdrDoc in the database
             *
             *  @return void        Throws exception in the event of error
             */
            void store ();

            /**
             * For some document types ("GlossaryTermConcept" as of this
             * writing), the document title incorporates the doc ID.
             * For new docs, the doc ID is not assigned until the document
             * is saved, so the saved title is not yet correct.
             *
             * This function is called after storing a new doc (not an
             * updated doc.)  It regenerates the title and compares the
             * regeneration to the stored title.  If they differ, it
             * stores the new title in the database.
             *
             * @return          True  = Title changed and was updated.
             *                  False = Title unchanged, no update needed.
             */
            bool checkTitleChange();

            /**
             * Replaces the rows in the query_term tables for the current
             * document.
             *
             * The update can be to the query_term table for current working
             * documents, the query_term_pub table for last publishable
             * versions, or both (the default).
             *
             * @param doCWD         True=update query_term
             * @param doPUB         True=update query_term_pub
             */
            void updateQueryTerms(bool doCWD=true, bool doPUB=true);

            /**
             * Tell whether a parse tree is available for the (possibly
             * filtered) XML in the document.
             *
             * If the document has not been filtered but needs to be, it
             * will be so filtered.  If it has not yet been parsed, it
             * will be parsed.
             *
             *  @return             true=document element is available.
             *                      false=malformed document, could not parse.
             */
            bool parseAvailable();

            /**
             * If it has not already been done, pass the XML for the
             * document through filtering to remove revision markup as
             * per our default revision markup rules.
             *
             * Uses revFilterLevel member, which must contain one of
             * the following values:
             *
             *   1 = Proposed, accepted, and published revisions included
             *   2 = Accepted and published revisions included
             *   3 = Only publishable revisions are included
             *
             * These are Well Known Numbers.
             *
             * The default value (3 or DEFAULT_REVISION_LEVEL) is set in
             * the constructor, and can be overridden in one of the
             * constructors by the RevisionFilterLevel attribute on the
             * CdrDoc element.
             *
             *  @param getIfUnfiltered  True=Return unfiltered Xml if
             *                            revision filtering fails for any
             *                            reason.  Note that raw Xml may
             *                            well be unusable if filtering
             *                            didn't work.
             *                          Default = false;
             *  @return                 Filtered (or raw) XML, or L"".
             */
            cdr::String getRevisionFilteredXml (bool getIfUnfiltered=false);

            /**
             * Pass the XML for the document through filtering to
             * add or replace cdr-eid attributes in every element.
             *
             * These attributes are used during validation to enable the
             * validation program to insert cdr:eref references to the
             * specific element instance that had a problem.
             *
             * createErrorIdXml() must be called before
             * getRevisionFilteredXML(), and before any other changes are
             * made to the record if error markup will be used.
             *
             *  @return                 XML with cdr-eid attributes.
             */
            cdr::String createErrorIdXml();

            /**
             * Mark a document as malformed.
             */
            void malFormed();

            /**
             * Walk through an array of pre-processing routines to modify the
             * document.
             *
             *  @param  validating      whether the user has requested
             *                          validation as part of the save
             *                          command.
             */
            void preProcess(bool validating);

            /**
             * Generate fragment identifiers for all elements for which
             * cdr:id is a legal attribute, but no cdr:id attribute exists.
             *
             * The fragment identifier is a unique (within the document)
             * value of the form "_n" where n is a number starting at 1
             * and going up as high as necessary.  The last used number
             * is stored with the row for the document in the document
             * table, so that the next time this routine is invoked,
             * no fragment id will be re-used.
             *
             * Generating fragment ids saves the user the time of generating
             * them and keeps him from having to search the and/or
             * validate the document be sure the id he creates is unique.
             */
            void genFragmentIds ();

            /**
             * Get a serialized version of the document using one of
             * the in-memory XML strings, wrapped in a CdrDoc/CdrDocXml
             * wrapper.
             *
             * The serial XML will include cdr-eid attributes if and only
             * if:
             *
             *   The CdrDoc was constructed withLocators = true.
             *
             *   There were errors that might reference those locators.
             *
             * Otherwise any cdr-eid attributes will be stripped.
             *
             * NOTES:
             *   The GetCdrDoc module was written before we created a CdrDoc
             *   module to encapsulate handling of documents in the system
             *   (even then we were too old to blame our youthful inexperience
             *   for that.)
             *
             *   If we ever re-write the CDR, for example in C#, we should
             *   definitely merge the functionality of the GetCdrDoc module
             *   into the CdrDoc module and eliminate a separate, non-object
             *   oriented approach to fetching serialized versions of
             *   documents.
             *
             *   This version only implements what we need for now.  more may
             *   be added later if we have a need.
             *
             * @return          Serial string within wrapper.
             */
            cdr::String getSerialXml();

            /**
             * Update the status of a protocol.
             *
             * A custom routine for InScopeProtocols - not part of regular
             * document processing.
             *
             *  @param  validating      whether the user has requested
             *                          validation as part of the save
             *                          command.
             */
            void updateProtocolStatus(bool validating);

            /**
             * Sort protocol organization sites.
             *
             * A custom routine for InScopeProtocols - not part of regular
             * document processing.
             */
            void sortProtocolSites();

            /**
             * Eliminate elements supplied by the template which the user
             * has decided not to use.  See the XSL/T script itself for
             * documentation of the logic.
             *
             *  @param  validating      whether the user has requested
             *                          validation as part of the save
             *                          command.
             */
            void stripXmetalPis(bool validating);

            /**
             * Is this a control type document?
             *
             *  @return                 True=yes.
             */
            bool isControlType();

            /**
             * Is this a content type document, reverse of isControlType.
             *
             *  @return                 True=yes.
             */
            bool isContentType();

            /**
             * Add an error message pertaining to this document.
             * Pass through to ValidationControl.addError().
             *
             *  @param msg              Error message.
             */
            void addError(cdr::String msg) {
                valCtl.addError(msg);
            }

            /**
             * Set the flag saying whether cdr-eid attribute error locators
             * will be used.
             *
             * Locators may only be used with content documents, never
             * with control type documents (schemas, css, filters).
             * An attempt to set locators for those types will (almost)
             * silently fail.  This allows callers to not worry about
             * the content type before calling the function.
             *
             *  @param locators         True=use cdr-eid attributes.
             *
             *  @return                 True=will use them.  Note that caller
             *                          might request them but we still
             *                          refuse to do it, returning false.
             */
            bool setLocators(bool locators);

            // Accessors
            int getId()                    {return Id;}
            int getDocType()               {return DocType;}
            int getRevFilterLevel()        {return revFilterLevel;}
            int getBlobId()                {return blobId; }
            bool getNeedsReview()          {return needsReview;}
            cdr::String getTextId()        {return textId;}
            cdr::String getValStatus()     {return valStatus;}
            cdr::String getValDate()       {return valDate;}
            cdr::String getActiveStatus()  {return activeStatus;}
            cdr::String getDbActiveStatus(){return dbActiveStatus;}
            cdr::String getTextDocType()   {return textDocType;}
            cdr::String getTitle()         {return title;}
            cdr::String getXml()           {return Xml;}
            cdr::String getComment()       {return comment;}
            cdr::Blob   getBlobData()      {return blobData;}
            cdr::db::Connection& getConn() {return docDbConn;}
            cdr::dom::Element& getDocumentElement() {return docElem;}
            cdr::VerPubStatus getVerPubStatus()     {return verPubStatus;}

            // Setters
            void setValStatus(const cdr::String& vs) {valStatus = vs;}
            void setVerPubStatus(cdr::VerPubStatus pvs) {verPubStatus=pvs;}

            // Return a reference to the ValidationControl object
            cdr::ValidationControl& getValCtl() { return valCtl; }

            // Set/get ValidationControl locator information
            // Pass through to ValidationControl, q.v.
            bool hasLocators() { return valCtl.hasLocators(); }

            // Get errors as an STL list of strings
            cdr::StringList& getErrList() {return errList;}

            // Get errors as XML
            // Pass through to ValidationControl
            // Gets errors (if any) from wherever they are
            cdr::String getErrorXml () {
                return valCtl.getErrorXml(errList);
            }

            /**
             * Access to count of error messages generated by
             * document validation.
             *
             *  @return            number of warnings.
             */
            int getErrorCount() const;

            /**
             * Set the Xml field to the serial content of the document.
             *
             * When this is done, if the Xml has changed, we have to force
             * anything generated from it to be re-generated.  This can
             * affect any of the following objects:
             *
             *   revisedXml - XML string with revision markup
             *   errorIdXml - XML string with error ID attributes
             *   docElem    - parse tree of the Xml.
             *
             * WARNING!
             *   Except for initialization in the constructors, this
             *   should be the ONLY WAY to set the value of the Xml
             *   field.  Any other way is dangerous and probably less
             *   optimized anyway.
             *
             * @param newXml    The new Xml to put in.
             */
            void setXml(cdr::String newXml);

            /**
             * Store a change to PermaTargIds that will be required when
             * updating a record that has inserted or deleted PermaTargIds.
             *
             *  @param targOp       Operation to perform, Insert or Delete
             *  @param targIdStr    Id on which to perform op, "0" for Insert.
             */
            void addPermaTargIdChange(ptargOp targOp, cdr::String targIdStr);

            /**
             * Answer the question: Has an action on a PermaTargId been queued
             * for execution?
             *
             * Used to allow the validation checks to ignore errors that are
             * about to be corrected when the queued operations are performed.
             *
             *  @param targOp       Operation to check: Insert or Delete.
             *  @param targIdNum    PermaTargId number to check for.
             *
             *  @return             True = Yes, this operation is queued.
             *                      Else false.
             */
            bool isPermaTargChangeQueued(ptargOp targOp, int targIdNum);

            /**
             * Apply all required changes to the PermaTargIds in a document.
             *
             * CdrLink.cpp ckPermaTargNode(), applied recursively to all
             * nodes, built a vector of pairs of operation identifiers
             * (Insert or Delete) + attribute values that were actually
             * found in the document XML.
             *
             * This function processes those pairs, performing the inserts
             * or deletes.
             *
             * It applies them one at a time, generating an amended xml
             * string after each application which is fed into the next.
             * It also performs any required database updates.
             *
             * At the end, it replaces the document XML with the transformed
             * version using setXml().
             *
             * WARNING:
             *
             *    This function modifies the XML in the document.  It is
             *    safest to call this function after we're done with
             *    all functions using the DOM parse tree used in validation.
             *
             *  @return     Void.
             *
             *  @throws cdr::Exception  If a serious error is encountered
             *                          that implies corruption of the
             *                          document or the database.
             */
            void applyPermaTargChanges();

        private:
            // Values corresponding to document table data
            int Id;                     // Numeric form of document id
            int DocType;                // Internal key to document type
            cdr::String textId;         // With "CDR00..." prefix
            cdr::String valStatus;      // V(alid) I(nvalid)
                                        //   U(nvalidated) M(alformed)
            cdr::String valDate;        // Datetime
            cdr::String activeStatus;   // 'A', 'I', 'D' - may change
            cdr::String dbActiveStatus; // activeStatus in database, before chg
            VerPubStatus verPubStatus;  // Object XML publishbility
            cdr::String textDocType;    // Form used in document tag
            cdr::String title;          // External title
            cdr::String Xml;            // Actual document as XML, not CDATA
            cdr::String revisedXml;     // After any filtering of insertion
                                        //   and deletion markup
            cdr::String schemaXml;      // Schema text for this doc
            cdr::Blob   blobData;       // Associated non-XML, if any
                                        //   Only loaded if exists and needed
            cdr::String comment;        // Free text
            cdr::dom::Element docElem;  // Top node of a parsed document
            bool needsReview;           // True=User marked doc as needing it
            bool parsed;                // True=parse was attempted
            bool malformed;             // True=parse failed
            bool revFilterFailed;       // True=Revision filtering failed
            int  revFilterLevel;        // Filtering done at this level
            int  requestedFilterLevel;  // Level requested in constructor
            int  schemaDocId;           // Doc id for the schema for this doc
            int  titleFilterId;         // Filter id for constructing title
            int  lastFragmentId;        // Last used generated cdr:id number
            int  blobId;                // ID of blob we just store, if any
            cdr::StringList errList;    // Errors from validation, parsing,
                                        //   filtering, or wherever.
            ContentOrControl conType;   // Treat as content or control info
            cdr::ValidationControl valCtl; // Holds error info

            // Vector of PermaTargId changes to apply.
            // Pairs of (op {Insert|Delete}, PermaTargId)
            ptargPairVector ptargPairs;

            // Connection to the database
            cdr::db::Connection& docDbConn;

            /**
             * Recursively finds all nodes in the tree which need to be
             * represented in the query_term table.
             *
             *  @param  parentPath  Reference to string representing path for
             *                      the parent node; e.g., "/Person/Status".
             *                      Will be an empty string for the top-level
             *                      invocation.
             *  @param  nodeName    Name of the current element; will be the
             *                      name of the document's top-level element
             *                      when initially invoked.
             *  @param  node        Reference to current node of document's
             *                      DOM tree.
             *  @param  paths       Reference to set of paths to be indexed.
             *  @param  ordPosPathp Pointer to a hex character representation
             *                      of the ordinal position of each element
             *                      in the DOM tree down to and including the
             *                      element to be indexed.
             *  @param  depth       Number of signficant levels in the
             *                      ordPosPathp.
             *  @param  qtSet       query term set collection to be populated
             *                      recursively by this method.
             */
            void collectQueryTerms(const cdr::String& path,
                                   const cdr::String& nodeName,
                                   const cdr::dom::Node& node,
                                   const StringSet& paths,
                                   wchar_t *ordPosPathp,
                                   int   depth,
                                   QueryTermSet& qtSet);


            /**
             * Generate a title for a document using an XSLT filter, if
             * one is available.
             *
             * Always creates a title, either from titleFilterId, if one
             * exists, or from a literal default string.  If an existing
             * user supplied title must be preserved, don't call this
             * function.
             */
            void createTitle();
    };
}

#endif // CDR_DOC_
