/*
 * CdrDoc.h
 *
 * Header file for adding/replacing/deleting documents in the database.
 *
 *                                          Alan Meyer  May, 2000
 *
 * $Id: CdrDoc.h,v 1.7 2001-06-20 00:57:19 ameyer Exp $
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
             *  @param errorStr         Reference to string to receive
             *                            any filter warning or error
             *                            messages.
             *  @param revisionLevel    Integer 1..3 where
             *                            1=All proposed revisions included
             *                            2=All accepted revisions included
             *                            3=Only accepted and publishable
             *                              revisions included
             *                          Default = 3.
             *                          These are Well Known Numbers.
             *  @param getIfUnfiltered  True=Return unfiltered Xml if
             *                            revision filtering fails for any
             *                            reason.  Note that raw Xml may
             *                            well be unusuable if filtering
             *                            didn't work.
             *                          Default = false;
             *  @return                 Filtered (or raw) XML, or L"".
             */
            cdr::String getRevisionFilteredXml (cdr::String &errorStr,
                                                int revisionLevel=3,
                                                bool getIfUnfiltered=false);

            /**
             * Mark a document as malformed.
             */
            void malFormed();

            // Accessors
            int getId()                    {return Id;}
            int getDocType()               {return DocType;}
            cdr::String getTextId()        {return TextId;}
            cdr::String getValStatus()     {return ValStatus;}
            cdr::String getValDate()       {return ValDate;}
            cdr::String getActiveStatus()  {return ActiveStatus;}
            cdr::String getTextDocType()   {return TextDocType;}
            cdr::String getTitle()         {return Title;}
            cdr::String getXml()           {return Xml;}
            cdr::String getComment()       {return Comment;}
            cdr::db::Connection& getConn() {return docDbConn;}
            cdr::dom::Element& getDocumentElement() {return docElem;}

            // Get errors as an STL list of strings
            cdr::StringList getErrList() {return errList;}

            // Get errors packed as a single string
            cdr::String getErrString() {return cdr::packErrors(errList); }


        private:
            // Values corresponding to document table data
            int Id;                     // Numeric form of document id
            int DocType;                // Internal key to document type
            cdr::String TextId;         // With "CDR00..." prefix
            cdr::String ValStatus;      // V(alid) I(nvalid)
                                        //   U(nvalidated) M(alformed)
            cdr::String ValDate;        // Datetime
            cdr::String ActiveStatus;   // Y/N
            cdr::String TextDocType;    // Form used in document tag
            cdr::String Title;          // External title
            cdr::String Xml;            // Actual document as XML, not CDATA
            cdr::String revisedXml;     // After any filtering of insertion
                                        //  and deletion markup
            cdr::Blob   BlobData;       // Associated non-XML, if any
            cdr::String Comment;        // Free text
            cdr::dom::Element docElem;  // Top node of a parsed document
            bool parsed;                // True=parse was attempted
            bool malformed;             // True=parse failed
            bool revFilterFailed;       // True=Revision filtering failed
            int  revFilterLevel;        // Filtering done at this level
            cdr::StringList errList;    // Errors from validation, parsing,
                                        //   filtering, or wherever.

            // Connection to the database
            cdr::db::Connection& docDbConn;

            /**
             * Adds a row to the query_term table for the current node if
             * appropriate and recursively does the same for all sub-elements.
             *
             *  @param  path        reference to string representing path for
             *                      current node; e.g., "/Person/PersonStatus".
             *  @param  node        reference to current node of document's
             *                      DOM tree.
             *  @param  paths       reference to set of paths to be indexed.
             */
            void addQueryTerms(const cdr::String& path,
                               const cdr::dom::Node& node,
                               const StringSet& paths);

            /**
             * Add a row to the query_term table for the current node
             * if appropriate, and recursively do the same for all
             * sub-elements.
             *
             *  @param  parentPath  Reference to string representing
             *                      path for parent of current node;
             *                      e.g., "/Person/PersonStatus".
             *                      Null string if there is no parent.
             *  @param  nodeName    Name of current element or attribute.
             *  @param  node        Reference to current node of
             *                      document's DOM tree.
             *  @param  paths       Reference to set of paths to be indexed.
             */
            void cdr::CdrDoc::addQueryTerms(const cdr::String& parentPath,
                                            const cdr::String& nodeName,
                                            const cdr::dom::Node& node,
                                            const StringSet& paths);
    };
}

#endif // CDR_DOC_
