/*
 * $Id: CdrCache.h,v 1.4 2004-07-02 03:17:37 ameyer Exp $
 *
 * Header for cacheing used to speed operations.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2004/07/02 01:27:23  ameyer
 * New internal routine and constants.  New comments.
 *
 * Revision 1.2  2004/05/26 01:14:30  ameyer
 * New handling of PdqKey and cdr:ref attributes.
 *
 * Revision 1.1  2004/05/14 02:04:04  ameyer
 * Header for CdrCache serverside cacheing to speed publishing.
 *
 */

#ifndef CDR_CACHE_
#define CDR_CACHE_

#include <map>
#include <set>
#include <string>
#include <iostream>

#include "CdrException.h"
#include "CdrString.h"
#include "CdrDbConnection.h"

namespace cdr {
 namespace cache {

    // Forward declaration to make typedef's work
    class Term;

    // Map of Term document ID to pointer to Term object
    //   representing the document
    typedef std::map<int, cdr::cache::Term*> TERM_MAP;

    // Set of pointers to Terms for a term's parents.
    // As a simple set, each parent occurs only once even if inserted more
    //   than once.
    typedef std::map<int, Term*> PARENT_MAP;

    /**
     * All information pertaining to a Term document and its
     * ancestors needed for denormalizing a Term reference in
     * an XML document.
     *
     * This information is used repeatedly when publishing protocols.
     * It requires numerous database accesses to acquire it.  By
     * cacheing the info we can speed up the denormalization.
     */
    class Term {
        public:
            /**
             * Initialize cacheing for terminology denormalization.
             *
             * Cacheing requests are stacked.  If there are three
             * calls to turn on cacheing, followed by two to turn
             * it off, cacheing remains on until the third request
             * arrives to turn it off.
             *
             * This enables publishing jobs to be run in parallel in
             * the future, with cacheing enabled in all of them,
             * without having one job turn it off for all.
             *
             * The age of the cache is checked on every call to
             * denormalize a term.  If it's beyond MAX_CACHE_DURATION,
             * the cache will be cleared.  A log entry is written.
             *
             * @param state        True=turn on system-wide cacheing.
             *                     False=Turn it off
             * @return             Previous state of cacheing.
             *                      True=was on before (possible) change.
             *                      False=was off.
             */
            static bool initTermCache(bool state);

            /**
             * Retrieve a utf-8 format string containing the denormalization
             * for a Term document and all its parent Term documents except
             * those that are not used in denormaliztion.
             *
             *  @param conn        Database connection.
             *  @param docIdStr    Document ID, in string form.
             *
             *  @return            Utf-8 term string.  See class comments
             *                      for documentation.
             *
             *  @throws cdr::Exception if fatal error here or below.
             *                     Caller MUST catch this exception and
             *                     not pass it back to the XSLT engine
             *                     if being called as part of an XSLT
             *                     scheme callback.
             */
            static std::string denormalizeTermId(const cdr::String &docIdStr,
                                                 cdr::db::Connection& conn);

            /**
             * Get the document ID for a term, as an integer.
             *
             *  @return             ID.
             */
            int getId() { return id; }

            /**
             * Get the Term name as a utf-8 string
             *
             *  @return             ID.
             */
            std::string getName() { return name; }

            /**
             * Get the preferred name as an utf-8 XML string.
             *
             * Lazy evaluation produces the string only upon
             * the first request for it.
             *
             * Format:
             *   &lt;Term&gt;
             *     &lt;Name&gt;preferred name of term&lt;/Name&gt;
             *   &lt;/Term&gt;
             *
             *  @return             PreferredName as XML.
             */
            std::string getNameXml();

            /**
             * Get full denormalized utf-8 XML for term and its parents.
             *
             * Lazy evaluation produces the string only upon
             * the first request for it.
             *
             * Format:
             *   &lt;Term&gt;
             *     &lt;Name&gt;preferred name of term&lt;/Name&gt;
             *     &lt;Term&gt;&lt;Name&gt;Name of a parent&lt;/Name&gt;&lt;/Term&gt;
             *     &lt;Term&gt;&lt;Name&gt;Name of another parent&lt;/Name&gt;&lt;/Term&gt;
             *     ...
             *   &lt;/Term&gt;
             *
             *  @return             Term and parents as XML.
             */
            std::string getFamilyXml();

        private:
            //--------------------------
            // Class data
            //--------------------------

            // Looking up more levels than this must be a loop error
            const static int MAX_HIERARCHY = 15;

            // Number of seconds cache can live = 1 day
            const static time_t MAX_CACHE_DURATION = (60 * 60 * 24);

            //--------------------------
            // Instance data
            //--------------------------

            int         id;         // CDR ID of term
            bool        typeOK;     // Term type is usable for denormalization
            std::string cdrRef;     // Id in cdr:ref attribute format
            std::string name;       // Preferred name for the concept
            std::string nameXml;    // Preferred name as XML string
            std::string familyXml;  // Full denormalization, see above.
            std::string pdqKey;     // PdqKey for this Term, if there is one
            PARENT_MAP  parentPtrs; // Hierarchical parents of this term

            // Functions

            /**
             * Constructor.
             *
             * For reasons having mostly to do with personal beliefs
             * about software packaging, this function does not know
             * about or access the database.  getTerm() does that work,
             * enriching a constructed object with actual database data.
             *
             * Default deconstruction is all that is needed.
             *
             *  @param docId       Document ID
             */
            Term (int docId);

            /**
             * Get a Term object corresponding to an ID.
             *
             * Tries first to get it from the cache.  If unsuccessful,
             * it constructs a new Term with info from the database,
             * denormalizes it, stores it in the cache, and returns it.
             *
             * If a term id can not be denormalized, because it doesn't
             * exist, is obsolete, or possibly for other reasons, a
             * null Term pointer is stored in the cache (so we won't
             * attempt to denormalize it again) and is returned to the
             * caller.
             *
             *  @param conn        Database connection.
             *  @param pMap        Ptr to map of id -> Term.
             *  @param depth       How many levels of the tree have already
             *                      been examined.  Stops infinite recursion.
             *  @param xsltParms   Document, in string form.
             *
             *  @return            Ptr to Term object
             *  @throws cdr::Exception if fatal error.
             */
            static Term *getTerm(cdr::db::Connection& conn, TERM_MAP *pMap,
                                 const int depth, const cdr::String &docIdStr);

            /**
             * Create the XML that begins the start of a Term.
             * Includes:
             * <ul>
             *    <li>Term start tag</li>
             *    <li>cdr namespace declaration</li>
             *    <li>optional PdqKey attribute - if pdqkey is known</li>
             *    <li>cdr:ref to term id</li>
             *    <li>PreferredName of Term</li>
             * </ul>
             *
             * Caller must provide any additional internal element content
             * if desired (i.e., parent terms) and the Term end tag.
             *
             * @return              String beginning Term XML.
             */
            std::string makeTermStart();

            /**
             * Clear all Term pointers from a map of id -> Term,
             * then delete the map itself.
             *
             *  @param pMap         Ptr to map to be cleared and deleted.
             */
            static void clearMap(TERM_MAP *pMap);
    };

    /**
     * Record an error.
     *
     *  @param msg              Message explaining the error.
     */
    void error (cdr::String msg);

    /**
     * Fatal error.
     *
     * Throw an exception that will be logged.
     *
     * If this is part of an XSLT callback, the programmer is
     * responsible for catching this before it goes back into
     * the XSLT engine.
     *
     *  @param msg              Message explaining the error.
     */
    void fatal (cdr::String msg);

  } // namespace cache
} // namespace cdr

#endif // #ifndef CDR_CACHE_
