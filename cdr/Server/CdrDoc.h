/*
 * CdrDoc.h
 *
 * Header file for adding/replacing/deleting documents in the database.
 *
 *                                          Alan Meyer  May, 2000
 *
 * $Id: CdrDoc.h,v 1.17 2003-01-10 00:59:29 ameyer Exp $
 *
 */

#ifndef CDR_DOC_
#define CDR_DOC_

#include "CdrSession.h"
#include "CdrDom.h"
#include "CdrString.h"
#include "CdrDbConnection.h"
#include "CdrBlob.h"

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
             */
            CdrDoc (cdr::db::Connection& dbConn, cdr::dom::Node& docDom);

            /**
             * Create a CdrDoc object using a passed document ID.
             * The document is present in the database.
             *
             *  @param  dbConn      Reference to database connection allowing
             *                      access to doc in database.
             *
             *  @param  docId       CDR document ID for a document currently
             *                      in the database.
             */
            CdrDoc (cdr::db::Connection& dbConn, const int docId);

            /**
             * Delete a CdrDoc object
             */
            ~CdrDoc () {}

            /**
             * Store a CdrDoc in the database
             *
             *  @return void        Throws exception in the event of error
             */
            void Store ();

            /**
             * Replaces the rows in the query_term table for the current
             * document.
             */
            void updateQueryTerms();

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
             *  @param revisionLevel    Integer 1..3 where
             *                            1=All proposed revisions included
             *                            2=All accepted revisions included
             *                            3=Only accepted and publishable
             *                              revisions included
             *                          Default = DEFAULT_REVISION_LEVEL.
             *                          These are Well Known Numbers.
             *  @param getIfUnfiltered  True=Return unfiltered Xml if
             *                            revision filtering fails for any
             *                            reason.  Note that raw Xml may
             *                            well be unusuable if filtering
             *                            didn't work.
             *                          Default = false;
             *  @return                 Filtered (or raw) XML, or L"".
             */
            cdr::String getRevisionFilteredXml (
                    int revisionLevel=DEFAULT_REVISION_LEVEL,
                    bool getIfUnfiltered=false);

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
             * Update the status of a protocol.
             *
             *  @param  validating      whether the user has requested
             *                          validation as part of the save
             *                          command.
             */
            void updateProtocolStatus(bool validating);

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

            // Accessors
            int getId()                    {return Id;}
            int getDocType()               {return DocType;}
            bool getNeedsReview()          {return NeedsReview;}
            cdr::String getTextId()        {return TextId;}
            cdr::String getValStatus()     {return ValStatus;}
            cdr::String getValDate()       {return ValDate;}
            cdr::String getActiveStatus()  {return ActiveStatus;}
            cdr::String getDbActiveStatus(){return dbActiveStatus;}
            cdr::String getTextDocType()   {return TextDocType;}
            cdr::String getTitle()         {return Title;}
            cdr::String getXml()           {return Xml;}
            cdr::String getComment()       {return Comment;}
            cdr::db::Connection& getConn() {return docDbConn;}
            cdr::dom::Element& getDocumentElement() {return docElem;}
            void setValStatus(const cdr::String& vs) { ValStatus = vs; }

            // Get errors as an STL list of strings
            cdr::StringList& getErrList() {return errList;}

            // Get errors packed as a single string - only if there are any
            cdr::String getErrString() {
                if (errList.size() > 0)
                    return cdr::packErrors(errList);
                return L"";
            }


        private:
            // Values corresponding to document table data
            int Id;                     // Numeric form of document id
            int DocType;                // Internal key to document type
            cdr::String TextId;         // With "CDR00..." prefix
            cdr::String ValStatus;      // V(alid) I(nvalid)
                                        //   U(nvalidated) M(alformed)
            cdr::String ValDate;        // Datetime
            cdr::String ActiveStatus;   // 'A', 'I', 'D' - may change
            cdr::String dbActiveStatus; // ActiveStatus in database, before chg
            cdr::String TextDocType;    // Form used in document tag
            cdr::String Title;          // External title
            cdr::String Xml;            // Actual document as XML, not CDATA
            cdr::String revisedXml;     // After any filtering of insertion
                                        //  and deletion markup
            cdr::String schemaXml;      // Schema text for this doc
            cdr::Blob   BlobData;       // Associated non-XML, if any
            cdr::String Comment;        // Free text
            cdr::dom::Element docElem;  // Top node of a parsed document
            bool NeedsReview;           // True=User marked doc as needing it
            bool parsed;                // True=parse was attempted
            bool malformed;             // True=parse failed
            bool revFilterFailed;       // True=Revision filtering failed
            int  revFilterLevel;        // Filtering done at this level
            int  schemaDocId;           // Doc id for the schema for this doc
            int  titleFilterId;         // Filter id for constructing Title
            int  lastFragmentId;        // Last used generated cdr:id number
            cdr::StringList errList;    // Errors from validation, parsing,
                                        //   filtering, or wherever.
            ContentOrControl conType;   // Treat as content or control info

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
            void cdr::CdrDoc::createTitle();
    };
}

#endif // CDR_DOC_
