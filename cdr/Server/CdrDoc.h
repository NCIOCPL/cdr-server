/*
 * CdrDoc.h
 *
 * Header file for adding/replacing/deleting documents in the database.
 *
 *                                          Alan Meyer  May, 2000
 *
 * $Id: CdrDoc.h,v 1.2 2000-06-23 15:31:14 bkline Exp $
 *
 */

#ifndef CDR_DOC_
#define CDR_DOC_

#include "CdrSession.h"
#include "CdrDom.h"
#include "CdrString.h"
#include "CdrDbConnection.h"

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
            // CdrDoc (cdr::Connection& dbConn, const cdr::String& docId);

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

            // Accessors
            int getId()                 {return Id;}
            int getDocType()            {return DocType;}
            cdr::String getTextId()     {return TextId;}
            cdr::String getValStatus()  {return ValStatus;}
            cdr::String getValDate()    {return ValDate;}
            cdr::String getApproved()   {return Approved;}
            cdr::String getTextDocType(){return TextDocType;}
            cdr::String getTitle()      {return Title;}
            cdr::String getXml()        {return Xml;}
            cdr::String getBlob()       {return Blob;} // XXXX Encode?
            cdr::String getComment()    {return Comment;}

        private:
            // Values corresponding to document table data
            int Id;                  // Numeric form of document id
            int DocType;             // Internal key to document type
            cdr::String TextId;      // With "CDR00..." prefix
            cdr::String ValStatus;   // V/I/U
            cdr::String ValDate;     // Datetime
            cdr::String Approved;    // Y/N
            cdr::String TextDocType; // Form used in document tag
            cdr::String Title;       // External title
            cdr::String Xml;         // Actual document as XML, not CDATA
            cdr::String Blob;        // Associated non-XML, if any
            cdr::String Comment;     // Free text

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

    };
}

#endif // CDR_DOC_
