/*
 * Header file for LinkChkProcs.cpp - custom link check procedures.
 *
 * CdrLink software must include this.  No other program should.
 *
 *                                      @author Alan Meyer
 *                                      @date February 2001
 *
 * $Id: CdrLinkProcs.h,v 1.7 2006-05-17 01:53:26 ameyer Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.6  2004/02/10 22:13:02  ameyer
 * Removed definitions of obsolete functions - makeWhere() and
 * getLinkTargetRestrictions().  Neither was ever in production.
 *
 * Revision 1.5  2004/02/06 03:13:54  ameyer
 * Added info to linkChkNode to say whether a link target check is looking
 * for a specific value, or for just the existence or non-existence of the
 * element.
 *
 * Revision 1.4  2002/05/08 20:33:34  pzhang
 * Added makeSubQueries.
 *
 * Revision 1.3  2001/09/28 01:22:01  ameyer
 * Added getLinkTargetRestrictions().
 *
 * Revision 1.2  2001/09/25 15:06:34  ameyer
 * Added makeWhere, proper namespaces.
 *
 * Revision 1.1  2001/04/06 00:08:29  ameyer
 * Initial revision
 *
 */

// #ifndef CDR_LINK_PROCS_
// #define CDR_LINK_PROCS_

/**@#-*/

namespace cdr {
  namespace link {

/**@#+*/

/** @pkg cdr */

/**
 * Classes for parsing, storing, and executing a rule for checking
 * whether a linked document contains one or more fields with specified
 * values.
 *
 * Calling programs (in the CdrLink package) need only know about the
 * top level class - LinkChkTargContains.  They allocate an object of
 * this class for each distinct rule to process.  For efficiency, they
 * should hold onto and re-use the created object rather than re-create
 * the parse tree each time.
 *
 * These classes are used for two different purposes that turn out to
 * have slightly different needs.  One purpose is validation.  Rules
 * can be specified that only allow links to documents meeting certain
 * criteria.  The other is picklist generation.  When a user enters
 * data that must pass validation rules, the rules can be used to generate
 * a picklist of valid values from which to select.
 *
 * These two uses turn out to be slightly different because it has become
 * necessary to restrict the domain of values that people may enter into
 * a field without invalidating old data.  To do that, we produce an
 * inclusive validation rule and a more restrictive picklist generator.
 * Old data remains valid, but new data is entered using the more
 * restrictive rules.
 *
 * This capability was implemented in order to eliminate drug combinations
 * from index term selections, without invalidating old documents that
 * linked to drug combinations.  It may or may not ever be needed for
 * anything else.
 *
 * <pre>
 * The management of rules is as follows:
 *
 *  Rules are stored in the database in the link_prop table in the
 *  following form:
 *
 *    tag relator "value" {booleanOp (tag relator "value")} ...
 *
 *  Where:
 *
 *    tag:
 *      XML tag structure mirroring the one in the query_term table.
 *        If query_term uses doctag/fieldtag, this must also use it.
 *        If query_term requires a fuller path, this must use it.
 *
 *    relator:
 *      One of the following relationships between tag and value:
 *        ==   Equal, by string matching.
 *        !=   Not equal, by string matching.
 *      Two parallel relators are used for picklist generation:
 *        +=   Equal by string matching.
 *        -=   Not equal, by string matching.
 *      The picklist relators always evaluate to true in a validation
 *      context but are actually evaluated in a picklist generation
 *      context.
 *
 *      Note: Equality tests are currently performed in SQLServer,
 *            using the default comparison, which is currently configured
 *            as case insensitive.
 *            This is easy to change by modifying the comparison code
 *            in CdrLinkProcs.cpp, evalRelation().
 *
 *    value:
 *      A double quoted string to compare against the values stored
 *        in the query term table.
 *
 *    booleanOp:
 *      One of the following:
 *        |     Boolean OR of the the left and right hand expressions.
 *        &     Boolean AND of the left and right hand expressions.
 *
 *   All Boolean operators share the same precedence, thus:
 *       A | B & C
 *     equates to:
 *       (A | B) & C
 *
 *  Parentheses are supported and must be used if precedence is required.
 *
 *  Expressions are evaluated left to right, with short circuiting.
 *
 * REQUIREMENTS NOTES:
 *
 *  This package is a generalization of the requirement for checking that
 *  an index term UI link points to term with the correct type, for example
 *  it will be used to check the following requirement:
 *
 *      /Term/Type == "diagnosis"
 *
 * IMPLEMENTATION NOTES:
 *
 *  When the link manager finds a requirement to check a field using one
 *  of these rules it will:
 *
 *      Search already processed rules (a vector of LinkChkTargContains
 *      objects) to see if it has seen and parsed the rule.
 *
 *      If it doesn't already exist, then
 *        Build a parse tree representing the complete validation expression.
 *
 *      Execute the parsed rule.
 * </pre>
 *
 *                                             @author: Alan Meyer
 *                                                date: January, 2001
 */

/**
 * Relationships between a field tag and a value
 *
 * We may wish to add more, later, e.g., to distinguish between
 * case sensitive/insensitive compares, greater than/less than, etc.
 */
typedef enum LinkChkRelator {
    relEqual,           // Equal
    relNotEqual,        // Not equal
    pickListEqual,      // Equal for picklist purposes only
    pickListNotEqual    // Not equal for picklist purposes only
} LinkChkRelator;

typedef enum LinkChkBoolOp {
    boolAnd,
    boolOr
} LinkChkBoolOp;

/**
 * Types of nodes we allow.
 */
typedef enum LinkChkNodeType {
    typeRel,            // Tag Relator Value - node
    typePair            // Node boolOp Node - node
} LinkChkNodeType;


/**
 * A single node in the expression parse tree.
 *
 * This is a base class allowing us to build a parse tree combining two
 * different types of subclass - one for simple expressions and one for
 * complex expressions containing two subexpressions joined by a Boolean
 * operator.
 */

class LinkChkNode {
    public:
        /**
         * Subclass returns its type
         */
        virtual LinkChkNodeType getNodeType () = 0;

        /**
         * Generate sub-queries for a complete parse tree.
         *
         * The SQL generated here is contains potentially complex conditions
         * to use in searching the query_term table for linked documents
         * that qualify for inclusion in the pick list.
         *
         * The generated SQL is not complete.  It is the caller's
         * responsibility to generate everything up to the WHERE clause,
         * to create his own WHERE clause with primary search information
         * (e.g., a pattern string for the titles of the documents he
         * wants), and then to append this clause to his own using " AND "
         * as appropriate.
         *
         * <pre>
         * Example SQL to generate:
         *
         *   Part before the makeWhere:
         *     SELECT d.id, d.title FROM document d, doc_type t,
         *       query_term q
         *     WHERE d.title like 'br%' AND d.doc_type=t.id AND
         *       t.name='Term' AND q.doc_id=d.id AND
         * </pre>
         *
         * The original version of this encountered problems with
         *  SQL Server executing a query containing 3 query_term tables
         *  joined, although joining 2 tables seemed fine.
         * This version therefore uses EXISTS and NOT EXISTS subqueries
         *  to achieve the limitation required.
         *
         * @param  query      Ptr to string containing the query to be returned.
         * @param  cdrId      A table alias followed by column name for CDR Id.
         * @throws            CdrException if syntax or other error.
         */
        void makeSubQueries (std::string& query,
                             std::string& cdrid);
};


/**
 * A simple expression representing a tag, relator, and value.
 */

class LinkChkRelation : public LinkChkNode {
    private:
        std::string    tag;         // Tag in format used in query_term table
        LinkChkRelator relator;     // Relationship
        std::string    value;       // Value to compare against linked doc
        bool           chkValue;
                                    // 0=ignore value, just check existence

    public:

        /**
         * Constructor takes the 3 fields in a relationship and initializes
         * the object with them.
         */
        LinkChkRelation (std::string rtag, LinkChkRelator rrel,
                         std::string rval, bool ckVal) : tag (rtag),
                         relator (rrel), value (rval), chkValue (ckVal) {}

        /**
         * Report this as a relator node
         */
        LinkChkNodeType getNodeType() { return typeRel; }

        /**
         * Accessors
         */
        std::string    getTag()      { return tag;      }
        LinkChkRelator getRelator()  { return relator;  }
        std::string    getValue()    { return value;    }
        bool           getChkValue() { return chkValue; }

        /**
         * Evaluate a single relation expression.
         *
         * Checks to see if a relation is true for a particular document,
         * i.e., does the document have a field with the expected value
         * in it or not.
         *
         * We don't check the documents themselves.
         *
         * In order for an expression to evaluate true, there must be an
         * entry in the query_term table containing tag=value for this
         * doc id.
         *
         * It is the responsibility of the programmer to insure that any
         * field to be evaluated is indexed in the query term table, and
         * that the path to the term be expressed in the same language in
         * both the query_term table and the validation rule expression.
         *
         * @param dbConn    Reference to database connection
         * @param docId     ID of doc for which the relation is to be evaluated
         *
         * @returns         True=expression evaluates true, else false.
         */
        bool evalRelation (cdr::db::Connection&, int);
};


/**
 * A complex expression containing two subnodes and a boolean operator.
 */

class LinkChkPair : public LinkChkNode {
    private:
        LinkChkNode   *lNode;       // Left hand node
        LinkChkNode   *rNode;       // Right hand node
        LinkChkBoolOp connector;    // Boolean operator connecting pair

    public:
        /**
         * Constructor takes the 3 fields and initializes the object.
         */
        LinkChkPair (LinkChkNode *lnode, LinkChkNode *rnode,
                LinkChkBoolOp connective) : lNode (lnode), rNode (rnode),
                connector (connective) {}

        /**
         * Destructor releases nodes if they exist.
         */
        ~LinkChkPair () {
            if (rNode) delete rNode;
            if (lNode) delete lNode;
        }

        /**
         * Report this as an expression pair node.
         */
        LinkChkNodeType getNodeType() { return typePair; }

        /**
         * Accessors
         */
        LinkChkNode*  getLNode()     { return lNode;     }
        LinkChkNode*  getRNode()     { return rNode;     }
        LinkChkBoolOp getConnector() { return connector; }

        /**
         * Evaluate a complete parse tree.
         *
         * More than likely it only has one expression in it, but we can
         * handle multiples.
         *
         * @param dbConn    Reference to database connection
         * @param docId     ID of doc for which the tree is to be evaluated
         *
         * @returns         True=tree evaluates true, else false.
         */

        bool evalRule (cdr::db::Connection&, int);
};


/**
 * Top level class for checking target dependencies
 *
 * This is the one to be called from outside
 */

class LinkChkTargContains {
    private:
        cdr::String ruleString;     // Copy of rule string from database
        LinkChkPair *treeTop;       // Parse tree for the rule

    public:

        /**
         * Constructor.
         *
         * Creates a parse tree for a rule string and store the tree
         * and copy of the string in this object.
         *
         * @param  rule            Rule string to parse.
         * @throw  cdr::Exception  With specific error message if
         *                         string is invalid.
         */
        LinkChkTargContains (cdr::String linkRule);

        /**
         * Destructor
         */
        ~LinkChkTargContains () { if (treeTop) delete treeTop; }

        /**
         * Compare passed rule with stored rule.
         *
         * If they are the same, tell the caller so that he can
         * execute this object without re-parsing the rule.
         *
         * @param  chkRule      Rule string to compare to stored copy.
         *
         * @return              True=Rules match.
         */
        bool ruleCompare (cdr::String& rule) {
            return (!(ruleString != rule));
        }

        /**
         * Get the top of the tree.  Useful in invoking a makeWhere()
         * to generate a SQL representation of the rule.
         *
         * @return              Tree top
         */
        LinkChkPair *getTreeTop() { return treeTop; }

        /**
         * Evaluate the stored rule, reporting to the user whether
         * a target document does or does not conform.
         *
         * @param  docId        Evaluate rule for this target doc id.
         * @param  dbConn       Reference to database connection
         *
         * @return              True=Target doc passes rule, false=fails.
         * @throw               cdr::Exception if SQL Server error.
         */
        bool ruleEval (cdr::db::Connection& dbConn, int docId) {
            return (treeTop->evalRule (dbConn, docId));
        }
};

 } // namespaces
}

// #endif // CDR_LINK_PROCS_
